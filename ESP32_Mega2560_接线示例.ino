/*
 * ESP32 与 Arduino Mega 2560 接线示例
 * 演示串口通信、WiFi数据传输等功能
 */

// ==================== ESP32端代码 ====================
#ifdef ESP32

#include <WiFi.h>
#include <ArduinoJson.h>

// 引脚定义
#define ESP32_TX_PIN 1      // GPIO1 (TX)
#define ESP32_RX_PIN 3      // GPIO3 (RX)
#define STATUS_LED_PIN 2    // GPIO2 (状态指示)
#define WIFI_LED_PIN 4      // GPIO4 (WiFi状态)

// WiFi配置
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";
const char* serverIP = "192.168.1.100";

// 串口配置
HardwareSerial megaSerial(1); // 使用Serial1

// 系统状态
bool wifiConnected = false;
bool megaConnected = false;
unsigned long lastHeartbeat = 0;
unsigned long lastDataSend = 0;

void setup() {
  Serial.begin(115200);
  megaSerial.begin(115200, SERIAL_8N1, ESP32_RX_PIN, ESP32_TX_PIN);
  
  // 初始化引脚
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(WIFI_LED_PIN, OUTPUT);
  
  Serial.println("ESP32启动中...");
  
  // 连接WiFi
  connectToWiFi();
  
  // 初始化与Mega2560的通信
  initMegaCommunication();
  
  Serial.println("ESP32初始化完成！");
}

void loop() {
  // 处理WiFi连接
  handleWiFiConnection();
  
  // 处理与Mega2560的通信
  handleMegaCommunication();
  
  // 发送心跳
  sendHeartbeat();
  
  // 发送数据
  sendDataToMega();
  
  delay(100);
}

// 连接WiFi
void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("正在连接WiFi...");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    digitalWrite(WIFI_LED_PIN, HIGH);
    Serial.println();
    Serial.println("WiFi连接成功！");
    Serial.print("IP地址: ");
    Serial.println(WiFi.localIP());
  } else {
    wifiConnected = false;
    digitalWrite(WIFI_LED_PIN, LOW);
    Serial.println();
    Serial.println("WiFi连接失败！");
  }
}

// 初始化Mega2560通信
void initMegaCommunication() {
  Serial.println("初始化Mega2560通信...");
  
  // 发送初始化命令
  DynamicJsonDocument doc(256);
  doc["type"] = "init";
  doc["timestamp"] = millis();
  doc["source"] = "esp32";
  doc["version"] = "1.0";
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  megaSerial.println(jsonString);
  Serial.println("发送初始化命令: " + jsonString);
  
  // 等待响应
  unsigned long timeout = millis() + 5000;
  while (millis() < timeout) {
    if (megaSerial.available()) {
      String response = megaSerial.readStringUntil('\n');
      Serial.println("收到响应: " + response);
      
      if (response.indexOf("init_response") != -1) {
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
      digitalWrite(WIFI_LED_PIN, LOW);
      Serial.println("WiFi连接断开！");
    }
    connectToWiFi();
  }
}

// 处理Mega2560通信
void handleMegaCommunication() {
  if (megaSerial.available()) {
    String data = megaSerial.readStringUntil('\n');
    Serial.println("收到Mega2560数据: " + data);
    
    // 解析JSON数据
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, data);
    
    if (!error) {
      String type = doc["type"];
      
      if (type == "sensor_data") {
        handleSensorData(doc);
      } else if (type == "status_response") {
        handleStatusResponse(doc);
      } else if (type == "error") {
        handleError(doc);
      }
    }
  }
}

// 处理传感器数据
void handleSensorData(JsonDocument& doc) {
  int sensorValue = doc["value"];
  String sensorType = doc["sensor_type"];
  
  Serial.println("传感器数据 - 类型: " + sensorType + ", 值: " + String(sensorValue));
  
  // 通过WiFi发送到服务器
  if (wifiConnected) {
    sendToServer(doc);
  }
}

// 处理状态响应
void handleStatusResponse(JsonDocument& doc) {
  bool systemReady = doc["system_ready"];
  int executionCount = doc["execution_count"];
  
  Serial.println("系统状态 - 就绪: " + String(systemReady) + ", 执行次数: " + String(executionCount));
}

// 处理错误
void handleError(JsonDocument& doc) {
  String error = doc["error"];
  Serial.println("错误: " + error);
}

// 发送心跳
void sendHeartbeat() {
  if (millis() - lastHeartbeat > 10000) { // 每10秒发送一次
    DynamicJsonDocument doc(256);
    doc["type"] = "heartbeat";
    doc["timestamp"] = millis();
    doc["source"] = "esp32";
    doc["wifi_connected"] = wifiConnected;
    doc["mega_connected"] = megaConnected;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    megaSerial.println(jsonString);
    lastHeartbeat = millis();
  }
}

// 发送数据到Mega2560
void sendDataToMega() {
  if (millis() - lastDataSend > 5000) { // 每5秒发送一次
    DynamicJsonDocument doc(256);
    doc["type"] = "control";
    doc["target"] = "led";
    doc["value"] = random(0, 2);
    doc["timestamp"] = millis();
    doc["source"] = "esp32";
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    megaSerial.println(jsonString);
    Serial.println("发送控制命令: " + jsonString);
    lastDataSend = millis();
  }
}

// 发送数据到服务器
void sendToServer(JsonDocument& doc) {
  // 这里可以添加HTTP客户端代码
  // 发送数据到远程服务器
  Serial.println("发送数据到服务器...");
}

#endif

// ==================== Mega2560端代码 ====================
#ifdef ARDUINO_AVR_MEGA2560

#include <ArduinoJson.h>

// 引脚定义
#define MEGA_TX_PIN 18     // TX1
#define MEGA_RX_PIN 19     // RX1
#define STATUS_LED_PIN 2   // 状态指示
#define RELAY_PIN 4        // 继电器控制
#define MOTOR_PIN 5        // 电机控制
#define SENSOR_PIN A0      // 传感器输入

// 串口配置
HardwareSerial esp32Serial(1); // 使用Serial1

// 系统状态
bool systemReady = false;
int executionCount = 0;
unsigned long lastCommandTime = 0;
unsigned long systemUptime = 0;

void setup() {
  Serial.begin(115200);
  esp32Serial.begin(115200);
  
  // 初始化引脚
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(SENSOR_PIN, INPUT);
  
  Serial.println("Mega2560启动中...");
  
  // 初始化系统
  initSystem();
  
  Serial.println("Mega2560初始化完成！");
}

void loop() {
  // 处理ESP32通信
  handleESP32Communication();
  
  // 更新系统状态
  updateSystemStatus();
  
  // 读取传感器数据
  readSensorData();
  
  delay(100);
}

// 初始化系统
void initSystem() {
  systemReady = true;
  systemUptime = millis();
  
  // 发送初始化响应
  DynamicJsonDocument doc(256);
  doc["type"] = "init_response";
  doc["timestamp"] = millis();
  doc["source"] = "mega2560";
  doc["version"] = "1.0";
  doc["system_ready"] = systemReady;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  esp32Serial.println(jsonString);
  Serial.println("发送初始化响应: " + jsonString);
}

// 处理ESP32通信
void handleESP32Communication() {
  if (esp32Serial.available()) {
    String data = esp32Serial.readStringUntil('\n');
    Serial.println("收到ESP32数据: " + data);
    
    // 解析JSON数据
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, data);
    
    if (!error) {
      String type = doc["type"];
      
      if (type == "control") {
        handleControlCommand(doc);
      } else if (type == "heartbeat") {
        handleHeartbeat(doc);
      } else if (type == "init") {
        handleInit(doc);
      }
    } else {
      // 发送错误响应
      sendError("JSON解析失败");
    }
  }
}

// 处理控制命令
void handleControlCommand(JsonDocument& doc) {
  String target = doc["target"];
  int value = doc["value"];
  
  bool success = false;
  
  if (target == "relay") {
    digitalWrite(RELAY_PIN, value > 0 ? HIGH : LOW);
    success = true;
    Serial.println("继电器控制: " + String(value > 0 ? "开启" : "关闭"));
  } else if (target == "led") {
    digitalWrite(STATUS_LED_PIN, value > 0 ? HIGH : LOW);
    success = true;
    Serial.println("LED控制: " + String(value > 0 ? "开启" : "关闭"));
  } else if (target == "motor") {
    analogWrite(MOTOR_PIN, value);
    success = true;
    Serial.println("电机控制: " + String(value));
  } else {
    sendError("未知控制目标: " + target);
    return;
  }
  
  if (success) {
    executionCount++;
    lastCommandTime = millis();
    
    // 发送控制响应
    DynamicJsonDocument response(256);
    response["type"] = "control_response";
    response["target"] = target;
    response["value"] = value;
    response["success"] = true;
    response["timestamp"] = millis();
    response["source"] = "mega2560";
    response["execution_count"] = executionCount;
    
    String jsonString;
    serializeJson(response, jsonString);
    
    esp32Serial.println(jsonString);
    Serial.println("发送控制响应: " + jsonString);
  }
}

// 处理心跳
void handleHeartbeat(JsonDocument& doc) {
  bool esp32WifiConnected = doc["wifi_connected"];
  bool esp32MegaConnected = doc["mega_connected"];
  
  Serial.println("ESP32状态 - WiFi: " + String(esp32WifiConnected ? "已连接" : "未连接") + 
                 ", Mega: " + String(esp32MegaConnected ? "已连接" : "未连接"));
  
  // 发送心跳响应
  DynamicJsonDocument response(256);
  response["type"] = "heartbeat";
  response["timestamp"] = millis();
  response["source"] = "mega2560";
  response["system_ready"] = systemReady;
  response["execution_count"] = executionCount;
  response["uptime"] = millis() - systemUptime;
  
  String jsonString;
  serializeJson(response, jsonString);
  
  esp32Serial.println(jsonString);
}

// 处理初始化
void handleInit(JsonDocument& doc) {
  String version = doc["version"];
  Serial.println("收到初始化命令，版本: " + version);
  
  // 发送初始化响应
  DynamicJsonDocument response(256);
  response["type"] = "init_response";
  response["timestamp"] = millis();
  response["source"] = "mega2560";
  response["version"] = "1.0";
  response["system_ready"] = systemReady;
  
  String jsonString;
  serializeJson(response, jsonString);
  
  esp32Serial.println(jsonString);
}

// 更新系统状态
void updateSystemStatus() {
  // 这里可以添加系统状态更新逻辑
  // 例如：检查传感器状态、更新执行计数等
}

// 读取传感器数据
void readSensorData() {
  static unsigned long lastSensorRead = 0;
  
  if (millis() - lastSensorRead > 2000) { // 每2秒读取一次
    int sensorValue = analogRead(SENSOR_PIN);
    
    // 发送传感器数据
    DynamicJsonDocument doc(256);
    doc["type"] = "sensor_data";
    doc["sensor_type"] = "analog";
    doc["value"] = sensorValue;
    doc["timestamp"] = millis();
    doc["source"] = "mega2560";
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    esp32Serial.println(jsonString);
    Serial.println("发送传感器数据: " + jsonString);
    
    lastSensorRead = millis();
  }
}

// 发送错误
void sendError(String error) {
  DynamicJsonDocument doc(256);
  doc["type"] = "error";
  doc["error"] = error;
  doc["timestamp"] = millis();
  doc["source"] = "mega2560";
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  esp32Serial.println(jsonString);
  Serial.println("发送错误: " + error);
}

#endif
