/*
 * ESP32-S3 从控制器 - 使用实际引脚
 * 功能：双核处理、WiFi + 蓝牙5.0
 * 通信：Serial1 (TX0/RX0) 与Mega2560通信
 */

#include <WiFi.h>
#include <BluetoothSerial.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <esp_timer.h>
#include <esp_system.h>

// ESP32-S3 实际引脚定义
#define ESP32_TX_PIN 1   // TX0 (GPIO1)
#define ESP32_RX_PIN 3   // RX0 (GPIO3)
#define ESP32_STATUS_PIN 2  // D2 (GPIO2)
#define LED_PIN 13       // D13 (GPIO13)
#define RELAY_PIN 5      // D5 (GPIO5)
#define MOTOR_PIN 6      // D6 (GPIO6)
#define SENSOR_PIN A0    // A0 (GPIO0)

// 通信配置
#define BAUD_RATE 115200
#define JSON_BUFFER_SIZE 1024
#define WDT_TIMEOUT 30

// WiFi配置
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";

// 蓝牙配置
BluetoothSerial SerialBT;
const char* BT_DEVICE_NAME = "ESP32-S3_Slave";

// 系统状态
struct SystemStatus {
  bool wifi_connected;
  bool bluetooth_connected;
  bool mega2560_connected;
  bool system_ready;
  unsigned long last_command_time;
  int execution_count;
  String last_command;
  int sensor_value;
  unsigned long uptime;
  int cpu_freq;
  int free_heap;
  float cpu_temp;
  int wifi_rssi;
  String mac_address;
  String ip_address;
};

// 全局变量
SystemStatus systemStatus = {false, false, false, false, 0, 0, "", 0, 0, 0, 0, 0.0, 0, "", ""};
unsigned long lastStatusCheck = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastSensorRead = 0;
unsigned long lastPerformanceCheck = 0;

// 任务句柄
TaskHandle_t wifiTaskHandle = NULL;
TaskHandle_t bluetoothTaskHandle = NULL;
TaskHandle_t sensorTaskHandle = NULL;

void setup() {
  // 初始化串口
  Serial.begin(115200);
  Serial1.begin(BAUD_RATE, SERIAL_8N1, ESP32_RX_PIN, ESP32_TX_PIN);
  
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
  
  // 等待Mega2560启动
  delay(3000);
  
  Serial.println("=== ESP32-S3 从控制器启动 ===");
  Serial.println("功能：双核处理 + WiFi + 蓝牙5.0");
  Serial.println("引脚配置：TX0=1, RX0=3, D2=2, D5=5, D6=6, D13=13, A0=0");
  Serial.println();
  
  // ESP32-S3 系统优化
  Serial.println("0. ESP32-S3 系统优化...");
  
  // 设置CPU频率
  setCpuFrequencyMhz(240);
  systemStatus.cpu_freq = getCpuFrequencyMhz();
  Serial.print("   CPU频率: ");
  Serial.print(systemStatus.cpu_freq);
  Serial.println(" MHz");
  
  // 初始化看门狗
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  Serial.println("   看门狗初始化完成");
  
  // 获取系统信息
  systemStatus.free_heap = ESP.getFreeHeap();
  systemStatus.mac_address = WiFi.macAddress();
  systemStatus.uptime = millis();
  Serial.print("   可用内存: ");
  Serial.print(systemStatus.free_heap);
  Serial.println(" bytes");
  Serial.print("   MAC地址: ");
  Serial.println(systemStatus.mac_address);
  
  // 初始化WiFi
  Serial.println("1. 初始化WiFi...");
  initWiFi();
  
  // 初始化蓝牙5.0
  Serial.println("2. 初始化蓝牙5.0...");
  initBluetooth();
  
  // 检查Mega2560连接
  Serial.println("3. 检查Mega2560连接...");
  checkMega2560Connection();
  
  // 创建多任务
  Serial.println("4. 创建多任务...");
  createTasks();
  
  // 系统就绪
  systemStatus.system_ready = true;
  
  Serial.println("=== 系统初始化完成 ===");
  Serial.println("等待控制命令...");
  Serial.println("支持的控制目标：");
  Serial.println("  relay  - 继电器控制 (D5)");
  Serial.println("  led    - LED控制 (D13)");
  Serial.println("  motor  - 电机控制 (D6)");
  Serial.println("  sensor - 传感器读取 (A0)");
  Serial.println();
}

void loop() {
  // 处理Mega2560命令
  handleMega2560Commands();
  
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
  
  // 性能监控
  if (millis() - lastPerformanceCheck > 30000) {
    lastPerformanceCheck = millis();
    checkPerformance();
  }
  
  // 喂看门狗
  esp_task_wdt_reset();
  
  delay(10);
}

// 初始化WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  
  // ESP32-S3 特有的WiFi配置
  WiFi.setTxPower(WIFI_POWER_19_5dBm);  // 最大发射功率
  WiFi.setSleep(false);  // 禁用WiFi睡眠模式
  WiFi.setAutoReconnect(true);  // 自动重连
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    systemStatus.wifi_connected = true;
    systemStatus.ip_address = WiFi.localIP().toString();
    systemStatus.wifi_rssi = WiFi.RSSI();
    
    Serial.println();
    Serial.println("   ✓ WiFi连接成功");
    Serial.print("   IP地址: ");
    Serial.println(systemStatus.ip_address);
    Serial.print("   信号强度: ");
    Serial.print(systemStatus.wifi_rssi);
    Serial.println(" dBm");
  } else {
    Serial.println();
    Serial.println("   ✗ WiFi连接失败");
  }
}

// 初始化蓝牙5.0
void initBluetooth() {
  if (SerialBT.begin(BT_DEVICE_NAME)) {
    systemStatus.bluetooth_connected = true;
    Serial.println("   ✓ 蓝牙5.0初始化成功");
    Serial.print("   设备名称: ");
    Serial.println(BT_DEVICE_NAME);
    
    // ESP32-S3 蓝牙5.0特有配置
    SerialBT.setPin("1234");  // 设置配对密码
    SerialBT.enableSSP();  // 启用安全简单配对
  } else {
    Serial.println("   ✗ 蓝牙5.0初始化失败");
  }
}

// 创建多任务
void createTasks() {
  // 创建WiFi处理任务（运行在核心0）
  xTaskCreatePinnedToCore(
    wifiTask,           // 任务函数
    "WiFiTask",         // 任务名称
    4096,               // 栈大小
    NULL,               // 参数
    1,                  // 优先级
    &wifiTaskHandle,    // 任务句柄
    0                   // 核心0
  );
  
  // 创建蓝牙处理任务（运行在核心1）
  xTaskCreatePinnedToCore(
    bluetoothTask,      // 任务函数
    "BluetoothTask",    // 任务名称
    4096,               // 栈大小
    NULL,               // 参数
    1,                  // 优先级
    &bluetoothTaskHandle, // 任务句柄
    1                   // 核心1
  );
  
  // 创建传感器处理任务（运行在核心1）
  xTaskCreatePinnedToCore(
    sensorTask,         // 任务函数
    "SensorTask",       // 任务名称
    2048,               // 栈大小
    NULL,               // 参数
    2,                  // 优先级
    &sensorTaskHandle,  // 任务句柄
    1                   // 核心1
  );
  
  Serial.println("   ✓ 多任务创建完成");
}

// WiFi处理任务
void wifiTask(void* parameter) {
  while (true) {
    // 处理WiFi通信
    handleWiFiCommunication();
    
    // 检查WiFi连接状态
    if (WiFi.status() != WL_CONNECTED) {
      systemStatus.wifi_connected = false;
      Serial.println("WiFi连接断开，尝试重连...");
      WiFi.reconnect();
    } else {
      systemStatus.wifi_connected = true;
      systemStatus.wifi_rssi = WiFi.RSSI();
    }
    
    vTaskDelay(1000 / portTICK_PERIOD_MS);  // 延迟1秒
  }
}

// 蓝牙处理任务
void bluetoothTask(void* parameter) {
  while (true) {
    // 处理蓝牙通信
    handleBluetoothCommunication();
    
    // 检查蓝牙连接状态
    systemStatus.bluetooth_connected = SerialBT.hasClient();
    
    vTaskDelay(100 / portTICK_PERIOD_MS);  // 延迟100ms
  }
}

// 传感器处理任务
void sensorTask(void* parameter) {
  while (true) {
    // 读取传感器
    systemStatus.sensor_value = analogRead(SENSOR_PIN);
    
    vTaskDelay(1000 / portTICK_PERIOD_MS);  // 延迟1秒
  }
}

// 处理Mega2560命令
void handleMega2560Commands() {
  if (Serial1.available()) {
    String command = Serial1.readStringUntil('\n');
    command.trim();
    
    if (command.length() > 0) {
      Serial.println("收到Mega2560命令: " + command);
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
  } else if (type == "performance") {
    sendPerformanceInfo();
  } else {
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
    // 控制继电器 (D5)
    digitalWrite(RELAY_PIN, value > 0 ? HIGH : LOW);
    success = true;
    Serial.println("继电器: " + String(value > 0 ? "开启" : "关闭"));
    
  } else if (target == "led") {
    // 控制LED (D13)
    digitalWrite(LED_PIN, value > 0 ? HIGH : LOW);
    success = true;
    Serial.println("LED: " + String(value > 0 ? "开启" : "关闭"));
    
  } else if (target == "motor") {
    // 控制电机 (D6)
    if (value >= 0 && value <= 255) {
      analogWrite(MOTOR_PIN, value);
      success = true;
      Serial.println("电机速度: " + String(value));
    } else {
      Serial.println("错误：电机速度超出范围 (0-255)");
    }
    
  } else if (target == "sensor") {
    // 读取传感器 (A0)
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
  String testType = doc["testType"].as<String>();
  
  Serial.println("执行测试: " + testType);
  
  bool result = false;
  
  if (testType == "wifi") {
    result = systemStatus.wifi_connected;
  } else if (testType == "bluetooth") {
    result = systemStatus.bluetooth_connected;
  } else if (testType == "mega2560") {
    result = systemStatus.mega2560_connected;
  } else if (testType == "system") {
    result = systemStatus.system_ready;
  } else if (testType == "all") {
    // 执行完整测试序列
    result = executeFullTest();
  } else {
    Serial.println("错误：未知测试类型: " + testType);
    sendErrorResponse("未知测试类型: " + testType);
    return;
  }
  
  sendTestResponse(testType, result);
}

// 执行完整测试
bool executeFullTest() {
  Serial.println("执行完整测试序列...");
  
  bool testResult = true;
  
  // 测试LED (D13)
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  delay(100);
  
  // 测试继电器 (D5)
  digitalWrite(RELAY_PIN, HIGH);
  delay(100);
  digitalWrite(RELAY_PIN, LOW);
  delay(100);
  
  // 测试电机 (D6)
  analogWrite(MOTOR_PIN, 128);
  delay(200);
  analogWrite(MOTOR_PIN, 0);
  
  // 测试传感器 (A0)
  int sensorValue = analogRead(SENSOR_PIN);
  Serial.println("传感器测试值: " + String(sensorValue));
  
  // 测试WiFi
  if (!systemStatus.wifi_connected) {
    testResult = false;
    Serial.println("WiFi测试失败");
  }
  
  // 测试蓝牙
  if (!systemStatus.bluetooth_connected) {
    testResult = false;
    Serial.println("蓝牙测试失败");
  }
  
  // 测试Mega2560连接
  if (!systemStatus.mega2560_connected) {
    testResult = false;
    Serial.println("Mega2560连接测试失败");
  }
  
  Serial.println("完整测试结果: " + String(testResult ? "通过" : "失败"));
  return testResult;
}

// 处理心跳
void handleHeartbeat(DynamicJsonDocument& doc) {
  systemStatus.last_heartbeat = millis();
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
  response["wifi_connected"] = systemStatus.wifi_connected;
  response["bluetooth_connected"] = systemStatus.bluetooth_connected;
  response["mega2560_connected"] = systemStatus.mega2560_connected;
  
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
  doc["sensor_value"] = systemStatus.sensor_value;
  
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
  doc["wifi_connected"] = systemStatus.wifi_connected;
  doc["bluetooth_connected"] = systemStatus.bluetooth_connected;
  doc["mega2560_connected"] = systemStatus.mega2560_connected;
  doc["cpu_freq"] = systemStatus.cpu_freq;
  doc["free_heap"] = systemStatus.free_heap;
  doc["cpu_temp"] = systemStatus.cpu_temp;
  doc["wifi_rssi"] = systemStatus.wifi_rssi;
  doc["ip_address"] = systemStatus.ip_address;
  doc["mac_address"] = systemStatus.mac_address;
  doc["timestamp"] = millis();
  doc["source"] = "slave";
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial1.println(jsonString);
  Serial.println("发送状态响应: " + jsonString);
}

// 发送测试响应
void sendTestResponse(String testType, bool result) {
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  doc["type"] = "test_response";
  doc["testType"] = testType;
  doc["result"] = result;
  doc["timestamp"] = millis();
  doc["source"] = "slave";
  doc["sensor_value"] = systemStatus.sensor_value;
  doc["wifi_connected"] = systemStatus.wifi_connected;
  doc["bluetooth_connected"] = systemStatus.bluetooth_connected;
  doc["mega2560_connected"] = systemStatus.mega2560_connected;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial1.println(jsonString);
  Serial.println("发送测试响应: " + jsonString);
}

// 发送性能信息
void sendPerformanceInfo() {
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  doc["type"] = "performance_info";
  doc["cpu_freq"] = systemStatus.cpu_freq;
  doc["free_heap"] = systemStatus.free_heap;
  doc["cpu_temp"] = systemStatus.cpu_temp;
  doc["wifi_rssi"] = systemStatus.wifi_rssi;
  doc["uptime"] = millis() - systemStatus.uptime;
  doc["execution_count"] = systemStatus.execution_count;
  doc["wifi_task_stack"] = uxTaskGetStackHighWaterMark(wifiTaskHandle);
  doc["bluetooth_task_stack"] = uxTaskGetStackHighWaterMark(bluetoothTaskHandle);
  doc["sensor_task_stack"] = uxTaskGetStackHighWaterMark(sensorTaskHandle);
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial1.println(jsonString);
  Serial.println("发送性能信息: " + jsonString);
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
  doc["wifi_connected"] = systemStatus.wifi_connected;
  doc["bluetooth_connected"] = systemStatus.bluetooth_connected;
  doc["mega2560_connected"] = systemStatus.mega2560_connected;
  doc["free_heap"] = systemStatus.free_heap;
  doc["cpu_temp"] = systemStatus.cpu_temp;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial1.println(jsonString);
}

// 处理WiFi通信
void handleWiFiCommunication() {
  // 这里可以添加WiFi服务器功能
  // 接收来自其他ESP32-S3的连接
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

// 检查系统状态
void checkSystemStatus() {
  // 更新系统信息
  systemStatus.free_heap = ESP.getFreeHeap();
  systemStatus.cpu_temp = temperatureRead();
  
  if (systemStatus.wifi_connected) {
    systemStatus.wifi_rssi = WiFi.RSSI();
  }
  
  // 检查Mega2560连接
  checkMega2560Connection();
  
  Serial.println("系统状态检查:");
  Serial.println("  WiFi: " + String(systemStatus.wifi_connected ? "连接" : "断开"));
  Serial.println("  蓝牙: " + String(systemStatus.bluetooth_connected ? "连接" : "断开"));
  Serial.println("  Mega2560: " + String(systemStatus.mega2560_connected ? "连接" : "断开"));
  Serial.println("  可用内存: " + String(systemStatus.free_heap) + " bytes");
  Serial.println("  CPU温度: " + String(systemStatus.cpu_temp) + "°C");
  Serial.println("  执行次数: " + String(systemStatus.execution_count));
}

// 检查Mega2560连接
void checkMega2560Connection() {
  // 这里可以添加Mega2560连接检测逻辑
  systemStatus.mega2560_connected = true; // 简化处理
}

// 性能监控
void checkPerformance() {
  Serial.println("=== 性能监控 ===");
  Serial.println("CPU频率: " + String(systemStatus.cpu_freq) + " MHz");
  Serial.println("可用内存: " + String(systemStatus.free_heap) + " bytes");
  Serial.println("CPU温度: " + String(systemStatus.cpu_temp) + "°C");
  Serial.println("WiFi信号: " + String(systemStatus.wifi_rssi) + " dBm");
  Serial.println("运行时间: " + String(millis() - systemStatus.uptime) + " ms");
  Serial.println("执行次数: " + String(systemStatus.execution_count));
  Serial.println("WiFi任务栈: " + String(uxTaskGetStackHighWaterMark(wifiTaskHandle)) + " bytes");
  Serial.println("蓝牙任务栈: " + String(uxTaskGetStackHighWaterMark(bluetoothTaskHandle)) + " bytes");
  Serial.println("传感器任务栈: " + String(uxTaskGetStackHighWaterMark(sensorTaskHandle)) + " bytes");
  Serial.println("================");
}
