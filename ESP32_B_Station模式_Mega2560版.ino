/*
 * ESP32-B Station模式 + Mega2560通信版本
 * 基于之前好用的ESP32_B_Station模式.ino，添加Mega2560串口通信
 */

#include <WiFi.h>
#include <WiFiUDP.h>

// 连接配置
const char* ssid = "ESP32_A";
const char* password = "12345678";

// UDP配置
WiFiUDP udp;
const int udp_port = 1234;
IPAddress server_ip(192, 168, 4, 1); // ESP32-A的IP地址

// 串口配置 - 与Mega2560通信
#define MEGA_TX_PIN 16     // ESP32 TX
#define MEGA_RX_PIN 17     // ESP32 RX
#define STATUS_LED_PIN 2   // 状态指示

// 在ESP32上使用Serial2与Mega2560通信
#define megaSerial Serial2

// 系统状态
bool wifiConnected = false;
bool megaConnected = false;
bool serverConnected = false;
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
  Serial.println("ESP32-B Station模式 + Mega2560通信");
  Serial.println("==========================================");
  
  // 初始化引脚
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  
  // 初始化与Mega2560的通信
  megaSerial.begin(115200, SERIAL_8N1, MEGA_RX_PIN, MEGA_TX_PIN);
  
  // 连接到ESP32-A的热点
  WiFi.begin(ssid, password);
  Serial.print("正在连接热点: ");
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
    Serial.println("✓ 连接成功！");
    Serial.print("本机IP地址: ");
    Serial.println(WiFi.localIP());
    Serial.print("服务器IP: ");
    Serial.println(server_ip);
    Serial.print("UDP端口: ");
    Serial.println(udp_port);
    Serial.println("==========================================");
  } else {
    wifiConnected = false;
    Serial.println();
    Serial.println("✗ 连接失败！");
    Serial.println("请检查ESP32-A是否已启动");
    return;
  }
  
  // 启动UDP客户端
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
      if (receivedData.startsWith("心跳:") || receivedData.startsWith("ESP32-A确认收到")) {
        handleESP32AData(receivedData);
      } else if (receivedData.startsWith("MASTER_DATA:")) {
        handleRemoteMasterData(receivedData);
      } else if (receivedData.startsWith("SLAVE_DATA:")) {
        handleRemoteSlaveData(receivedData);
      }
    }
  }
  
  // 定期发送数据到ESP32-A
  sendDataToESP32A();
  
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
  
  // 通过UDP发送到服务器
  if (wifiConnected && serverConnected) {
    sendUDPData(data);
  }
}

// 处理从设备数据
void handleSlaveData(String data) {
  lastSlaveData = data;
  lastSlaveUpdate = millis();
  
  Serial.println("处理从设备数据: " + data);
  
  // 通过UDP发送到服务器
  if (wifiConnected && serverConnected) {
    sendUDPData(data);
  }
}

// 处理ESP32-A数据
void handleESP32AData(String data) {
  Serial.print("收到ESP32-A数据: ");
  Serial.println(data);
  
  // 不发送响应，避免循环
  Serial.println("已确认收到，不发送响应");
  
  if (data.startsWith("心跳:")) {
    serverConnected = true;
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

// 发送数据到ESP32-A
void sendDataToESP32A() {
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 3000) {
    String message = "Hello from ESP32-B: " + String(millis());
    udp.beginPacket(server_ip, udp_port);
    udp.print(message);
    udp.endPacket();
    
    Serial.print("发送数据: ");
    Serial.println(message);
    lastSend = millis();
  }
}

// 发送UDP数据
void sendUDPData(String data) {
  udp.beginPacket(server_ip, udp_port);
  udp.print(data);
  udp.endPacket();
  
  Serial.println("发送UDP数据: " + data);
}

// 同步数据
void syncData() {
  if (millis() - lastDataSync > 1000) { // 每1秒同步一次
    // 检查主设备数据是否需要同步
    if (lastMasterData.length() > 0 && millis() - lastMasterUpdate < 5000) {
      if (wifiConnected && serverConnected) {
        sendUDPData(lastMasterData);
      }
    }
    
    // 检查从设备数据是否需要同步
    if (lastSlaveData.length() > 0 && millis() - lastSlaveUpdate < 5000) {
      if (wifiConnected && serverConnected) {
        sendUDPData(lastSlaveData);
      }
    }
    
    lastDataSync = millis();
  }
}
