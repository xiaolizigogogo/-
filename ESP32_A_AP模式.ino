/*
 * ESP32-A 作为AP热点，等待ESP32-B连接
 * 最简单的两个ESP32直接通信方案
 */

#include <WiFi.h>
#include <WiFiUDP.h>

// AP配置
const char* ap_ssid = "ESP32_A";
const char* ap_password = "12345678";

// UDP配置
WiFiUDP udp;
const int udp_port = 1234;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("==========================================");
  Serial.println("ESP32-A AP模式 - 主设备");
  Serial.println("==========================================");
  
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
}

void loop() {
  // 检查是否有UDP数据包
  int packetSize = udp.parsePacket();
  if (packetSize) {
    // 读取数据
    char buffer[255];
    int len = udp.read(buffer, 255);
    if (len > 0) {
      buffer[len] = '\0';
      String receivedData = String(buffer);
      
      // 避免循环响应 - 只处理来自ESP32-B的原始消息
      if (receivedData.startsWith("Hello from ESP32-B:")) {
        Serial.print("收到数据: ");
        Serial.println(receivedData);
        
        // 发送简单响应
        String response = "ESP32-A确认收到";
        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        udp.print(response);
        udp.endPacket();
        
        Serial.print("发送响应: ");
        Serial.println(response);
      }
    }
  }
  
  // 定期发送心跳
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
  
  delay(10);
}
