/*
 * ESP32 简化UDP通信 - 通用版本
 * 通过修改 DEVICE_ROLE 来选择设备角色
 * DEVICE_ROLE = 1: AP模式（主设备）
 * DEVICE_ROLE = 2: Station模式（从设备）
 */

#include <WiFi.h>
#include <WiFiUDP.h>

// 设备角色配置
#define DEVICE_ROLE 1  // 1=AP模式, 2=Station模式

// 网络配置
const char* ap_ssid = "ESP32_Comm";
const char* ap_password = "12345678";
const char* station_ssid = "ESP32_Comm";
const char* station_password = "12345678";

// UDP配置
WiFiUDP udp;
const int udp_port = 1234;
IPAddress target_ip(192, 168, 4, 2); // 目标设备IP

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("==========================================");
  Serial.println("ESP32 简化UDP通信");
  Serial.println("==========================================");
  
#if DEVICE_ROLE == 1
  // AP模式 - 主设备
  Serial.println("设备角色: AP模式（主设备）");
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress IP = WiFi.softAPIP();
  
  Serial.print("热点名称: ");
  Serial.println(ap_ssid);
  Serial.print("热点密码: ");
  Serial.println(ap_password);
  Serial.print("AP IP地址: ");
  Serial.println(IP);
  
#else
  // Station模式 - 从设备
  Serial.println("设备角色: Station模式（从设备）");
  WiFi.begin(station_ssid, station_password);
  Serial.print("正在连接热点: ");
  Serial.println(station_ssid);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("✓ 连接成功！");
    Serial.print("本机IP地址: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("✗ 连接失败！");
    return;
  }
#endif

  Serial.print("UDP端口: ");
  Serial.println(udp_port);
  Serial.println("==========================================");
  
  // 启动UDP
  udp.begin(udp_port);
}

void loop() {
  // 接收UDP数据
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char buffer[255];
    int len = udp.read(buffer, 255);
    if (len > 0) {
      buffer[len] = '\0';
      String receivedData = String(buffer);
      
      // 避免循环响应
      if (receivedData.startsWith("消息:") && !receivedData.startsWith("回复:")) {
        Serial.print("收到: ");
        Serial.println(receivedData);
        
        // 发送简单确认
        String reply = "确认收到";
        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        udp.print(reply);
        udp.endPacket();
        
        Serial.print("发送: ");
        Serial.println(reply);
      }
    }
  }
  
  // 发送数据
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 2000) {
    String message = "消息: " + String(millis());
    
#if DEVICE_ROLE == 1
    // AP模式发送到Station
    udp.beginPacket(target_ip, udp_port);
#else
    // Station模式发送到AP
    udp.beginPacket("192.168.4.1", udp_port);
#endif
    
    udp.print(message);
    udp.endPacket();
    
    Serial.print("发送: ");
    Serial.println(message);
    lastSend = millis();
  }
  
  delay(10);
}
