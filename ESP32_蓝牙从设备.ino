/*
 * ESP32 蓝牙从设备测试程序
 * 功能：作为蓝牙从设备，等待主设备连接
 * 用途：测试ESP32之间的蓝牙通信
 */

#include <BluetoothSerial.h>
#include <ArduinoJson.h>

// 蓝牙串口对象
BluetoothSerial SerialBT;

// 设备名称
const char* DEVICE_NAME = "ESP32_Slave";

// 系统状态
struct SystemStatus {
  bool bluetooth_connected;
  unsigned long last_heartbeat;
  int command_count;
  int free_heap;
  float cpu_temp;
  String last_message;
};

// 全局变量
SystemStatus systemStatus = {false, 0, 0, 0, 0.0, ""};

void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(1000);
  
  Serial.println();
  Serial.println("==========================================");
  Serial.println("ESP32 蓝牙从设备测试程序");
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
  
  if (SerialBT.begin(DEVICE_NAME)) { // 默认作为从设备
    Serial.println("✓ 蓝牙从设备初始化成功");
    Serial.println("设备名称: " + String(DEVICE_NAME));
    Serial.println("配对密码: 1234");
    
    // 设置配对密码
    SerialBT.setPin("1234", 4);
    
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
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  SerialBT.println(jsonString);
  Serial.println("发送测试响应: " + jsonString);
}

// 发送系统信息
void sendSystemInfo() {
  DynamicJsonDocument doc(1024);
  doc["type"] = "system_info";
  doc["device"] = "slave";
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
  doc["device"] = "slave";
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
  doc["device"] = "slave";
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
  doc["device"] = "slave";
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
  static unsigned long lastDiscoveryCheck = 0;
  
  if (millis() - lastUpdate > 1000) {
    lastUpdate = millis();
    
    // 更新内存状态
    systemStatus.free_heap = ESP.getFreeHeap();
    
    // 更新CPU温度
    systemStatus.cpu_temp = temperatureRead();
    
    // 更新蓝牙连接状态
    systemStatus.bluetooth_connected = SerialBT.hasClient();
    
    // 串口输出状态
    if (millis() % 10000 < 1000) { // 每10秒输出一次
      Serial.println("系统状态:");
      Serial.println("  设备类型: 从设备");
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
        Serial.println();
      }
    }
  }
  
  // 每5秒检查一次蓝牙发现状态
  if (millis() - lastDiscoveryCheck > 5000) {
    lastDiscoveryCheck = millis();
    
    if (!systemStatus.bluetooth_connected) {
      // 重新设置配对密码
      SerialBT.setPin("1234", 4);
    }
  }
}
