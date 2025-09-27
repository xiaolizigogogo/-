/*
 * ESP32-S3 主控制器 - 使用实际引脚
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
#define BUTTON_PIN 12    // D12 (GPIO12)

// 通信配置
#define BAUD_RATE 115200
#define JSON_BUFFER_SIZE 1024
#define WDT_TIMEOUT 30

// WiFi配置
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";
const char* serverIP = "192.168.1.100";
const int serverPort = 8080;

// 蓝牙配置
BluetoothSerial SerialBT;
const char* BT_DEVICE_NAME = "ESP32-S3_Master";

// 系统状态
struct SystemStatus {
  bool wifi_connected;
  bool bluetooth_connected;
  bool mega2560_connected;
  unsigned long last_heartbeat;
  int command_count;
  String last_response;
  int cpu_freq;
  int free_heap;
  float cpu_temp;
  int wifi_rssi;
  String mac_address;
  String ip_address;
};

// 全局变量
SystemStatus systemStatus = {false, false, false, 0, 0, "", 0, 0, 0.0, 0, "", ""};
WiFiClient client;
unsigned long lastStatusCheck = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastPerformanceCheck = 0;

// 任务句柄
TaskHandle_t wifiTaskHandle = NULL;
TaskHandle_t bluetoothTaskHandle = NULL;

void setup() {
  // 初始化串口
  Serial.begin(115200);
  Serial1.begin(BAUD_RATE, SERIAL_8N1, ESP32_RX_PIN, ESP32_TX_PIN);
  
  // 初始化引脚
  pinMode(ESP32_STATUS_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // 等待Mega2560启动
  delay(3000);
  
  Serial.println("=== ESP32-S3 主控制器启动 ===");
  Serial.println("功能：双核处理 + WiFi + 蓝牙5.0");
  Serial.println("引脚配置：TX0=1, RX0=3, D2=2, D12=12, D13=13");
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
  
  Serial.println("=== 系统初始化完成 ===");
  Serial.println("等待Mega2560命令...");
  Serial.println();
}

void loop() {
  // 处理Mega2560命令
  handleMega2560Commands();
  
  // 处理按钮输入
  handleButtonInput();
  
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
    sendStatusResponse();
  } else if (type == "test") {
    sendTestCommand(doc);
  } else if (type == "heartbeat") {
    handleHeartbeat(doc);
  } else if (type == "init") {
    handleInitCommand(doc);
  } else if (type == "performance") {
    sendPerformanceInfo();
  } else {
    sendErrorResponse("未知命令: " + type);
  }
}

// 处理控制命令
void handleControlCommand(DynamicJsonDocument& doc) {
  String target = doc["target"].as<String>();
  String action = doc["action"].as<String>();
  int value = doc["value"].as<int>();
  
  Serial.println("执行控制命令: " + target + " -> " + action + " = " + String(value));
  
  // 通过WiFi转发
  if (systemStatus.wifi_connected) {
    forwardViaWiFi(doc);
  }
  
  // 通过蓝牙转发
  if (systemStatus.bluetooth_connected) {
    forwardViaBluetooth(doc);
  }
  
  systemStatus.command_count++;
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
      Serial1.println(response);
    }
  }
}

// 发送状态响应
void sendStatusResponse() {
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  doc["type"] = "status_response";
  doc["wifi_connected"] = systemStatus.wifi_connected;
  doc["bluetooth_connected"] = systemStatus.bluetooth_connected;
  doc["mega2560_connected"] = systemStatus.mega2560_connected;
  doc["command_count"] = systemStatus.command_count;
  doc["cpu_freq"] = systemStatus.cpu_freq;
  doc["free_heap"] = systemStatus.free_heap;
  doc["cpu_temp"] = systemStatus.cpu_temp;
  doc["wifi_rssi"] = systemStatus.wifi_rssi;
  doc["ip_address"] = systemStatus.ip_address;
  doc["mac_address"] = systemStatus.mac_address;
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial1.println(jsonString);
  Serial.println("发送状态响应: " + jsonString);
}

// 发送测试命令
void sendTestCommand(DynamicJsonDocument& doc) {
  String testType = doc["testType"].as<String>();
  
  bool result = false;
  if (testType == "wifi") {
    result = systemStatus.wifi_connected;
  } else if (testType == "bluetooth") {
    result = systemStatus.bluetooth_connected;
  } else if (testType == "mega2560") {
    result = systemStatus.mega2560_connected;
  } else if (testType == "all") {
    result = systemStatus.wifi_connected && systemStatus.bluetooth_connected && systemStatus.mega2560_connected;
  }
  
  DynamicJsonDocument response(JSON_BUFFER_SIZE);
  response["type"] = "test_response";
  response["testType"] = testType;
  response["result"] = result;
  response["wifi"] = systemStatus.wifi_connected;
  response["bluetooth"] = systemStatus.bluetooth_connected;
  response["mega2560"] = systemStatus.mega2560_connected;
  response["timestamp"] = millis();
  
  String jsonString;
  serializeJson(response, jsonString);
  
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
  doc["uptime"] = millis();
  doc["wifi_task_stack"] = uxTaskGetStackHighWaterMark(wifiTaskHandle);
  doc["bluetooth_task_stack"] = uxTaskGetStackHighWaterMark(bluetoothTaskHandle);
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial1.println(jsonString);
  Serial.println("发送性能信息: " + jsonString);
}

// 发送心跳
void sendHeartbeat() {
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  doc["type"] = "heartbeat";
  doc["timestamp"] = millis();
  doc["command_count"] = systemStatus.command_count;
  doc["wifi_connected"] = systemStatus.wifi_connected;
  doc["bluetooth_connected"] = systemStatus.bluetooth_connected;
  doc["mega2560_connected"] = systemStatus.mega2560_connected;
  doc["free_heap"] = systemStatus.free_heap;
  doc["cpu_temp"] = systemStatus.cpu_temp;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial1.println(jsonString);
}

// 发送错误响应
void sendErrorResponse(String error) {
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  doc["type"] = "error";
  doc["error"] = error;
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial1.println(jsonString);
  Serial.println("发送错误响应: " + jsonString);
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
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    doc["type"] = "test";
    doc["testType"] = "all";
    doc["timestamp"] = millis();
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    Serial1.println(jsonString);
    Serial.println("按钮触发测试: " + jsonString);
    
    // LED指示
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
  }
  
  buttonState = currentState;
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
  Serial.println("WiFi任务栈: " + String(uxTaskGetStackHighWaterMark(wifiTaskHandle)) + " bytes");
  Serial.println("蓝牙任务栈: " + String(uxTaskGetStackHighWaterMark(bluetoothTaskHandle)) + " bytes");
  Serial.println("================");
}
