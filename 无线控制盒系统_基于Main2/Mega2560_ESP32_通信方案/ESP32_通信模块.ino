/*
 * ESP32通信模块 - 作为Mega2560之间的通信桥梁
 * 功能：接收Mega2560命令，通过WiFi/蓝牙转发给另一个ESP32模块
 * 通信：Serial1 (RX1/TX1) 与Mega2560通信
 */

#include <WiFi.h>
#include <BluetoothSerial.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

// 通信配置
#define BAUD_RATE 115200
#define JSON_BUFFER_SIZE 512
#define WDT_TIMEOUT 30

// WiFi配置
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";
const char* serverIP = "192.168.1.100";  // 目标ESP32的IP地址
const int serverPort = 8080;

// 蓝牙配置
BluetoothSerial SerialBT;
const char* BT_DEVICE_NAME = "ESP32_Bridge";

// 系统状态
struct BridgeStatus {
  bool wifi_connected;
  bool bluetooth_connected;
  bool mega2560_connected;
  unsigned long last_heartbeat;
  int message_count;
  String last_message;
};

// 全局变量
BridgeStatus bridgeStatus = {false, false, false, 0, 0, ""};
WiFiClient client;
unsigned long lastStatusCheck = 0;
unsigned long lastHeartbeat = 0;

void setup() {
  // 初始化串口
  Serial.begin(115200);
  Serial1.begin(BAUD_RATE);
  
  // 等待Mega2560启动
  delay(3000);
  
  Serial.println("=== ESP32通信模块启动 ===");
  Serial.println("功能：Mega2560通信桥梁");
  Serial.println();
  
  // ESP32系统优化
  Serial.println("0. ESP32系统优化...");
  setCpuFrequencyMhz(240);
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  Serial.println("   ✓ ESP32系统优化完成");
  
  // 初始化WiFi
  Serial.println("1. 初始化WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    bridgeStatus.wifi_connected = true;
    Serial.println();
    Serial.println("   ✓ WiFi连接成功");
    Serial.print("   IP地址: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("   ✗ WiFi连接失败");
  }
  
  // 初始化蓝牙
  Serial.println("2. 初始化蓝牙...");
  if (SerialBT.begin(BT_DEVICE_NAME)) {
    bridgeStatus.bluetooth_connected = true;
    Serial.println("   ✓ 蓝牙初始化成功");
    Serial.print("   设备名称: ");
    Serial.println(BT_DEVICE_NAME);
  } else {
    Serial.println("   ✗ 蓝牙初始化失败");
  }
  
  // 检查Mega2560连接
  Serial.println("3. 检查Mega2560连接...");
  checkMega2560Connection();
  
  Serial.println("=== 系统初始化完成 ===");
  Serial.println("等待Mega2560命令...");
  Serial.println();
}

void loop() {
  // 处理Mega2560命令
  handleMega2560Commands();
  
  // 处理WiFi通信
  handleWiFiCommunication();
  
  // 处理蓝牙通信
  handleBluetoothCommunication();
  
  // 定期状态检查
  if (millis() - lastStatusCheck > 5000) {
    lastStatusCheck = millis();
    checkSystemStatus();
  }
  
  // 发送心跳
  if (millis() - lastHeartbeat > 10000) {
    lastHeartbeat = millis();
    sendHeartbeat();
  }
  
  // 喂看门狗
  esp_task_wdt_reset();
  
  delay(10);
}

// 处理Mega2560命令
void handleMega2560Commands() {
  if (Serial1.available()) {
    String command = Serial1.readStringUntil('\n');
    command.trim();
    
    if (command.length() > 0) {
      Serial.println("收到Mega2560命令: " + command);
      processMega2560Command(command);
    }
  }
}

// 处理Mega2560命令
void processMega2560Command(String command) {
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  DeserializationError error = deserializeJson(doc, command);
  
  if (error) {
    Serial.println("JSON解析错误: " + String(error.c_str()));
    sendErrorToMega2560("JSON解析失败");
    return;
  }
  
  String type = doc["type"].as<String>();
  
  if (type == "control") {
    handleControlCommand(doc);
  } else if (type == "status_request") {
    handleStatusRequest(doc);
  } else if (type == "test") {
    handleTestCommand(doc);
  } else if (type == "heartbeat") {
    handleHeartbeat(doc);
  } else if (type == "init") {
    handleInitCommand(doc);
  } else {
    Serial.println("未知命令类型: " + type);
    sendErrorToMega2560("未知命令类型: " + type);
  }
}

// 处理控制命令
void handleControlCommand(DynamicJsonDocument& doc) {
  Serial.println("转发控制命令...");
  
  // 通过WiFi转发
  if (bridgeStatus.wifi_connected) {
    forwardViaWiFi(doc);
  }
  
  // 通过蓝牙转发
  if (bridgeStatus.bluetooth_connected) {
    forwardViaBluetooth(doc);
  }
  
  // 更新状态
  bridgeStatus.message_count++;
  bridgeStatus.last_message = "control";
}

// 处理状态请求
void handleStatusRequest(DynamicJsonDocument& doc) {
  Serial.println("转发状态请求...");
  
  // 通过WiFi转发
  if (bridgeStatus.wifi_connected) {
    forwardViaWiFi(doc);
  }
  
  // 通过蓝牙转发
  if (bridgeStatus.bluetooth_connected) {
    forwardViaBluetooth(doc);
  }
  
  // 发送本地状态
  sendStatusToMega2560();
}

// 处理测试命令
void handleTestCommand(DynamicJsonDocument& doc) {
  Serial.println("执行测试...");
  
  // 测试WiFi连接
  bool wifiTest = bridgeStatus.wifi_connected;
  
  // 测试蓝牙连接
  bool bluetoothTest = bridgeStatus.bluetooth_connected;
  
  // 测试Mega2560连接
  bool mega2560Test = bridgeStatus.mega2560_connected;
  
  // 发送测试结果
  DynamicJsonDocument response(JSON_BUFFER_SIZE);
  response["type"] = "test_response";
  response["result"] = wifiTest && bluetoothTest && mega2560Test;
  response["wifi"] = wifiTest;
  response["bluetooth"] = bluetoothTest;
  response["mega2560"] = mega2560Test;
  response["timestamp"] = millis();
  response["source"] = "bridge";
  
  String jsonString;
  serializeJson(response, jsonString);
  
  Serial1.println(jsonString);
  Serial.println("发送测试响应: " + jsonString);
}

// 处理心跳
void handleHeartbeat(DynamicJsonDocument& doc) {
  bridgeStatus.last_heartbeat = millis();
  Serial.println("收到Mega2560心跳");
}

// 处理初始化命令
void handleInitCommand(DynamicJsonDocument& doc) {
  Serial.println("收到初始化命令");
  
  // 发送初始化响应
  DynamicJsonDocument response(JSON_BUFFER_SIZE);
  response["type"] = "init_response";
  response["timestamp"] = millis();
  response["source"] = "bridge";
  response["wifi_connected"] = bridgeStatus.wifi_connected;
  response["bluetooth_connected"] = bridgeStatus.bluetooth_connected;
  response["mega2560_connected"] = bridgeStatus.mega2560_connected;
  
  String jsonString;
  serializeJson(response, jsonString);
  
  Serial1.println(jsonString);
  Serial.println("发送初始化响应: " + jsonString);
}

// 通过WiFi转发
void forwardViaWiFi(DynamicJsonDocument& doc) {
  if (client.connect(serverIP, serverPort)) {
    String jsonString;
    serializeJson(doc, jsonString);
    
    client.println(jsonString);
    Serial.println("WiFi转发: " + jsonString);
    
    // 等待响应
    if (client.available()) {
      String response = client.readStringUntil('\n');
      response.trim();
      
      if (response.length() > 0) {
        Serial.println("WiFi响应: " + response);
        // 转发响应给Mega2560
        Serial1.println(response);
      }
    }
    
    client.stop();
  } else {
    Serial.println("WiFi连接失败");
  }
}

// 通过蓝牙转发
void forwardViaBluetooth(DynamicJsonDocument& doc) {
  String jsonString;
  serializeJson(doc, jsonString);
  
  SerialBT.println(jsonString);
  Serial.println("蓝牙转发: " + jsonString);
  
  // 等待响应
  if (SerialBT.available()) {
    String response = SerialBT.readStringUntil('\n');
    response.trim();
    
    if (response.length() > 0) {
      Serial.println("蓝牙响应: " + response);
      // 转发响应给Mega2560
      Serial1.println(response);
    }
  }
}

// 处理WiFi通信
void handleWiFiCommunication() {
  // 这里可以添加WiFi服务器功能
  // 接收来自其他ESP32的连接
}

// 处理蓝牙通信
void handleBluetoothCommunication() {
  if (SerialBT.available()) {
    String data = SerialBT.readStringUntil('\n');
    data.trim();
    
    if (data.length() > 0) {
      Serial.println("收到蓝牙数据: " + data);
      // 转发给Mega2560
      Serial1.println(data);
    }
  }
}

// 发送状态给Mega2560
void sendStatusToMega2560() {
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  doc["type"] = "status_response";
  doc["wifi_connected"] = bridgeStatus.wifi_connected;
  doc["bluetooth_connected"] = bridgeStatus.bluetooth_connected;
  doc["mega2560_connected"] = bridgeStatus.mega2560_connected;
  doc["message_count"] = bridgeStatus.message_count;
  doc["last_message"] = bridgeStatus.last_message;
  doc["uptime"] = millis();
  doc["timestamp"] = millis();
  doc["source"] = "bridge";
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial1.println(jsonString);
  Serial.println("发送状态响应: " + jsonString);
}

// 发送错误给Mega2560
void sendErrorToMega2560(String error) {
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  doc["type"] = "error";
  doc["error"] = error;
  doc["timestamp"] = millis();
  doc["source"] = "bridge";
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial1.println(jsonString);
  Serial.println("发送错误响应: " + jsonString);
}

// 发送心跳
void sendHeartbeat() {
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  doc["type"] = "heartbeat";
  doc["timestamp"] = millis();
  doc["source"] = "bridge";
  doc["wifi_connected"] = bridgeStatus.wifi_connected;
  doc["bluetooth_connected"] = bridgeStatus.bluetooth_connected;
  doc["mega2560_connected"] = bridgeStatus.mega2560_connected;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial1.println(jsonString);
}

// 检查系统状态
void checkSystemStatus() {
  // 检查WiFi状态
  bridgeStatus.wifi_connected = WiFi.status() == WL_CONNECTED;
  
  // 检查蓝牙状态
  bridgeStatus.bluetooth_connected = SerialBT.hasClient();
  
  // 检查Mega2560连接
  checkMega2560Connection();
  
  Serial.println("系统状态检查:");
  Serial.println("  WiFi: " + String(bridgeStatus.wifi_connected ? "连接" : "断开"));
  Serial.println("  蓝牙: " + String(bridgeStatus.bluetooth_connected ? "连接" : "断开"));
  Serial.println("  Mega2560: " + String(bridgeStatus.mega2560_connected ? "连接" : "断开"));
}

// 检查Mega2560连接
void checkMega2560Connection() {
  // 这里可以添加Mega2560连接检测逻辑
  // 例如：检查串口数据或状态引脚
  bridgeStatus.mega2560_connected = true; // 简化处理
}
