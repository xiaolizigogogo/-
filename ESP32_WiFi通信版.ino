/*
 * ESP32 WiFi通信版
 * 功能：使用WiFi进行ESP32间通信，替代BLE
 * 特点：稳定可靠，兼容性好
 */

#include <WiFi.h>
#include <WiFiUdp.h>

// WiFi配置
const char* ssid = "YourWiFiSSID";        // 修改为您的WiFi名称
const char* password = "YourWiFiPassword"; // 修改为您的WiFi密码

// UDP通信配置
WiFiUDP udp;
const int udpPort = 12345;
const char* targetIP = "192.168.1.100";   // 目标ESP32的IP地址

// 设备配置
const char* deviceName = "ESP32_Device_1"; // 设备名称
bool isServer = true; // true=服务器模式, false=客户端模式

// 全局变量
unsigned long lastHeartbeat = 0;
int messageCount = 0;
bool wifiConnected = false;

void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(2000);
  
  Serial.println();
  Serial.println("==========================================");
  Serial.println("ESP32 WiFi通信版");
  Serial.println("==========================================");
  
  // 显示系统信息
  displaySystemInfo();
  
  // 初始化GPIO
  pinMode(2, OUTPUT); // 内置LED
  
  // 连接WiFi
  connectToWiFi();
  
  // 初始化UDP
  if (wifiConnected) {
    udp.begin(udpPort);
    Serial.println("✓ UDP通信初始化成功");
    Serial.println("本地端口: " + String(udpPort));
  }
  
  Serial.println("==========================================");
  Serial.println("WiFi通信系统初始化完成！");
  Serial.println("设备模式: " + String(isServer ? "服务器" : "客户端"));
  Serial.println("==========================================");
}

void loop() {
  // 检查WiFi连接
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    Serial.println("WiFi连接断开，尝试重连...");
    connectToWiFi();
  }
  
  if (wifiConnected) {
    // 处理UDP消息
    handleUDPMessages();
    
    // 发送心跳
    sendHeartbeat();
    
    // 发送测试消息
    sendTestMessages();
  }
  
  // 闪烁LED
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 1000) {
    lastBlink = millis();
    digitalWrite(2, !digitalRead(2));
  }
  
  // 显示状态信息
  showStatus();
  
  delay(100);
}

// 显示系统信息
void displaySystemInfo() {
  Serial.println("系统信息:");
  Serial.print("  芯片型号: ");
  Serial.println(ESP.getChipModel());
  Serial.print("  可用内存: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
  Serial.print("  CPU温度: ");
  Serial.print(temperatureRead());
  Serial.println("°C");
  Serial.println();
}

// 连接WiFi
void connectToWiFi() {
  Serial.println("连接WiFi...");
  Serial.println("SSID: " + String(ssid));
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println();
    Serial.println("✓ WiFi连接成功！");
    Serial.println("IP地址: " + WiFi.localIP().toString());
    Serial.println("MAC地址: " + WiFi.macAddress());
  } else {
    wifiConnected = false;
    Serial.println();
    Serial.println("✗ WiFi连接失败");
  }
}

// 处理UDP消息
void handleUDPMessages() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char buffer[256];
    int len = udp.read(buffer, sizeof(buffer) - 1);
    buffer[len] = '\0';
    
    String message = String(buffer);
    Serial.println("收到消息: " + message);
    messageCount++;
    
    // 处理不同类型的消息
    if (message.startsWith("heartbeat:")) {
      Serial.println("收到心跳: " + message);
    } else if (message.startsWith("test:")) {
      Serial.println("收到测试消息: " + message);
      // 发送回复
      sendUDPMessage("test_response: " + message + " from " + deviceName);
    } else if (message.startsWith("ping:")) {
      Serial.println("收到Ping: " + message);
      // 发送Pong
      sendUDPMessage("pong: " + message + " from " + deviceName);
    } else {
      Serial.println("收到未知消息: " + message);
    }
  }
}

// 发送UDP消息
void sendUDPMessage(String message) {
  if (wifiConnected) {
    udp.beginPacket(targetIP, udpPort);
    udp.write((uint8_t*)message.c_str(), message.length());
    udp.endPacket();
    Serial.println("发送消息: " + message);
  }
}

// 发送心跳
void sendHeartbeat() {
  if (millis() - lastHeartbeat > 5000) { // 每5秒发送一次
    lastHeartbeat = millis();
    
    String heartbeat = "heartbeat:" + String(millis()) + ":" + deviceName;
    sendUDPMessage(heartbeat);
  }
}

// 发送测试消息
void sendTestMessages() {
  static unsigned long lastTest = 0;
  if (millis() - lastTest > 10000) { // 每10秒发送一次
    lastTest = millis();
    
    static int testIndex = 0;
    String testMessages[] = {"test: message " + String(testIndex), 
                            "ping: " + String(millis()),
                            "status: " + String(ESP.getFreeHeap())};
    
    String testMessage = testMessages[testIndex % 3];
    sendUDPMessage(testMessage);
    testIndex++;
  }
}

// 显示状态信息
void showStatus() {
  static unsigned long lastStatusDisplay = 0;
  
  if (millis() - lastStatusDisplay > 15000) { // 每15秒显示一次
    lastStatusDisplay = millis();
    
    Serial.println();
    Serial.println("==========================================");
    Serial.println("WiFi通信状态");
    Serial.println("==========================================");
    
    Serial.println("WiFi状态:");
    Serial.println("  连接状态: " + String(wifiConnected ? "已连接" : "未连接"));
    if (wifiConnected) {
      Serial.println("  IP地址: " + WiFi.localIP().toString());
      Serial.println("  信号强度: " + String(WiFi.RSSI()) + " dBm");
    }
    
    Serial.println("通信状态:");
    Serial.println("  消息计数: " + String(messageCount));
    Serial.println("  目标IP: " + String(targetIP));
    Serial.println("  端口: " + String(udpPort));
    
    Serial.println("系统状态:");
    Serial.println("  可用内存: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("  CPU温度: " + String(temperatureRead()) + "°C");
    Serial.println("  运行时间: " + String(millis() / 1000) + " 秒");
    
    Serial.println("==========================================");
    Serial.println();
  }
}
