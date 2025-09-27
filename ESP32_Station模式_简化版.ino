/*
 * ESP32 Station模式简化版 - 避免重启问题
 * 功能：连接到AP热点，连接Mega2560，实现基本通信
 */

#include <WiFi.h>
#include <WiFiUDP.h>

// Station模式配置
const char* ssid = "ESP32_MASTER_AP";
const char* password = "12345678";

// UDP配置
WiFiUDP udp;
const int udp_port = 1234;
IPAddress server_ip(192, 168, 4, 1); // AP的IP地址

// 串口配置
#define MEGA_TX_PIN 1      // ESP32 TX
#define MEGA_RX_PIN 3      // ESP32 RX
#define STATUS_LED_PIN 2   // 状态指示

// 在ESP32上使用Serial1与Mega2560通信
#define megaSerial Serial1

// 系统状态
bool wifiConnected = false;
bool megaConnected = false;
bool serverConnected = false;
unsigned long lastHeartbeat = 0;

void setup() {
  // 先初始化调试串口
  Serial.begin(115200);
  delay(100);
  
  Serial.println("==========================================");
  Serial.println("ESP32 Station模式简化版启动");
  Serial.println("==========================================");
  
  // 初始化引脚
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  
  // 初始化与Mega2560的通信
  megaSerial.begin(115200, SERIAL_8N1, MEGA_RX_PIN, MEGA_TX_PIN);
  
  // 等待启动完成
  delay(2000);
  
  // 连接WiFi
  connectToWiFi();
  
  // 初始化与Mega2560的通信
  initMegaCommunication();
  
  Serial.println("ESP32 Station简化版初始化完成！");
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
  
  delay(10);
}

// 连接WiFi
void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("正在连接WiFi热点: ");
  Serial.println(ssid);
  
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
    Serial.print("本地IP地址: ");
    Serial.println(WiFi.localIP());
    Serial.print("服务器IP地址: ");
    Serial.println(server_ip);
    
    // 启动UDP
    udp.begin(udp_port);
    Serial.println("UDP客户端已启动，端口: " + String(udp_port));
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
      serverConnected = false;
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
  Serial.println("处理主设备数据: " + data);
  
  // 通过UDP发送到服务器
  if (wifiConnected && serverConnected) {
    sendUDPData(data);
  }
}

// 处理从设备数据
void handleSlaveData(String data) {
  Serial.println("处理从设备数据: " + data);
  
  // 通过UDP发送到服务器
  if (wifiConnected && serverConnected) {
    sendUDPData(data);
  }
}

// 处理UDP通信
void handleUDPCommunication() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char buffer[512];  // 减小缓冲区大小
    int len = udp.read(buffer, 512);
    if (len > 0) {
      buffer[len] = '\0';
      String receivedData = String(buffer);
      
      Serial.println("收到UDP数据: " + receivedData);
      Serial.println("来自IP: " + udp.remoteIP().toString());
      
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
  serverConnected = true;
  Serial.println("收到远程心跳，服务器连接正常");
}

// 发送UDP数据
void sendUDPData(String data) {
  udp.beginPacket(server_ip, udp_port);
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
