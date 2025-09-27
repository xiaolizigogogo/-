/*
 * ESP32 AP模式 - 修改引脚版本
 * 功能：创建WiFi热点，连接Mega2560主设备，实现无线数据同步
 * 使用GPIO16和GPIO17避免与烧录引脚冲突
 */

#include <WiFi.h>
#include <WiFiUDP.h>

// AP模式配置
const char* ap_ssid = "ESP32_MASTER_AP";
const char* ap_password = "12345678";

// UDP配置
WiFiUDP udp;
const int udp_port = 1234;

// 串口配置 - 使用GPIO16和GPIO17避免与烧录引脚冲突
#define MEGA_TX_PIN 16     // ESP32 TX (原来是GPIO1)
#define MEGA_RX_PIN 17     // ESP32 RX (原来是GPIO3)
#define STATUS_LED_PIN 2   // 状态指示

// 在ESP32上使用Serial2与Mega2560通信
#define megaSerial Serial2

// 系统状态
bool apStarted = false;
bool megaConnected = false;
bool clientConnected = false;
unsigned long lastHeartbeat = 0;

void setup() {
  // 先初始化调试串口
  Serial.begin(115200);
  delay(100);
  
  Serial.println("==========================================");
  Serial.println("ESP32 AP模式启动 - 修改引脚版本");
  Serial.println("==========================================");
  
  // 初始化引脚
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  
  // 初始化与Mega2560的通信 - 使用Serial2
  megaSerial.begin(115200, SERIAL_8N1, MEGA_RX_PIN, MEGA_TX_PIN);
  
  // 等待启动完成
  delay(2000);
  
  // 启动AP模式
  startAP();
  
  // 初始化与Mega2560的通信
  initMegaCommunication();
  
  Serial.println("ESP32 AP模式初始化完成！");
}

void loop() {
  // 处理与Mega2560的通信
  handleMegaCommunication();
  
  // 处理UDP通信
  handleUDPCommunication();
  
  // 发送心跳
  sendHeartbeat();
  
  delay(10);
}

// 启动AP模式
void startAP() {
  Serial.println("启动AP模式...");
  
  // 启动WiFi热点
  WiFi.softAP(ap_ssid, ap_password);
  
  // 获取AP的IP地址
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP地址: ");
  Serial.println(IP);
  Serial.print("热点名称: ");
  Serial.println(ap_ssid);
  Serial.print("热点密码: ");
  Serial.println(ap_password);
  
  // 启动UDP服务器
  udp.begin(udp_port);
  Serial.println("UDP服务器已启动，端口: " + String(udp_port));
  
  apStarted = true;
  Serial.println("AP模式启动成功！");
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
  
  // 通过UDP广播到所有连接的客户端
  if (apStarted && clientConnected) {
    broadcastUDPData(data);
  }
}

// 处理从设备数据
void handleSlaveData(String data) {
  Serial.println("处理从设备数据: " + data);
  
  // 通过UDP广播到所有连接的客户端
  if (apStarted && clientConnected) {
    broadcastUDPData(data);
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
  clientConnected = true;
  Serial.println("收到远程心跳，客户端连接正常");
}

// 广播UDP数据
void broadcastUDPData(String data) {
  // 广播到AP网络中的所有设备
  IPAddress broadcastIP = WiFi.softAPIP();
  broadcastIP[3] = 255; // 广播地址
  
  udp.beginPacket(broadcastIP, udp_port);
  udp.print(data);
  udp.endPacket();
  
  Serial.println("广播UDP数据: " + data);
}

// 发送心跳
void sendHeartbeat() {
  if (millis() - lastHeartbeat > 10000) { // 每10秒发送一次
    if (apStarted) {
      broadcastUDPData("HEARTBEAT");
      lastHeartbeat = millis();
    }
  }
}
