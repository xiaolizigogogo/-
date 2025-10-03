/*
 * ESP32 蓝牙优化版 - 主设备
 * 功能：改进的蓝牙主设备，具有更好的连接稳定性和错误处理
 * 特点：自动重连、连接状态监控、数据完整性检查
 */

#include <BluetoothSerial.h>
#include <ArduinoJson.h>

// 蓝牙串口对象
BluetoothSerial SerialBT;

// 目标设备名称
const char* TARGET_DEVICE_NAME = "ESP32_Slave";

// 连接配置
const int MAX_RETRY_COUNT = 5;
const int RETRY_DELAY = 3000;
const int HEARTBEAT_INTERVAL = 5000;
const int CONNECTION_CHECK_INTERVAL = 2000;

// 系统状态
struct SystemStatus {
  bool bluetooth_connected;
  bool scanning;
  unsigned long last_scan;
  unsigned long last_heartbeat;
  unsigned long last_connection_check;
  int retry_count;
  int command_count;
  int free_heap;
  float cpu_temp;
  String last_message;
  String connection_status;
};

// 全局变量
SystemStatus systemStatus = {false, false, 0, 0, 0, 0, 0, 0, 0.0, "", "未连接"};

void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(1000);
  
  Serial.println();
  Serial.println("==========================================");
  Serial.println("ESP32 蓝牙优化版 - 主设备");
  Serial.println("==========================================");
  
  // 获取系统信息
  systemStatus.free_heap = ESP.getFreeHeap();
  systemStatus.cpu_temp = temperatureRead();
  
  Serial.println("系统信息:");
  Serial.print("  可用内存: ");
  Serial.print(systemStatus.free_heap);
  Serial.println(" bytes");
  Serial.print("  CPU温度: ");
  Serial.print(systemStatus.cpu_temp);
  Serial.println("°C");
  Serial.println();
  
  // 初始化蓝牙
  Serial.println("初始化蓝牙主设备...");
  
  if (initBluetooth()) {
    Serial.println("✓ 蓝牙主设备初始化成功");
    Serial.println("设备名称: ESP32_Master");
    Serial.println("目标设备: " + String(TARGET_DEVICE_NAME));
    Serial.println();
    
    // 开始连接流程
    startConnectionProcess();
  } else {
    Serial.println("✗ 蓝牙主设备初始化失败");
    Serial.println("请检查蓝牙硬件");
  }
}

void loop() {
  // 处理蓝牙数据
  handleBluetoothData();
  
  // 检查连接状态
  checkConnectionStatus();
  
  // 发送心跳
  sendHeartbeatIfNeeded();
  
  // 更新系统状态
  updateSystemStatus();
  
  delay(100);
}

// 初始化蓝牙
bool initBluetooth() {
  // 尝试初始化蓝牙
  if (!SerialBT.begin("ESP32_Master", true)) {
    Serial.println("蓝牙初始化失败");
    return false;
  }
  
  // 设置配对密码
  SerialBT.setPin("1234", 4);
  
  // 启用安全简单配对
  SerialBT.enableSSP();
  
  Serial.println("蓝牙配置完成");
  return true;
}

// 开始连接流程
void startConnectionProcess() {
  Serial.println("开始连接流程...");
  systemStatus.scanning = true;
  systemStatus.last_scan = millis();
  systemStatus.connection_status = "正在连接";
  
  // 尝试连接到目标设备
  bool connected = SerialBT.connect(TARGET_DEVICE_NAME);
  
  if (connected) {
    Serial.println("✓ 成功连接到: " + String(TARGET_DEVICE_NAME));
    systemStatus.bluetooth_connected = true;
    systemStatus.scanning = false;
    systemStatus.retry_count = 0;
    systemStatus.connection_status = "已连接";
    
    // 发送连接确认
    sendConnectionConfirm();
  } else {
    Serial.println("✗ 连接失败，设备可能未启动或名称不匹配");
    systemStatus.scanning = false;
    systemStatus.connection_status = "连接失败";
    
    // 开始重试机制
    startRetryProcess();
  }
}

// 开始重试流程
void startRetryProcess() {
  if (systemStatus.retry_count < MAX_RETRY_COUNT) {
    systemStatus.retry_count++;
    Serial.println("重试连接 (" + String(systemStatus.retry_count) + "/" + String(MAX_RETRY_COUNT) + ")");
    systemStatus.connection_status = "重试中";
    
    // 延迟后重试
    delay(RETRY_DELAY);
    startConnectionProcess();
  } else {
    Serial.println("达到最大重试次数，停止连接尝试");
    systemStatus.connection_status = "连接失败";
    systemStatus.retry_count = 0;
  }
}

// 处理蓝牙接收的数据
void handleBluetoothData() {
  if (SerialBT.available()) {
    String data = SerialBT.readStringUntil('\n');
    data.trim();
    
    if (data.length() > 0) {
      Serial.println("收到数据: " + data);
      systemStatus.last_message = data;
      systemStatus.command_count++;
      
      // 解析JSON数据
      if (data.startsWith("{")) {
        processJsonData(data);
      } else {
        // 处理简单命令
        processSimpleCommand(data);
      }
    }
  }
}

// 处理JSON数据
void processJsonData(String jsonData) {
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, jsonData);
  
  if (error) {
    Serial.println("JSON解析错误: " + String(error.c_str()));
    return;
  }
  
  String type = doc["type"].as<String>();
  String device = doc["device"].as<String>();
  
  Serial.println("收到JSON数据 - 类型: " + type + ", 设备: " + device);
  
  // 处理不同类型的JSON消息
  if (type == "heartbeat") {
    handleHeartbeatResponse(doc);
  } else if (type == "status") {
    handleStatusResponse(doc);
  } else if (type == "test_result") {
    handleTestResponse(doc);
  } else if (type == "system_info") {
    handleSystemInfoResponse(doc);
  } else if (type == "pong") {
    handlePongResponse(doc);
  } else if (type == "echo") {
    handleEchoResponse(doc);
  } else if (type == "error") {
    handleErrorResponse(doc);
  } else {
    Serial.println("未知的JSON消息类型: " + type);
  }
}

// 处理简单命令
void processSimpleCommand(String command) {
  if (command == "status") {
    sendStatusRequest();
  } else if (command == "test") {
    sendTestRequest();
  } else if (command == "info") {
    sendSystemInfoRequest();
  } else if (command == "ping") {
    sendPingRequest();
  } else if (command.startsWith("echo:")) {
    sendEchoRequest(command.substring(5));
  } else {
    sendUnknownCommandResponse(command);
  }
}

// 检查连接状态
void checkConnectionStatus() {
  if (millis() - systemStatus.last_connection_check > CONNECTION_CHECK_INTERVAL) {
    systemStatus.last_connection_check = millis();
    
    bool currentlyConnected = SerialBT.hasClient();
    
    if (currentlyConnected != systemStatus.bluetooth_connected) {
      systemStatus.bluetooth_connected = currentlyConnected;
      
      if (currentlyConnected) {
        Serial.println("✓ 蓝牙连接已建立");
        systemStatus.connection_status = "已连接";
        systemStatus.retry_count = 0;
      } else {
        Serial.println("✗ 蓝牙连接已断开");
        systemStatus.connection_status = "已断开";
        
        // 开始重连
        Serial.println("尝试重新连接...");
        startRetryProcess();
      }
    }
  }
}

// 发送心跳
void sendHeartbeatIfNeeded() {
  if (millis() - systemStatus.last_heartbeat > HEARTBEAT_INTERVAL) {
    systemStatus.last_heartbeat = millis();
    sendHeartbeat();
  }
}

// 发送心跳
void sendHeartbeat() {
  if (systemStatus.bluetooth_connected) {
    DynamicJsonDocument doc(512);
    doc["type"] = "heartbeat";
    doc["device"] = "master";
    doc["timestamp"] = millis();
    doc["command_count"] = systemStatus.command_count;
    doc["free_heap"] = systemStatus.free_heap;
    doc["cpu_temp"] = systemStatus.cpu_temp;
    doc["connection_status"] = systemStatus.connection_status;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    SerialBT.println(jsonString);
    Serial.println("发送心跳: " + jsonString);
  }
}

// 发送连接确认
void sendConnectionConfirm() {
  DynamicJsonDocument doc(256);
  doc["type"] = "connection_confirm";
  doc["device"] = "master";
  doc["timestamp"] = millis();
  doc["message"] = "连接已建立";
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  SerialBT.println(jsonString);
  Serial.println("发送连接确认: " + jsonString);
}

// 发送状态请求
void sendStatusRequest() {
  SerialBT.println("status");
  Serial.println("发送状态请求");
}

// 发送测试请求
void sendTestRequest() {
  SerialBT.println("test");
  Serial.println("发送测试请求");
}

// 发送系统信息请求
void sendSystemInfoRequest() {
  SerialBT.println("info");
  Serial.println("发送系统信息请求");
}

// 发送Ping请求
void sendPingRequest() {
  SerialBT.println("ping");
  Serial.println("发送Ping请求");
}

// 发送回显请求
void sendEchoRequest(String message) {
  SerialBT.println("echo:" + message);
  Serial.println("发送回显请求: " + message);
}

// 发送未知命令响应
void sendUnknownCommandResponse(String command) {
  DynamicJsonDocument doc(512);
  doc["type"] = "error";
  doc["device"] = "master";
  doc["error"] = "未知命令: " + command;
  doc["available_commands"] = "status, test, info, ping, echo:message";
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  SerialBT.println(jsonString);
  Serial.println("发送错误响应: " + jsonString);
}

// 处理心跳响应
void handleHeartbeatResponse(DynamicJsonDocument& doc) {
  Serial.println("收到从设备心跳");
}

// 处理状态响应
void handleStatusResponse(DynamicJsonDocument& doc) {
  Serial.println("收到从设备状态响应");
  Serial.println("  连接状态: " + doc["bluetooth_connected"].as<String>());
  Serial.println("  命令计数: " + doc["command_count"].as<String>());
  Serial.println("  可用内存: " + doc["free_heap"].as<String>());
}

// 处理测试响应
void handleTestResponse(DynamicJsonDocument& doc) {
  Serial.println("收到测试响应: " + doc["message"].as<String>());
}

// 处理系统信息响应
void handleSystemInfoResponse(DynamicJsonDocument& doc) {
  Serial.println("收到系统信息响应");
  Serial.println("  芯片型号: " + doc["chip_model"].as<String>());
  Serial.println("  CPU频率: " + doc["cpu_freq"].as<String>() + " MHz");
}

// 处理Pong响应
void handlePongResponse(DynamicJsonDocument& doc) {
  Serial.println("收到Pong响应");
}

// 处理回显响应
void handleEchoResponse(DynamicJsonDocument& doc) {
  Serial.println("收到回显响应: " + doc["message"].as<String>());
}

// 处理错误响应
void handleErrorResponse(DynamicJsonDocument& doc) {
  Serial.println("收到错误响应: " + doc["error"].as<String>());
}

// 更新系统状态
void updateSystemStatus() {
  static unsigned long lastUpdate = 0;
  
  if (millis() - lastUpdate > 1000) {
    lastUpdate = millis();
    
    // 更新内存状态
    systemStatus.free_heap = ESP.getFreeHeap();
    
    // 更新CPU温度
    systemStatus.cpu_temp = temperatureRead();
    
    // 串口输出状态
    if (millis() % 10000 < 1000) { // 每10秒输出一次
      Serial.println("系统状态:");
      Serial.println("  设备类型: 主设备");
      Serial.println("  连接状态: " + systemStatus.connection_status);
      Serial.println("  蓝牙连接: " + String(systemStatus.bluetooth_connected ? "已连接" : "未连接"));
      Serial.println("  重试次数: " + String(systemStatus.retry_count));
      Serial.println("  命令计数: " + String(systemStatus.command_count));
      Serial.println("  可用内存: " + String(systemStatus.free_heap) + " bytes");
      Serial.println("  CPU温度: " + String(systemStatus.cpu_temp) + "°C");
      Serial.println();
    }
  }
}
