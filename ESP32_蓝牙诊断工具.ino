/*
 * ESP32 蓝牙诊断工具
 * 功能：诊断蓝牙连接问题，提供详细的调试信息
 * 特点：全面的状态检查、错误诊断、连接测试
 */

#include <BluetoothSerial.h>

// 蓝牙串口对象
BluetoothSerial SerialBT;

// 设备配置
const char* BT_DEVICE_NAME = "ESP32_Diagnostic";
const char* BT_PIN = "1234";

// 诊断状态
struct DiagnosticStatus {
  bool bluetoothInitialized;
  bool bluetoothConnected;
  bool hasClient;
  int connectionAttempts;
  int successfulConnections;
  int failedConnections;
  unsigned long lastConnectionTime;
  unsigned long lastDisconnectionTime;
  String lastError;
  int freeHeap;
  float cpuTemp;
};

DiagnosticStatus diagStatus = {false, false, false, 0, 0, 0, 0, 0, "", 0, 0.0};

void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(1000);
  
  Serial.println();
  Serial.println("==========================================");
  Serial.println("ESP32 蓝牙诊断工具");
  Serial.println("==========================================");
  
  // 显示系统信息
  displaySystemInfo();
  
  // 开始蓝牙诊断
  startBluetoothDiagnostic();
}

void loop() {
  // 更新诊断状态
  updateDiagnosticStatus();
  
  // 处理蓝牙数据
  handleBluetoothData();
  
  // 定期显示诊断报告
  displayDiagnosticReport();
  
  delay(1000);
}

// 显示系统信息
void displaySystemInfo() {
  Serial.println("系统信息:");
  Serial.print("  芯片型号: ");
  Serial.println(ESP.getChipModel());
  Serial.print("  芯片版本: ");
  Serial.println(ESP.getChipRevision());
  Serial.print("  CPU频率: ");
  Serial.print(getCpuFrequencyMhz());
  Serial.println(" MHz");
  Serial.print("  可用内存: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
  Serial.print("  CPU温度: ");
  Serial.print(temperatureRead());
  Serial.println("°C");
  Serial.println();
}

// 开始蓝牙诊断
void startBluetoothDiagnostic() {
  Serial.println("开始蓝牙诊断...");
  Serial.println();
  
  // 测试1: 蓝牙初始化
  Serial.println("测试1: 蓝牙初始化");
  testBluetoothInitialization();
  
  // 测试2: 设备名称设置
  Serial.println("测试2: 设备名称设置");
  testDeviceNameSetting();
  
  // 测试3: 配对密码设置
  Serial.println("测试3: 配对密码设置");
  testPinSetting();
  
  // 测试4: 蓝牙服务启动
  Serial.println("测试4: 蓝牙服务启动");
  testBluetoothService();
  
  Serial.println("诊断完成，开始监控...");
  Serial.println("==========================================");
}

// 测试蓝牙初始化
void testBluetoothInitialization() {
  Serial.print("  尝试初始化蓝牙... ");
  
  if (SerialBT.begin(BT_DEVICE_NAME)) {
    Serial.println("✓ 成功");
    diagStatus.bluetoothInitialized = true;
  } else {
    Serial.println("✗ 失败");
    diagStatus.lastError = "蓝牙初始化失败";
    Serial.println("  错误: 蓝牙硬件可能有问题");
  }
}

// 测试设备名称设置
void testDeviceNameSetting() {
  if (!diagStatus.bluetoothInitialized) {
    Serial.println("  跳过 - 蓝牙未初始化");
    return;
  }
  
  Serial.print("  设置设备名称: " + String(BT_DEVICE_NAME) + "... ");
  Serial.println("✓ 完成");
}

// 测试配对密码设置
void testPinSetting() {
  if (!diagStatus.bluetoothInitialized) {
    Serial.println("  跳过 - 蓝牙未初始化");
    return;
  }
  
  Serial.print("  设置配对密码: " + String(BT_PIN) + "... ");
  
  if (SerialBT.setPin(BT_PIN, 4)) {
    Serial.println("✓ 成功");
  } else {
    Serial.println("✗ 失败");
    diagStatus.lastError = "配对密码设置失败";
  }
}

// 测试蓝牙服务
void testBluetoothService() {
  if (!diagStatus.bluetoothInitialized) {
    Serial.println("  跳过 - 蓝牙未初始化");
    return;
  }
  
  Serial.print("  检查蓝牙服务状态... ");
  
  if (SerialBT.hasClient()) {
    Serial.println("✓ 有客户端连接");
    diagStatus.bluetoothConnected = true;
  } else {
    Serial.println("○ 无客户端连接（正常）");
  }
}

// 更新诊断状态
void updateDiagnosticStatus() {
  // 更新内存和温度
  diagStatus.freeHeap = ESP.getFreeHeap();
  diagStatus.cpuTemp = temperatureRead();
  
  // 检查连接状态
  bool currentlyConnected = SerialBT.hasClient();
  
  if (currentlyConnected != diagStatus.bluetoothConnected) {
    diagStatus.bluetoothConnected = currentlyConnected;
    
    if (currentlyConnected) {
      diagStatus.successfulConnections++;
      diagStatus.lastConnectionTime = millis();
      Serial.println("✓ 检测到蓝牙连接");
    } else {
      diagStatus.lastDisconnectionTime = millis();
      Serial.println("✗ 检测到蓝牙断开");
    }
  }
  
  diagStatus.hasClient = SerialBT.hasClient();
}

// 处理蓝牙数据
void handleBluetoothData() {
  if (SerialBT.available()) {
    String data = SerialBT.readStringUntil('\n');
    data.trim();
    
    if (data.length() > 0) {
      Serial.println("收到数据: " + data);
      
      // 处理诊断命令
      if (data == "diagnostic") {
        sendDiagnosticResponse();
      } else if (data == "status") {
        sendStatusResponse();
      } else if (data == "test") {
        sendTestResponse();
      } else {
        sendHelpResponse();
      }
    }
  }
}

// 发送诊断响应
void sendDiagnosticResponse() {
  String response = "diagnostic_response:";
  response += "init:" + String(diagStatus.bluetoothInitialized ? "ok" : "fail") + ":";
  response += "connected:" + String(diagStatus.bluetoothConnected ? "yes" : "no") + ":";
  response += "attempts:" + String(diagStatus.connectionAttempts) + ":";
  response += "success:" + String(diagStatus.successfulConnections) + ":";
  response += "failed:" + String(diagStatus.failedConnections) + ":";
  response += "memory:" + String(diagStatus.freeHeap) + ":";
  response += "temp:" + String(diagStatus.cpuTemp) + ":";
  response += "time:" + String(millis());
  
  SerialBT.println(response);
  Serial.println("发送诊断响应: " + response);
}

// 发送状态响应
void sendStatusResponse() {
  String response = "status_response:";
  response += "bluetooth:" + String(diagStatus.bluetoothConnected ? "connected" : "disconnected") + ":";
  response += "uptime:" + String(millis() / 1000) + ":";
  response += "memory:" + String(diagStatus.freeHeap) + ":";
  response += "temp:" + String(diagStatus.cpuTemp);
  
  SerialBT.println(response);
  Serial.println("发送状态响应: " + response);
}

// 发送测试响应
void sendTestResponse() {
  String response = "test_response:success:诊断工具运行正常:" + String(millis());
  SerialBT.println(response);
  Serial.println("发送测试响应: " + response);
}

// 发送帮助响应
void sendHelpResponse() {
  String response = "help:支持的命令:diagnostic,status,test";
  SerialBT.println(response);
  Serial.println("发送帮助响应: " + response);
}

// 显示诊断报告
void displayDiagnosticReport() {
  static unsigned long lastReport = 0;
  
  if (millis() - lastReport > 30000) { // 每30秒显示一次
    lastReport = millis();
    
    Serial.println();
    Serial.println("==========================================");
    Serial.println("蓝牙诊断报告");
    Serial.println("==========================================");
    
    Serial.println("蓝牙状态:");
    Serial.println("  初始化: " + String(diagStatus.bluetoothInitialized ? "成功" : "失败"));
    Serial.println("  连接状态: " + String(diagStatus.bluetoothConnected ? "已连接" : "未连接"));
    Serial.println("  有客户端: " + String(diagStatus.hasClient ? "是" : "否"));
    
    Serial.println("连接统计:");
    Serial.println("  连接尝试: " + String(diagStatus.connectionAttempts));
    Serial.println("  成功连接: " + String(diagStatus.successfulConnections));
    Serial.println("  连接失败: " + String(diagStatus.failedConnections));
    
    if (diagStatus.lastConnectionTime > 0) {
      Serial.println("  最后连接: " + String((millis() - diagStatus.lastConnectionTime) / 1000) + " 秒前");
    }
    
    if (diagStatus.lastDisconnectionTime > 0) {
      Serial.println("  最后断开: " + String((millis() - diagStatus.lastDisconnectionTime) / 1000) + " 秒前");
    }
    
    Serial.println("系统状态:");
    Serial.println("  可用内存: " + String(diagStatus.freeHeap) + " bytes");
    Serial.println("  CPU温度: " + String(diagStatus.cpuTemp) + "°C");
    Serial.println("  运行时间: " + String(millis() / 1000) + " 秒");
    
    if (diagStatus.lastError.length() > 0) {
      Serial.println("最后错误: " + diagStatus.lastError);
    }
    
    Serial.println("==========================================");
    Serial.println();
  }
}
