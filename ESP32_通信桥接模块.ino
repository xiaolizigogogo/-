/*
 * ESP32 通信桥接模块
 * 功能：连接两个Mega2560设备，实现无线数据同步
 * 架构：Mega2560主设备 ←→ ESP32 ←→ WiFi/蓝牙 ←→ ESP32 ←→ Mega2560从设备
 */

#include <WiFi.h>
#include <WiFiUDP.h>
#include <ArduinoJson.h>

// WiFi配置

const char* ssid = "ESP32_A";
const char* password = "12345678";
// UDP配置
WiFiUDP udp;
const int udp_port = 1234;
IPAddress target_ip(192, 168, 1, 101); // 目标ESP32的IP地址（第二个设备）

// 串口配置
#define MEGA_TX_PIN 1      // ESP32 TX
#define MEGA_RX_PIN 3      // ESP32 RX
#define STATUS_LED_PIN 2   // 状态指示

// 在ESP32上使用Serial1与Mega2560通信
#define megaSerial Serial1

// 系统状态
bool wifiConnected = false;
bool megaConnected = false;
bool remoteConnected = false;
unsigned long lastHeartbeat = 0;
unsigned long lastDataSync = 0;

// 数据缓存
String lastMasterData = "";
String lastSlaveData = "";
unsigned long lastMasterUpdate = 0;
unsigned long lastSlaveUpdate = 0;

void setup() {
  Serial.begin(115200);
  megaSerial.begin(115200, SERIAL_8N1, MEGA_RX_PIN, MEGA_TX_PIN);
  
  // 初始化引脚
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  
  Serial.println("==========================================");
  Serial.println("ESP32 通信桥接模块启动");
  Serial.println("==========================================");
  
  // 连接WiFi
  connectToWiFi();
  
  // 初始化与Mega2560的通信
  initMegaCommunication();
  
  Serial.println("ESP32桥接模块初始化完成！");
}

void loop() {
  // 处理WiFi连接
  handleWiFiConnection();
  
  // 处理与Mega2560的通信
  handleMegaCommunication();
  
  // 处理UDP通信
  handleUDPCommunication();
  
  // 发送心跳
  sendHeartbeat();
  
  // 同步数据
  syncData();
  
  delay(10);
}

// 连接WiFi
void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("正在连接WiFi...");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println();
    Serial.println("WiFi连接成功！");
    Serial.print("IP地址: ");
    Serial.println(WiFi.localIP());
    
    // 启动UDP
    udp.begin(udp_port);
    Serial.println("UDP服务器已启动，端口: " + String(udp_port));
  } else {
    wifiConnected = false;
    Serial.println();
    Serial.println("WiFi连接失败！");
  }
}

// 初始化Mega2560通信
void initMegaCommunication() {
  Serial.println("初始化Mega2560通信...");
  
  // 发送初始化响应
  megaSerial.println("ESP32_READY");
  Serial.println("发送ESP32_READY到Mega2560");
  
  // 等待Mega2560响应
  unsigned long timeout = millis() + 10000;
  while (millis() < timeout) {
    if (megaSerial.available()) {
      String response = megaSerial.readStringUntil('\n');
      response.trim();
      
      Serial.println("收到Mega2560消息: " + response);
      
      if (response == "INIT_MASTER" || response == "SLAVE_READY") {
        megaConnected = true;
        digitalWrite(STATUS_LED_PIN, HIGH);
        Serial.println("Mega2560连接成功！");
        return;
      }
    }
    delay(100);
  }
  
  megaConnected = false;
  digitalWrite(STATUS_LED_PIN, LOW);
  Serial.println("Mega2560连接超时！");
}

// 处理WiFi连接
void handleWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiConnected) {
      wifiConnected = false;
      remoteConnected = false;
      Serial.println("WiFi连接断开！");
    }
    connectToWiFi();
  }
}

// 处理Mega2560通信
void handleMegaCommunication() {
  if (megaSerial.available()) {
    String data = megaSerial.readStringUntil('\n');
    data.trim();
    
    Serial.println("收到Mega2560数据: " + data);
    
    // 处理不同类型的数据
    if (data.startsWith("MASTER_DATA:")) {
      handleMasterData(data);
    } else if (data.startsWith("SLAVE_STATUS:") || data.startsWith("SLAVE_DATA:")) {
      handleSlaveData(data);
    } else if (data == "INIT_MASTER" || data == "SLAVE_READY") {
      megaConnected = true;
      digitalWrite(STATUS_LED_PIN, HIGH);
      Serial.println("Mega2560重新连接成功！");
    }
  }
}

// 处理主设备数据
void handleMasterData(String data) {
  lastMasterData = data;
  lastMasterUpdate = millis();
  
  Serial.println("处理主设备数据: " + data);
  
  // 通过UDP发送到远程ESP32
  if (wifiConnected && remoteConnected) {
    sendUDPData(data);
  }
}

// 处理从设备数据
void handleSlaveData(String data) {
  lastSlaveData = data;
  lastSlaveUpdate = millis();
  
  Serial.println("处理从设备数据: " + data);
  
  // 通过UDP发送到远程ESP32
  if (wifiConnected && remoteConnected) {
    sendUDPData(data);
  }
}

// 处理UDP通信
void handleUDPCommunication() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char buffer[1024];
    int len = udp.read(buffer, 1024);
    if (len > 0) {
      buffer[len] = '\0';
      String receivedData = String(buffer);
      
      Serial.println("收到UDP数据: " + receivedData);
      
      // 处理远程数据
      if (receivedData.startsWith("MASTER_DATA:")) {
        handleRemoteMasterData(receivedData);
      } else if (receivedData.startsWith("SLAVE_STATUS:") || receivedData.startsWith("SLAVE_DATA:")) {
        handleRemoteSlaveData(receivedData);
      } else if (receivedData == "HEARTBEAT") {
        handleRemoteHeartbeat();
      }
    }
  }
}

// 处理远程主设备数据
void handleRemoteMasterData(String data) {
  Serial.println("收到远程主设备数据: " + data);
  
  // 转发给本地Mega2560
  if (megaConnected) {
    megaSerial.println(data);
    Serial.println("转发给本地Mega2560: " + data);
  }
}

// 处理远程从设备数据
void handleRemoteSlaveData(String data) {
  Serial.println("收到远程从设备数据: " + data);
  
  // 转发给本地Mega2560
  if (megaConnected) {
    megaSerial.println(data);
    Serial.println("转发给本地Mega2560: " + data);
  }
}

// 处理远程心跳
void handleRemoteHeartbeat() {
  remoteConnected = true;
  Serial.println("收到远程心跳，连接正常");
}

// 发送UDP数据
void sendUDPData(String data) {
  udp.beginPacket(target_ip, udp_port);
  udp.print(data);
  udp.endPacket();
  
  Serial.println("发送UDP数据: " + data);
}

// 发送心跳
void sendHeartbeat() {
  if (millis() - lastHeartbeat > 10000) { // 每10秒发送一次
    if (wifiConnected) {
      sendUDPData("HEARTBEAT");
      lastHeartbeat = millis();
    }
  }
}

// 同步数据
void syncData() {
  if (millis() - lastDataSync > 1000) { // 每1秒同步一次
    // 检查主设备数据是否需要同步
    if (lastMasterData.length() > 0 && millis() - lastMasterUpdate < 5000) {
      if (wifiConnected && remoteConnected) {
        sendUDPData(lastMasterData);
      }
    }
    
    // 检查从设备数据是否需要同步
    if (lastSlaveData.length() > 0 && millis() - lastSlaveUpdate < 5000) {
      if (wifiConnected && remoteConnected) {
        sendUDPData(lastSlaveData);
      }
    }
    
    lastDataSync = millis();
  }
}

// 获取连接状态
bool isWiFiConnected() {
  return wifiConnected;
}

bool isMegaConnected() {
  return megaConnected;
}

bool isRemoteConnected() {
  return remoteConnected;
}

// 获取系统状态
String getSystemStatus() {
  String status = "WiFi: " + String(wifiConnected ? "已连接" : "未连接") + 
                  ", Mega: " + String(megaConnected ? "已连接" : "未连接") + 
                  ", 远程: " + String(remoteConnected ? "已连接" : "未连接");
  return status;
}
