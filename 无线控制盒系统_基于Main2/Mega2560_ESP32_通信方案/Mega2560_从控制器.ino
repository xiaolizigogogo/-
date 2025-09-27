/*
 * Mega2560从控制器 - 通过ESP32模块进行数据交互
 * 功能：接收控制命令，执行操作，发送状态反馈
 * 通信：Serial1 (RX1/TX1) 与ESP32模块通信
 */

#include <ArduinoJson.h>

// 引脚定义
#define ESP32_STATUS_PIN 2
#define LED_PIN 13
#define RELAY_PIN 4
#define MOTOR_PIN 5
#define SENSOR_PIN A0

// 通信配置
#define BAUD_RATE 115200
#define JSON_BUFFER_SIZE 512

// 系统状态
struct SystemStatus {
  bool esp32_connected;
  bool system_ready;
  unsigned long last_command_time;
  int execution_count;
  String last_command;
  int sensor_value;
  unsigned long uptime;
};

// 全局变量
SystemStatus systemStatus = {false, false, 0, 0, "", 0, 0};
unsigned long lastStatusCheck = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastSensorRead = 0;

void setup() {
  // 初始化串口
  Serial.begin(115200);
  Serial1.begin(BAUD_RATE);
  
  // 初始化引脚
  pinMode(ESP32_STATUS_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(SENSOR_PIN, INPUT);
  
  // 初始化输出状态
  digitalWrite(LED_PIN, LOW);
  digitalWrite(RELAY_PIN, LOW);
  analogWrite(MOTOR_PIN, 0);
  
  // 等待ESP32模块启动
  delay(3000);
  
  Serial.println("=== Mega2560从控制器启动 ===");
  Serial.println("功能：通过ESP32模块接收控制命令");
  Serial.println();
  
  // 检查ESP32连接状态
  checkESP32Connection();
  
  // 系统就绪
  systemStatus.system_ready = true;
  systemStatus.uptime = millis();
  
  Serial.println("系统初始化完成，等待控制命令...");
  Serial.println("支持的控制目标：");
  Serial.println("  relay  - 继电器控制 (引脚4)");
  Serial.println("  led    - LED控制 (引脚13)");
  Serial.println("  motor  - 电机控制 (引脚5)");
  Serial.println("  sensor - 传感器读取 (A0)");
  Serial.println();
}

void loop() {
  // 处理ESP32命令
  handleESP32Commands();
  
  // 读取传感器
  readSensor();
  
  // 检查按钮输入
  handleButtonInput();
  
  // 定期状态检查
  if (millis() - lastStatusCheck > 5000) {
    lastStatusCheck = millis();
    checkESP32Connection();
  }
  
  // 发送心跳
  if (millis() - lastHeartbeat > 10000) {
    lastHeartbeat = millis();
    sendHeartbeat();
  }
  
  delay(10);
}

// 处理ESP32命令
void handleESP32Commands() {
  if (Serial1.available()) {
    String command = Serial1.readStringUntil('\n');
    command.trim();
    
    if (command.length() > 0) {
      Serial.println("收到命令: " + command);
      processCommand(command);
    }
  }
}

// 处理命令
void processCommand(String command) {
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  DeserializationError error = deserializeJson(doc, command);
  
  if (error) {
    Serial.println("JSON解析错误: " + String(error.c_str()));
    sendErrorResponse("JSON解析失败");
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
    sendErrorResponse("未知命令类型: " + type);
  }
}

// 处理控制命令
void handleControlCommand(DynamicJsonDocument& doc) {
  String target = doc["target"].as<String>();
  int value = doc["value"].as<int>();
  
  Serial.println("执行控制: " + target + " = " + String(value));
  
  bool success = false;
  
  if (target == "relay") {
    // 控制继电器
    digitalWrite(RELAY_PIN, value > 0 ? HIGH : LOW);
    success = true;
    Serial.println("继电器: " + String(value > 0 ? "开启" : "关闭"));
    
  } else if (target == "led") {
    // 控制LED
    digitalWrite(LED_PIN, value > 0 ? HIGH : LOW);
    success = true;
    Serial.println("LED: " + String(value > 0 ? "开启" : "关闭"));
    
  } else if (target == "motor") {
    // 控制电机
    if (value >= 0 && value <= 255) {
      analogWrite(MOTOR_PIN, value);
      success = true;
      Serial.println("电机速度: " + String(value));
    } else {
      Serial.println("错误：电机速度超出范围 (0-255)");
    }
    
  } else if (target == "sensor") {
    // 读取传感器
    systemStatus.sensor_value = analogRead(SENSOR_PIN);
    success = true;
    Serial.println("传感器值: " + String(systemStatus.sensor_value));
    
  } else {
    Serial.println("错误：未知控制目标: " + target);
  }
  
  // 更新状态
  if (success) {
    systemStatus.last_command = target;
    systemStatus.last_command_time = millis();
    systemStatus.execution_count++;
  }
  
  // 发送响应
  sendControlResponse(target, value, success);
}

// 处理状态请求
void handleStatusRequest(DynamicJsonDocument& doc) {
  Serial.println("发送状态信息...");
  sendStatusResponse();
}

// 处理测试命令
void handleTestCommand(DynamicJsonDocument& doc) {
  Serial.println("执行测试...");
  
  // 执行测试序列
  bool testResult = true;
  
  // 测试LED
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  delay(100);
  
  // 测试继电器
  digitalWrite(RELAY_PIN, HIGH);
  delay(100);
  digitalWrite(RELAY_PIN, LOW);
  delay(100);
  
  // 测试电机
  analogWrite(MOTOR_PIN, 128);
  delay(200);
  analogWrite(MOTOR_PIN, 0);
  
  // 测试传感器
  int sensorValue = analogRead(SENSOR_PIN);
  Serial.println("传感器测试值: " + String(sensorValue));
  
  sendTestResponse(testResult);
}

// 处理心跳
void handleHeartbeat(DynamicJsonDocument& doc) {
  Serial.println("收到心跳，系统运行正常");
}

// 处理初始化命令
void handleInitCommand(DynamicJsonDocument& doc) {
  String version = doc["version"].as<String>();
  Serial.println("收到初始化命令，版本: " + version);
  
  // 发送初始化响应
  DynamicJsonDocument response(JSON_BUFFER_SIZE);
  response["type"] = "init_response";
  response["timestamp"] = millis();
  response["source"] = "slave";
  response["version"] = "1.0";
  response["system_ready"] = systemStatus.system_ready;
  
  String jsonString;
  serializeJson(response, jsonString);
  
  Serial1.println(jsonString);
  Serial.println("发送初始化响应: " + jsonString);
}

// 发送控制响应
void sendControlResponse(String target, int value, bool success) {
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  doc["type"] = "control_response";
  doc["target"] = target;
  doc["value"] = value;
  doc["success"] = success;
  doc["timestamp"] = millis();
  doc["source"] = "slave";
  doc["execution_count"] = systemStatus.execution_count;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial1.println(jsonString);
  Serial.println("发送控制响应: " + jsonString);
}

// 发送状态响应
void sendStatusResponse() {
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  doc["type"] = "status_response";
  doc["system_ready"] = systemStatus.system_ready;
  doc["execution_count"] = systemStatus.execution_count;
  doc["last_command"] = systemStatus.last_command;
  doc["last_command_time"] = systemStatus.last_command_time;
  doc["sensor_value"] = systemStatus.sensor_value;
  doc["uptime"] = millis() - systemStatus.uptime;
  doc["timestamp"] = millis();
  doc["source"] = "slave";
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial1.println(jsonString);
  Serial.println("发送状态响应: " + jsonString);
}

// 发送测试响应
void sendTestResponse(bool result) {
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  doc["type"] = "test_response";
  doc["result"] = result;
  doc["timestamp"] = millis();
  doc["source"] = "slave";
  doc["sensor_value"] = systemStatus.sensor_value;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial1.println(jsonString);
  Serial.println("发送测试响应: " + jsonString);
}

// 发送错误响应
void sendErrorResponse(String error) {
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  doc["type"] = "error";
  doc["error"] = error;
  doc["timestamp"] = millis();
  doc["source"] = "slave";
  
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
  doc["source"] = "slave";
  doc["system_ready"] = systemStatus.system_ready;
  doc["execution_count"] = systemStatus.execution_count;
  doc["uptime"] = millis() - systemStatus.uptime;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial1.println(jsonString);
}

// 读取传感器
void readSensor() {
  if (millis() - lastSensorRead > 1000) {
    lastSensorRead = millis();
    systemStatus.sensor_value = analogRead(SENSOR_PIN);
  }
}

// 处理按钮输入
void handleButtonInput() {
  // 这里可以添加按钮处理逻辑
  // 例如：手动触发状态发送
}

// 检查ESP32连接状态
void checkESP32Connection() {
  bool status = digitalRead(ESP32_STATUS_PIN);
  systemStatus.esp32_connected = status;
  
  if (status) {
    Serial.println("ESP32模块连接正常");
  } else {
    Serial.println("警告：ESP32模块连接异常");
  }
}
