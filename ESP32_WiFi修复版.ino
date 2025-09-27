/*
 * ESP32 WiFi修复版
 * 功能：尝试修复WiFi导致的重启问题
 * 使用更保守的WiFi配置
 */

#include <WiFi.h>

// 串口配置
#define MEGA_TX_PIN 16     // ESP32 TX
#define MEGA_RX_PIN 17     // ESP32 RX
#define STATUS_LED_PIN 2   // 状态指示

// 在ESP32上使用Serial2与Mega2560通信
#define megaSerial Serial2

// 系统状态
bool megaConnected = false;
bool wifiInitialized = false;
unsigned long lastHeartbeat = 0;

void setup() {
  // 先初始化调试串口
  Serial.begin(115200);
  delay(100);
  
  Serial.println("==========================================");
  Serial.println("ESP32 WiFi修复版启动");
  Serial.println("==========================================");
  
  // 初始化引脚
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  
  // 初始化与Mega2560的通信
  megaSerial.begin(115200, SERIAL_8N1, MEGA_RX_PIN, MEGA_TX_PIN);
  
  // 等待启动完成
  delay(3000); // 增加启动延迟
  
  Serial.println("ESP32初始化完成！");
  
  // 尝试初始化WiFi
  initWiFi();
  
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
        break;
      }
    }
    delay(100);
  }
  
  if (!megaConnected) {
    Serial.println("Mega2560连接超时！");
  }
}

void loop() {
  // 处理与Mega2560的通信
  handleMegaCommunication();
  
  // 发送心跳
  sendHeartbeat();
  
  delay(10);
}

// 初始化WiFi
void initWiFi() {
  Serial.println("尝试初始化WiFi...");
  
  try {
    // 设置WiFi模式
    WiFi.mode(WIFI_OFF);
    delay(100);
    
    // 启动AP模式
    WiFi.mode(WIFI_AP);
    delay(100);
    
    // 配置AP参数
    WiFi.softAP("ESP32_TEST", "12345678", 1, 0, 4); // 限制连接数为4
    
    // 获取IP地址
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP地址: ");
    Serial.println(IP);
    
    wifiInitialized = true;
    Serial.println("WiFi初始化成功！");
    
  } catch (...) {
    Serial.println("WiFi初始化失败！");
    wifiInitialized = false;
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
  
  // 如果WiFi正常，可以在这里添加WiFi相关功能
  if (wifiInitialized) {
    Serial.println("WiFi功能可用");
  }
}

// 处理从设备数据
void handleSlaveData(String data) {
  Serial.println("处理从设备数据: " + data);
}

// 发送心跳
void sendHeartbeat() {
  if (millis() - lastHeartbeat > 5000) { // 每5秒发送一次
    if (megaConnected) {
      megaSerial.println("ESP32_HEARTBEAT");
      Serial.println("发送心跳到Mega2560");
    }
    lastHeartbeat = millis();
  }
}
