/*
 * Mega2560主控制器 - 通过ESP32模块进行数据交互
 * 功能：发送控制命令，接收状态反馈
 * 通信：Serial1 (RX1/TX1) 与ESP32模块通信
 */

#include <ArduinoJson.h>

// 引脚定义
#define ESP32_STATUS_PIN 2
#define LED_PIN 13
#define BUTTON_PIN 3

// 通信配置
#define BAUD_RATE 115200
#define JSON_BUFFER_SIZE 512

// 系统状态
struct SystemStatus {
  bool esp32_connected;
  bool last_command_success;
  unsigned long last_heartbeat;
  int command_count;
  String last_response;
};

// 全局变量
SystemStatus systemStatus = {false, false, 0, 0, ""};
unsigned long lastStatusCheck = 0;
unsigned long lastHeartbeat = 0;

void setup() {
  // 初始化串口
  Serial.begin(115200);
  Serial1.begin(BAUD_RATE);
  
  // 初始化引脚
  pinMode(ESP32_STATUS_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // 等待ESP32模块启动
  delay(3000);
  
  Serial.println("=== Mega2560主控制器启动 ===");
  Serial.println("功能：通过ESP32模块进行数据交互");
  Serial.println();
  
  // 检查ESP32连接状态
  checkESP32Connection();
  
  // 发送初始化命令
  sendInitCommand();
  
  Serial.println("系统初始化完成，等待用户输入...");
  Serial.println("命令格式：");
  Serial.println("  control,relay,1    - 控制继电器开启");
  Serial.println("  control,led,0      - 控制LED关闭");
  Serial.println("  status             - 获取状态");
  Serial.println("  test               - 测试连接");
  Serial.println();
}

void loop() {
  // 处理串口命令
  handleSerialCommands();
  
  // 处理ESP32响应
  handleESP32Response();
  
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

// 处理串口命令
void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command.length() > 0) {
      Serial.println("收到命令: " + command);
      processCommand(command);
    }
  }
}

// 处理ESP32响应
void handleESP32Response() {
  if (Serial1.available()) {
    String response = Serial1.readStringUntil('\n');
    response.trim();
    
    if (response.length() > 0) {
      Serial.println("ESP32响应: " + response);
      processESP32Response(response);
    }
  }
}

// 处理按钮输入
void handleButtonInput() {
  static unsigned long lastButtonPress = 0;
  static bool buttonState = HIGH;
  
  bool currentState = digitalRead(BUTTON_PIN);
  
  if (currentState == LOW && buttonState == HIGH && 
      millis() - lastButtonPress > 200) {
    lastButtonPress = millis();
    
    // 发送测试命令
    sendTestCommand();
  }
  
  buttonState = currentState;
}

// 处理命令
void processCommand(String command) {
  if (command.startsWith("control,")) {
    // 控制命令格式：control,target,value
    int firstComma = command.indexOf(',');
    int secondComma = command.indexOf(',', firstComma + 1);
    
    if (firstComma > 0 && secondComma > 0) {
      String target = command.substring(firstComma + 1, secondComma);
      String value = command.substring(secondComma + 1);
      
      sendControlCommand(target, value.toInt());
    } else {
      Serial.println("错误：控制命令格式不正确");
    }
  } else if (command == "status") {
    sendStatusRequest();
  } else if (command == "test") {
    sendTestCommand();
  } else {
    Serial.println("错误：未知命令");
  }
}

// 发送控制命令
void sendControlCommand(String target, int value) {
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  doc["type"] = "control";
  doc["target"] = target;
  doc["value"] = value;
  doc["timestamp"] = millis();
  doc["source"] = "master";
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial1.println(jsonString);
  Serial.println("发送控制命令: " + jsonString);
  
  systemStatus.command_count++;
}

// 发送状态请求
void sendStatusRequest() {
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  doc["type"] = "status_request";
  doc["timestamp"] = millis();
  doc["source"] = "master";
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial1.println(jsonString);
  Serial.println("发送状态请求: " + jsonString);
}

// 发送测试命令
void sendTestCommand() {
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  doc["type"] = "test";
  doc["timestamp"] = millis();
  doc["source"] = "master";
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial1.println(jsonString);
  Serial.println("发送测试命令: " + jsonString);
}

// 发送心跳
void sendHeartbeat() {
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  doc["type"] = "heartbeat";
  doc["timestamp"] = millis();
  doc["source"] = "master";
  doc["command_count"] = systemStatus.command_count;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial1.println(jsonString);
}

// 发送初始化命令
void sendInitCommand() {
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  doc["type"] = "init";
  doc["timestamp"] = millis();
  doc["source"] = "master";
  doc["version"] = "1.0";
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial1.println(jsonString);
  Serial.println("发送初始化命令: " + jsonString);
}

// 处理ESP32响应
void processESP32Response(String response) {
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  DeserializationError error = deserializeJson(doc, response);
  
  if (error) {
    Serial.println("JSON解析错误: " + String(error.c_str()));
    return;
  }
  
  String type = doc["type"].as<String>();
  
  if (type == "status_response") {
    handleStatusResponse(doc);
  } else if (type == "control_response") {
    handleControlResponse(doc);
  } else if (type == "test_response") {
    handleTestResponse(doc);
  } else if (type == "heartbeat") {
    handleHeartbeat(doc);
  } else {
    Serial.println("未知响应类型: " + type);
  }
}

// 处理状态响应
void handleStatusResponse(DynamicJsonDocument& doc) {
  Serial.println("=== 从控制器状态 ===");
  Serial.println("系统就绪: " + String(doc["system_ready"].as<bool>() ? "是" : "否"));
  Serial.println("执行次数: " + String(doc["execution_count"].as<int>()));
  Serial.println("最后命令: " + doc["last_command"].as<String>());
  Serial.println("运行时间: " + String(doc["uptime"].as<unsigned long>()) + "ms");
  Serial.println("==================");
}

// 处理控制响应
void handleControlResponse(DynamicJsonDocument& doc) {
  bool success = doc["success"].as<bool>();
  String target = doc["target"].as<String>();
  int value = doc["value"].as<int>();
  
  Serial.println("控制结果: " + target + " = " + String(value) + 
                 " (" + (success ? "成功" : "失败") + ")");
  
  systemStatus.last_command_success = success;
  systemStatus.last_response = response;
}

// 处理测试响应
void handleTestResponse(DynamicJsonDocument& doc) {
  bool result = doc["result"].as<bool>();
  Serial.println("测试结果: " + (result ? "通过" : "失败"));
  
  // 控制LED指示
  digitalWrite(LED_PIN, result ? HIGH : LOW);
}

// 处理心跳
void handleHeartbeat(DynamicJsonDocument& doc) {
  systemStatus.last_heartbeat = millis();
  Serial.println("收到心跳，从控制器运行正常");
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
