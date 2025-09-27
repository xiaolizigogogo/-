/*
 * ESP32-B 作为Station，连接到ESP32-A的热点
 * 最简单的两个ESP32直接通信方案
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

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("==========================================");
  Serial.println("ESP32-B Station模式 - 从设备");
  Serial.println("==========================================");
  
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
    Serial.println();
    Serial.println("✗ 连接失败！");
    Serial.println("请检查ESP32-A是否已启动");
    return;
  }
  
  // 启动UDP客户端
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
      
      // 避免循环响应 - 只处理心跳和确认消息
      if (receivedData.startsWith("心跳:") || receivedData.startsWith("ESP32-A确认收到")) {
        Serial.print("收到数据: ");
        Serial.println(receivedData);
        
        // 不发送响应，避免循环
        Serial.println("已确认收到，不发送响应");
      }
    }
  }
  
  // 定期发送数据到ESP32-A
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
  
  delay(10);
}
