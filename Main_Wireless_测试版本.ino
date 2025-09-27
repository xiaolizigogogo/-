/*
 * Main_Wireless 测试版本
 * 简化版本，用于测试基本的数据同步功能
 * 不包含完整的硬件控制逻辑，专注于通信测试
 */

#include <WiFi.h>
#include <WiFiUDP.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

// 设备角色选择
#define DEVICE_ROLE 1  // 1=主设备, 2=子设备

// WiFi配置
const char* ap_ssid = "MainController";
const char* ap_password = "12345678";
const char* station_ssid = "MainController";
const char* station_password = "12345678";

// UDP配置
WiFiUDP udp;
const int udp_port = 1234;
IPAddress target_ip(192, 168, 4, 2);

// 测试数据结构
typedef struct {
  int steerAngle;
  int steerSpeed;
  int motorSpeed;
  int encoders[5];
  bool motorSw;
  bool steerSw;
  unsigned long timestamp;
} testData_t;

testData_t systemData;
unsigned long lastSync = 0;
const unsigned long SYNC_INTERVAL = 3000;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("==========================================");
  Serial.println("Main_Wireless 测试版本");
  Serial.println("==========================================");
  
  EEPROM.begin(512);
  
#if DEVICE_ROLE == 1
  // 主设备模式
  Serial.println("设备角色: 主设备");
  setupMaster();
#else
  // 子设备模式
  Serial.println("设备角色: 子设备");
  setupSlave();
#endif
  
  // 初始化测试数据
  initTestData();
  
  Serial.println("系统初始化完成！");
}

void loop() {
#if DEVICE_ROLE == 1
  masterLoop();
#else
  slaveLoop();
#endif
  delay(100);
}

// 主设备设置
void setupMaster() {
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
  Serial.println("等待子设备连接...");
  
  udp.begin(udp_port);
}

// 子设备设置
void setupSlave() {
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
    Serial.print("主设备IP: ");
    Serial.println(target_ip);
  } else {
    Serial.println();
    Serial.println("✗ 连接失败！");
  }
  
  udp.begin(udp_port);
}

// 初始化测试数据
void initTestData() {
  systemData.steerAngle = 150;
  systemData.steerSpeed = 25;
  systemData.motorSpeed = 50;
  systemData.motorSw = true;
  systemData.steerSw = false;
  
  for (int i = 0; i < 5; i++) {
    systemData.encoders[i] = 100 + i * 10;
  }
  
  systemData.timestamp = millis();
}

// 主设备循环
void masterLoop() {
  // 处理接收到的数据
  handleReceivedData();
  
  // 模拟数据变化
  static unsigned long lastChange = 0;
  if (millis() - lastChange > 5000) {
    systemData.steerAngle = random(100, 200);
    systemData.motorSpeed = random(30, 80);
    systemData.timestamp = millis();
    lastChange = millis();
    Serial.println("模拟数据变化");
  }
  
  // 定时发送数据
  if (millis() - lastSync > SYNC_INTERVAL) {
    sendDataToSlave();
    lastSync = millis();
  }
}

// 子设备循环
void slaveLoop() {
  // 处理接收到的数据
  handleReceivedData();
  
  // 定期请求数据
  static unsigned long lastRequest = 0;
  if (millis() - lastRequest > 5000) {
    requestDataFromMaster();
    lastRequest = millis();
  }
}

// 处理接收到的数据
void handleReceivedData() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char buffer[512];
    int len = udp.read(buffer, 512);
    if (len > 0) {
      buffer[len] = '\0';
      String receivedString = String(buffer);
      
      Serial.print("收到数据: ");
      Serial.println(receivedString);
      
      if (receivedString.startsWith("REQUEST_DATA")) {
        Serial.println("收到数据请求");
#if DEVICE_ROLE == 1
        sendDataToSlave();
#endif
      } else if (receivedString.startsWith("CONFIRM_RECEIVED")) {
        Serial.println("收到确认消息");
      } else if (receivedString.startsWith("{")) {
        // JSON数据
        parseJsonData(receivedString);
      }
    }
  }
}

// 发送数据到子设备
void sendDataToSlave() {
  DynamicJsonDocument doc(512);
  
  doc["type"] = "SYSTEM_DATA";
  doc["timestamp"] = systemData.timestamp;
  doc["steerAngle"] = systemData.steerAngle;
  doc["steerSpeed"] = systemData.steerSpeed;
  doc["motorSpeed"] = systemData.motorSpeed;
  doc["motorSw"] = systemData.motorSw;
  doc["steerSw"] = systemData.steerSw;
  
  JsonArray encoders = doc.createNestedArray("encoders");
  for (int i = 0; i < 5; i++) {
    encoders.add(systemData.encoders[i]);
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  udp.beginPacket(target_ip, udp_port);
  udp.print(jsonString);
  udp.endPacket();
  
  Serial.print("发送数据: ");
  Serial.println(jsonString);
}

// 请求数据
void requestDataFromMaster() {
  String request = "REQUEST_DATA";
  udp.beginPacket(target_ip, udp_port);
  udp.print(request);
  udp.endPacket();
  
  Serial.println("请求数据");
}

// 解析JSON数据
void parseJsonData(String jsonString) {
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, jsonString);
  
  if (error) {
    Serial.print("JSON解析错误: ");
    Serial.println(error.c_str());
    return;
  }
  
  if (doc["type"] == "SYSTEM_DATA") {
    systemData.steerAngle = doc["steerAngle"];
    systemData.steerSpeed = doc["steerSpeed"];
    systemData.motorSpeed = doc["motorSpeed"];
    systemData.motorSw = doc["motorSw"];
    systemData.steerSw = doc["steerSw"];
    systemData.timestamp = doc["timestamp"];
    
    JsonArray encoders = doc["encoders"];
    for (int i = 0; i < 5 && i < encoders.size(); i++) {
      systemData.encoders[i] = encoders[i];
    }
    
    Serial.println("=== 接收到的数据 ===");
    Serial.print("舵机角度: "); Serial.println(systemData.steerAngle);
    Serial.print("舵机速度: "); Serial.println(systemData.steerSpeed);
    Serial.print("电机速度: "); Serial.println(systemData.motorSpeed);
    Serial.print("电机开关: "); Serial.println(systemData.motorSw ? "ON" : "OFF");
    Serial.print("舵机开关: "); Serial.println(systemData.steerSw ? "ON" : "OFF");
    Serial.print("编码器: ");
    for (int i = 0; i < 5; i++) {
      Serial.print(systemData.encoders[i]);
      if (i < 4) Serial.print(", ");
    }
    Serial.println();
    Serial.println("==================");
    
    // 保存到EEPROM
    saveToEEPROM();
    
    // 发送确认
    sendConfirmation();
  }
}

// 保存到EEPROM
void saveToEEPROM() {
  EEPROM.write(0, systemData.steerAngle);
  EEPROM.write(1, systemData.steerSpeed);
  EEPROM.write(2, systemData.motorSpeed);
  EEPROM.write(3, systemData.motorSw ? 1 : 0);
  EEPROM.write(4, systemData.steerSw ? 1 : 0);
  
  for (int i = 0; i < 5; i++) {
    EEPROM.write(10 + i, systemData.encoders[i]);
  }
  
  EEPROM.write(100, 1); // 数据有效标志
  EEPROM.commit();
  
  Serial.println("数据已保存到EEPROM");
}

// 发送确认
void sendConfirmation() {
  String confirm = "CONFIRM_RECEIVED";
  udp.beginPacket(target_ip, udp_port);
  udp.print(confirm);
  udp.endPacket();
  
  Serial.println("发送确认");
}

// 从EEPROM读取数据
void loadFromEEPROM() {
  if (EEPROM.read(100) == 1) {
    systemData.steerAngle = EEPROM.read(0);
    systemData.steerSpeed = EEPROM.read(1);
    systemData.motorSpeed = EEPROM.read(2);
    systemData.motorSw = EEPROM.read(3) == 1;
    systemData.steerSw = EEPROM.read(4) == 1;
    
    for (int i = 0; i < 5; i++) {
      systemData.encoders[i] = EEPROM.read(10 + i);
    }
    
    Serial.println("从EEPROM加载数据成功");
  } else {
    Serial.println("EEPROM中无有效数据");
  }
}
