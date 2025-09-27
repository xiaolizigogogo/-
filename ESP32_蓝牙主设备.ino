/*
 * ESP32 蓝牙主设备测试程序
 * 功能：作为蓝牙主设备，主动搜索并连接其他ESP32设备
 * 用途：测试ESP32之间的蓝牙通信
 */

#include <BluetoothSerial.h>
#include <ArduinoJson.h>

// 蓝牙串口对象
BluetoothSerial SerialBT;

// 目标设备名称
const char* TARGET_DEVICE_NAME = "ESP32_Slave";

// 系统状态
struct SystemStatus {
  bool bluetooth_connected;
  bool scanning;
  unsigned long last_scan;
  int scan_count;
  int command_count;
  int free_heap;
  float cpu_temp;
  String last_message;
};

// 全局变量
SystemStatus systemStatus = {false, false, 0, 0, 0, 0, 0.0, ""};

void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(1000);
  
  Serial.println();
  Serial.println("==========================================");
  Serial.println("ESP32 蓝牙主设备测试程序");
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
  
  if (SerialBT.begin("ESP32_Master", true)) { // true表示作为主设备
    Serial.println("✓ 蓝牙主设备初始化成功");
    Serial.println("设备名称: ESP32_Master");
    Serial.println("目标设备: ESP32_Slave");
    
    Serial.println();
    Serial.println("开始搜索目标设备...");
    Serial.println("==========================================");
    
    // 开始搜索
    startScanning();
  } else {
    Serial.println("✗ 蓝牙主设备初始化失败");
    Serial.println("请检查蓝牙硬件");
  }
}

void loop() {
  // 处理蓝牙数据
  handleBluetoothData();
  
  // 处理连接状态
  handleConnectionStatus();
  
  // 发送心跳
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 5000) {
    lastHeartbeat = millis();
    sendHeartbeat();
  }
  
  // 更新系统状态
  updateSystemStatus();
  
  delay(100);
}

// 开始搜索设备
void startScanning() {
  Serial.println("正在搜索设备: " + String(TARGET_DEVICE_NAME));
  systemStatus.scanning = true;
  systemStatus.last_scan = millis();
  
  // 尝试连接到目标设备
  bool connected = SerialBT.connect(TARGET_DEVICE_NAME);
  
  if (connected) {
    Serial.println("✓ 成功连接到: " + String(TARGET_DEVICE_NAME));
    systemStatus.bluetooth_connected = true;
    systemStatus.scanning = false;
  } else {
    Serial.println("✗ 连接失败，设备可能未启动或名称不匹配");
    systemStatus.scanning = false;
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
      
      // 处理不同类型的命令
      if (data == "status") {
        sendStatusResponse();
      } else if (data == "test") {
        sendTestResponse();
      } else if (data == "info") {
        sendSystemInfo();
      } else if (data == "ping") {
        sendPingResponse();
      } else if (data.startsWith("echo:")) {
        sendEchoResponse(data.substring(5));
      } else {
        sendUnknownCommandResponse(data);
      }
    }
  }
}

// 处理连接状态
void handleConnectionStatus() {
  static unsigned long lastConnectionCheck = 0;
  
  if (millis() - lastConnectionCheck > 3000) {
    lastConnectionCheck = millis();
    
    bool currentlyConnected = SerialBT.hasClient();
    
    if (currentlyConnected != systemStatus.bluetooth_connected) {
      systemStatus.bluetooth_connected = currentlyConnected;
      
      if (currentlyConnected) {
        Serial.println("✓ 蓝牙连接已建立");
      } else {
        Serial.println("✗ 蓝牙连接已断开");
        Serial.println("尝试重新连接...");
        startScanning();
      }
    }
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
  doc["device"] = "master";
  doc["bluetooth_connected"] = systemStatus.bluetooth_connected;
  doc["command_count"] = systemStatus.command_count;
  doc["free_heap"] = systemStatus.free_heap;
  doc["cpu_temp"] = systemStatus.cpu_temp;
  doc["uptime"] = millis();
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
  doc["device"] = "master";
  doc["result"] = "success";
  doc["message"] = "主设备通信正常";
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  SerialBT.println(jsonString);
  Serial.println("发送测试响应: " + jsonString);
}

// 发送系统信息
void sendSystemInfo() {
  DynamicJsonDocument doc(1024);
  doc["type"] = "system_info";
  doc["device"] = "master";
  doc["chip_model"] = ESP.getChipModel();
  doc["chip_revision"] = ESP.getChipRevision();
  doc["cpu_freq"] = getCpuFrequencyMhz();
  doc["free_heap"] = systemStatus.free_heap;
  doc["cpu_temp"] = systemStatus.cpu_temp;
  doc["uptime"] = millis();
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  SerialBT.println(jsonString);
  Serial.println("发送系统信息: " + jsonString);
}

// 发送Ping响应
void sendPingResponse() {
  DynamicJsonDocument doc(256);
  doc["type"] = "pong";
  doc["device"] = "master";
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  SerialBT.println(jsonString);
  Serial.println("发送Pong响应: " + jsonString);
}

// 发送回显响应
void sendEchoResponse(String message) {
  DynamicJsonDocument doc(512);
  doc["type"] = "echo";
  doc["device"] = "master";
  doc["message"] = message;
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  SerialBT.println(jsonString);
  Serial.println("发送回显响应: " + jsonString);
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
      Serial.println("  蓝牙连接: " + String(systemStatus.bluetooth_connected ? "已连接" : "未连接"));
      Serial.println("  命令计数: " + String(systemStatus.command_count));
      Serial.println("  可用内存: " + String(systemStatus.free_heap) + " bytes");
      Serial.println("  CPU温度: " + String(systemStatus.cpu_temp) + "°C");
      Serial.println();
    }
  }
}
