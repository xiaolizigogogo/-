/*
 * ESP32-S 蓝牙测试程序
 * 专门针对ESP32-S型号优化
 * 功能：简单的蓝牙串口通信测试
 */

#include <BluetoothSerial.h>
#include <ArduinoJson.h>

// 蓝牙串口对象
BluetoothSerial SerialBT;

// 蓝牙设备名称
const char* BT_DEVICE_NAME = "ESP32-S_Test";

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
  delay(2000); // 增加延迟确保系统稳定
  
  Serial.println();
  Serial.println("==========================================");
  Serial.println("ESP32-S 蓝牙测试程序");
  Serial.println("==========================================");
  
  // 获取系统信息
  systemStatus.free_heap = ESP.getFreeHeap();
  systemStatus.cpu_temp = temperatureRead();
  
  Serial.println("系统信息:");
  Serial.print("  芯片型号: ");
  Serial.println(ESP.getChipModel());
  Serial.print("  芯片版本: ");
  Serial.println(ESP.getChipRevision());
  Serial.print("  CPU频率: ");
  Serial.print(getCpuFrequencyMhz());
  Serial.println(" MHz");
  Serial.print("  可用内存: ");
  Serial.print(systemStatus.free_heap);
  Serial.println(" bytes");
  Serial.print("  CPU温度: ");
  Serial.print(systemStatus.cpu_temp);
  Serial.println("°C");
  Serial.println();
  
  // 初始化蓝牙
  Serial.println("初始化蓝牙...");
  
  // 尝试初始化蓝牙
  bool btResult = SerialBT.begin(BT_DEVICE_NAME);
  
  if (btResult) {
    Serial.println("✓ 蓝牙初始化成功");
    Serial.print("设备名称: ");
    Serial.println(BT_DEVICE_NAME);
    Serial.println("配对密码: 1234");
    
    // 设置配对密码
    SerialBT.setPin("1234", 4);
    
    Serial.println();
    Serial.println("蓝牙设备已启动，正在等待连接...");
    Serial.println("请确保：");
    Serial.println("1. 手机蓝牙已开启");
    Serial.println("2. 搜索设备：ESP32-S_Test");
    Serial.println("3. 配对密码：1234");
    Serial.println("4. 如果搜索不到，请重启手机蓝牙");
    Serial.println("==========================================");
  } else {
    Serial.println("✗ 蓝牙初始化失败");
    Serial.println("可能原因：");
    Serial.println("1. 蓝牙硬件故障");
    Serial.println("2. 蓝牙被其他程序占用");
    Serial.println("3. ESP32-S配置问题");
    Serial.println("4. 电源供应不稳定");
  }
}

void loop() {
  // 处理蓝牙数据
  handleBluetoothData();
  
  // 发送心跳
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 10000) { // 10秒心跳
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
      Serial.println("收到蓝牙数据: " + data);
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
      } else if (data == "restart") {
        sendRestartResponse();
      } else {
        sendUnknownCommandResponse(data);
      }
    }
  }
}

// 发送心跳
void sendHeartbeat() {
  if (SerialBT.hasClient()) {
    DynamicJsonDocument doc(512);
    doc["type"] = "heartbeat";
    doc["device"] = "ESP32-S";
    doc["timestamp"] = millis();
    doc["command_count"] = systemStatus.command_count;
    doc["bluetooth_connected"] = SerialBT.hasClient();
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
  doc["device"] = "ESP32-S";
  doc["bluetooth_connected"] = SerialBT.hasClient();
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
  doc["device"] = "ESP32-S";
  doc["result"] = "success";
  doc["message"] = "ESP32-S蓝牙通信正常";
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
  doc["device"] = "ESP32-S";
  doc["chip_model"] = ESP.getChipModel();
  doc["chip_revision"] = ESP.getChipRevision();
  doc["cpu_freq"] = getCpuFrequencyMhz();
  doc["free_heap"] = systemStatus.free_heap;
  doc["cpu_temp"] = systemStatus.cpu_temp;
  doc["uptime"] = millis();
  doc["bluetooth_name"] = BT_DEVICE_NAME;
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
  doc["device"] = "ESP32-S";
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
  doc["device"] = "ESP32-S";
  doc["message"] = message;
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  SerialBT.println(jsonString);
  Serial.println("发送回显响应: " + jsonString);
}

// 发送重启响应
void sendRestartResponse() {
  DynamicJsonDocument doc(256);
  doc["type"] = "restart";
  doc["device"] = "ESP32-S";
  doc["message"] = "设备即将重启";
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  SerialBT.println(jsonString);
  Serial.println("发送重启响应: " + jsonString);
  
  delay(1000);
  ESP.restart();
}

// 发送未知命令响应
void sendUnknownCommandResponse(String command) {
  DynamicJsonDocument doc(512);
  doc["type"] = "error";
  doc["device"] = "ESP32-S";
  doc["error"] = "未知命令: " + command;
  doc["available_commands"] = "status, test, info, ping, echo:message, restart";
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
    
    // 更新蓝牙连接状态
    systemStatus.bluetooth_connected = SerialBT.hasClient();
    
    // 串口输出状态
    if (millis() % 30000 < 1000) { // 每30秒输出一次
      Serial.println("系统状态:");
      Serial.println("  设备型号: ESP32-S");
      Serial.println("  蓝牙连接: " + String(systemStatus.bluetooth_connected ? "已连接" : "未连接"));
      Serial.println("  命令计数: " + String(systemStatus.command_count));
      Serial.println("  可用内存: " + String(systemStatus.free_heap) + " bytes");
      Serial.println("  CPU温度: " + String(systemStatus.cpu_temp) + "°C");
      
      // 每60秒提醒一次蓝牙连接
      if (millis() % 60000 < 1000 && !systemStatus.bluetooth_connected) {
        Serial.println();
        Serial.println("⚠️  蓝牙未连接，请检查：");
        Serial.println("   1. 手机蓝牙已开启");
        Serial.println("   2. 搜索设备：ESP32-S_Test");
        Serial.println("   3. 配对密码：1234");
        Serial.println("   4. 清除之前的配对记录");
        Serial.println("   5. 重启手机蓝牙");
        Serial.println("   6. 检查设备距离（1米内）");
        Serial.println();
      }
    }
  }
}
