/*
 * ESP32-A AP模式 + Mega2560通信版本
 * 基于之前好用的ESP32_A_AP模式.ino，添加Mega2560串口通信
 */

#include <WiFi.h>
#include <WiFiUDP.h>

// AP配置
const char* ap_ssid = "ESP32_A";
const char* ap_password = "12345678";

// UDP配置
WiFiUDP udp;
const int udp_port = 1234;

// 串口配置 - 与Mega2560通信
#define MEGA_TX_PIN 16     // ESP32 TX
#define MEGA_RX_PIN 17     // ESP32 RX
#define STATUS_LED_PIN 2   // 状态指示

// 在ESP32上使用Serial2与Mega2560通信
#define megaSerial Serial2

// 系统状态
bool megaConnected = false;
bool clientConnected = false;
unsigned long lastHeartbeat = 0;
unsigned long lastDataSync = 0;

// 数据缓存
String lastMasterData = "";
String lastSlaveData = "";
unsigned long lastMasterUpdate = 0;
unsigned long lastSlaveUpdate = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("==========================================");
  Serial.println("ESP32-A AP模式 + Mega2560通信");
  Serial.println("==========================================");
  
  // 初始化引脚
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  
  // 初始化与Mega2560的通信
  megaSerial.begin(115200, SERIAL_8N1, MEGA_RX_PIN, MEGA_TX_PIN);
  
  // 创建热点
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress IP = WiFi.softAPIP();
  
  Serial.print("热点名称: ");
  Serial.println(ap_ssid);
  Serial.print("热点密码: ");
  Serial.println(ap_password);
  Serial.print("AP IP地址: ");
  Serial.println(IP);
  Serial.print("UDP端口: ");
  Serial.println(udp_port);
  Serial.println();
  Serial.println("等待ESP32-B连接...");
  Serial.println("==========================================");
  
  // 启动UDP服务器
  udp.begin(udp_port);
  
  // 初始化Mega2560通信
  initMegaCommunication();
}

void loop() {
  // 处理Mega2560通信
  handleMegaCommunication();
  
  // 检查是否有UDP数据包
  int packetSize = udp.parsePacket();
  if (packetSize) {
    // 读取数据
    char buffer[255];
    int len = udp.read(buffer, 255);
    if (len > 0) {
      buffer[len] = '\0';
      String receivedData = String(buffer);
      
      Serial.print("收到UDP数据: ");
      Serial.println(receivedData);
      
      // 处理不同类型的数据
      if (receivedData.startsWith("Hello from ESP32-B:")) {
        handleESP32BData(receivedData);
      } else if (receivedData.startsWith("MASTER_DATA:")) {
        handleRemoteMasterData(receivedData);
      } else if (receivedData.startsWith("SLAVE_DATA:")) {
        handleRemoteSlaveData(receivedData);
      } else if (receivedData.startsWith("心跳:")) {
        handleRemoteHeartbeat(receivedData);
      }
    }
  }
  
  // 定期发送心跳
  sendHeartbeat();
  
  // 同步数据
  syncData();
  
  delay(10);
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

// 处理Mega2560通信
void handleMegaCommunication() {
  if (megaSerial.available()) {
    String data = megaSerial.readStringUntil('\n');
    data.trim();
    
    if (data.length() > 0) {
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
}

// 处理主设备数据
void handleMasterData(String data) {
  lastMasterData = data;
  lastMasterUpdate = millis();
  
  Serial.println("处理主设备数据: " + data);
  
  // 通过UDP发送到远程ESP32
  if (clientConnected) {
    sendUDPData(data);
  }
}

// 处理从设备数据
void handleSlaveData(String data) {
  lastSlaveData = data;
  lastSlaveUpdate = millis();
  
  Serial.println("处理从设备数据: " + data);
  
  // 通过UDP发送到远程ESP32
  if (clientConnected) {
    sendUDPData(data);
  }
}

// 处理ESP32-B数据
void handleESP32BData(String data) {
  Serial.print("收到ESP32-B数据: ");
  Serial.println(data);
  
  // 发送简单响应
  String response = "ESP32-A确认收到";
  udp.beginPacket(udp.remoteIP(), udp.remotePort());
  udp.print(response);
  udp.endPacket();
  
  Serial.print("发送响应: ");
  Serial.println(response);
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
void handleRemoteHeartbeat(String data) {
  clientConnected = true;
  Serial.println("收到远程心跳，客户端连接正常");
}

// 发送UDP数据
void sendUDPData(String data) {
  udp.beginPacket("192.168.4.2", udp_port); // 发送到ESP32-B
  udp.print(data);
  udp.endPacket();
  
  Serial.println("发送UDP数据: " + data);
}

// 发送心跳
void sendHeartbeat() {
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 5000) {
    String heartbeat = "心跳: " + String(millis());
    udp.beginPacket("192.168.4.2", udp_port); // 发送到ESP32-B
    udp.print(heartbeat);
    udp.endPacket();
    
    Serial.print("发送心跳: ");
    Serial.println(heartbeat);
    lastHeartbeat = millis();
  }
}

// 同步数据
void syncData() {
  if (millis() - lastDataSync > 1000) { // 每1秒同步一次
    // 检查主设备数据是否需要同步
    if (lastMasterData.length() > 0 && millis() - lastMasterUpdate < 5000) {
      if (clientConnected) {
        sendUDPData(lastMasterData);
      }
    }
    
    // 检查从设备数据是否需要同步
    if (lastSlaveData.length() > 0 && millis() - lastSlaveUpdate < 5000) {
      if (clientConnected) {
        sendUDPData(lastSlaveData);
      }
    }
    
    lastDataSync = millis();
  }
}
