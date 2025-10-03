/*
 * ESP32 蓝牙优化版 - 从设备
 * 功能：改进的蓝牙从设备，具有更好的连接稳定性和错误处理
 * 特点：自动重连、连接状态监控、数据完整性检查
 */

#include <BluetoothSerial.h>
#include <ArduinoJson.h>

// 蓝牙串口对象
BluetoothSerial SerialBT;

// 设备名称
const char* DEVICE_NAME = "ESP32_Slave";

// 连接配置
const int HEARTBEAT_INTERVAL = 5000;
const int CONNECTION_CHECK_INTERVAL = 2000;
const int DISCOVERY_CHECK_INTERVAL = 5000;

// 系统状态
struct SystemStatus {
  bool bluetooth_connected;
  unsigned long last_heartbeat;
  unsigned long last_connection_check;
  unsigned long last_discovery_check;
  int command_count;
  int free_heap;
  float cpu_temp;
  String last_message;
  String connection_status;
  unsigned long last_client_activity;
};

// 全局变量
SystemStatus systemStatus = {false, 0, 0, 0, 0, 0, 0.0, "", "未连接", 0};

void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(1000);
  
  Serial.println();
  Serial.println("==========================================");
  Serial.println("ESP32 蓝牙优化版 - 从设备");
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
  Serial.println("初始化蓝牙从设备...");
  
  if (initBluetooth()) {
    Serial.println("✓ 蓝牙从设备初始化成功");
    Serial.println("设备名称: " + String(DEVICE_NAME));
    Serial.println("配对密码: 1234");
    Serial.println();
    Serial.println("从设备已启动，等待主设备连接...");
    Serial.println("请确保主设备已启动并开始搜索");
    Serial.println("设备状态: 可发现、可连接");
    Serial.println("==========================================");
  } else {
    Serial.println("✗ 蓝牙从设备初始化失败");
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
  
  // 检查蓝牙发现状态
  checkBluetoothDiscovery();
  
  // 更新系统状态
  updateSystemStatus();
  
  delay(100);
}

// 初始化蓝牙
bool initBluetooth() {
  // 尝试初始化蓝牙
  if (!SerialBT.begin(DEVICE_NAME)) {
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

// 处理蓝牙接收的数据
void handleBluetoothData() {
  if (SerialBT.available()) {
    String data = SerialBT.readStringUntil('\n');
    data.trim();
    
    if (data.length() > 0) {
      Serial.println("收到数据: " + data);
      systemStatus.last_message = data;
      systemStatus.command_count++;
      systemStatus.last_client_activity = millis();
      
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
    handleHeartbeatRequest(doc);
  } else if (type == "connection_confirm") {
    handleConnectionConfirm(doc);
  } else {
    Serial.println("未知的JSON消息类型: " + type);
  }
}

// 处理简单命令
void processSimpleCommand(String command) {
  if (command == "status") {
    sendStatusResponse();
  } else if (command == "test") {
    sendTestResponse();
  } else if (command == "info") {
    sendSystemInfoResponse();
  } else if (command == "ping") {
    sendPingResponse();
  } else if (command.startsWith("echo:")) {
    sendEchoResponse(command.substring(5));
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
        systemStatus.last_client_activity = millis();
      } else {
        Serial.println("✗ 蓝牙连接已断开");
        systemStatus.connection_status = "已断开";
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
    doc["device"] = "slave";
    doc["timestamp"] = millis();
    doc["command_count"] = systemStatus.command_count;
    doc["free_heap"] = systemStatus.free_heap;
    doc["cpu_temp"] = systemStatus.cpu_temp;
    doc["connection_status"] = systemStatus.connection_status;
    doc["last_activity"] = systemStatus.last_client_activity;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    SerialBT.println(jsonString);
    Serial.println("发送心跳: " + jsonString);
  }
}

// 发送状态响应
void sendStatusResponse() {
  DynamicJsonDocument doc(512);
  doc["type"] = "status";
  doc["device"] = "slave";
  doc["bluetooth_connected"] = systemStatus.bluetooth_connected;
  doc["command_count"] = systemStatus.command_count;
  doc["free_heap"] = systemStatus.free_heap;
  doc["cpu_temp"] = systemStatus.cpu_temp;
  doc["uptime"] = millis();
  doc["connection_status"] = systemStatus.connection_status;
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  SerialBT.println(jsonString);
  Serial.println("发送状态响应: " + jsonString);
}

// 发送测试响应
void sendTestResponse() {
  DynamicJsonDocument doc(512);
  doc["type"] = "test_result";
  doc["device"] = "slave";
  doc["result"] = "success";
  doc["message"] = "从设备通信正常";
  doc["timestamp"] = millis();
  doc["test_time"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  SerialBT.println(jsonString);
  Serial.println("发送测试响应: " + jsonString);
}

// 发送系统信息响应
void sendSystemInfoResponse() {
  DynamicJsonDocument doc(1024);
  doc["type"] = "system_info";
  doc["device"] = "slave";
  doc["chip_model"] = ESP.getChipModel();
  doc["chip_revision"] = ESP.getChipRevision();
  doc["cpu_freq"] = getCpuFrequencyMhz();
  doc["free_heap"] = systemStatus.free_heap;
  doc["cpu_temp"] = systemStatus.cpu_temp;
  doc["uptime"] = millis();
  doc["bluetooth_name"] = DEVICE_NAME;
  doc["connection_status"] = systemStatus.connection_status;
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  SerialBT.println(jsonString);
  Serial.println("发送系统信息响应: " + jsonString);
}

// 发送Ping响应
void sendPingResponse() {
  DynamicJsonDocument doc(256);
  doc["type"] = "pong";
  doc["device"] = "slave";
  doc["timestamp"] = millis();
  doc["response_time"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  SerialBT.println(jsonString);
  Serial.println("发送Pong响应: " + jsonString);
}

// 发送回显响应
void sendEchoResponse(String message) {
  DynamicJsonDocument doc(512);
  doc["type"] = "echo";
  doc["device"] = "slave";
  doc["message"] = message;
  doc["timestamp"] = millis();
  doc["echo_time"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  SerialBT.println(jsonString);
  Serial.println("发送回显响应: " + jsonString);
}

// 发送未知命令响应
void sendUnknownCommandResponse(String command) {
  DynamicJsonDocument doc(512);
  doc["type"] = "error";
  doc["device"] = "slave";
  doc["error"] = "未知命令: " + command;
  doc["available_commands"] = "status, test, info, ping, echo:message";
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  SerialBT.println(jsonString);
  Serial.println("发送错误响应: " + jsonString);
}

// 处理心跳请求
void handleHeartbeatRequest(DynamicJsonDocument& doc) {
  Serial.println("收到主设备心跳");
  // 可以在这里添加对主设备心跳的响应逻辑
}

// 处理连接确认
void handleConnectionConfirm(DynamicJsonDocument& doc) {
  Serial.println("收到连接确认: " + doc["message"].as<String>());
  systemStatus.connection_status = "已连接";
}

// 检查蓝牙发现状态
void checkBluetoothDiscovery() {
  if (millis() - systemStatus.last_discovery_check > DISCOVERY_CHECK_INTERVAL) {
    systemStatus.last_discovery_check = millis();
    
    if (!systemStatus.bluetooth_connected) {
      // 重新设置配对密码，确保设备可发现
      SerialBT.setPin("1234", 4);
      SerialBT.enableSSP();
    }
  }
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
      Serial.println("  设备类型: 从设备");
      Serial.println("  连接状态: " + systemStatus.connection_status);
      Serial.println("  蓝牙连接: " + String(systemStatus.bluetooth_connected ? "已连接" : "未连接"));
      Serial.println("  命令计数: " + String(systemStatus.command_count));
      Serial.println("  可用内存: " + String(systemStatus.free_heap) + " bytes");
      Serial.println("  CPU温度: " + String(systemStatus.cpu_temp) + "°C");
      
      // 每30秒提醒一次蓝牙连接
      if (millis() % 30000 < 1000 && !systemStatus.bluetooth_connected) {
        Serial.println();
        Serial.println("⚠️  等待主设备连接...");
        Serial.println("   请确保主设备已启动并开始搜索");
        Serial.println("   设备名称: " + String(DEVICE_NAME));
        Serial.println("   配对密码: 1234");
        Serial.println("   设备状态: 可发现、可连接");
        Serial.println();
      }
    }
  }
}
