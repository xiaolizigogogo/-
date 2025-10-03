/*
 * ESP32 蓝牙快速测试版
 * 功能：简化的蓝牙测试程序，专注于快速连接和基本通信
 * 特点：最小化代码、快速启动、易于调试
 */

#include <BluetoothSerial.h>

// 蓝牙串口对象
BluetoothSerial SerialBT;

// 设备配置
const char* BT_DEVICE_NAME = "ESP32_QuickTest";
const char* BT_PIN = "1234";

// 状态变量
bool bluetoothConnected = false;
unsigned long lastHeartbeat = 0;
int commandCount = 0;

void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(1000);
  
  Serial.println();
  Serial.println("==========================================");
  Serial.println("ESP32 蓝牙快速测试版");
  Serial.println("==========================================");
  
  // 显示系统信息
  Serial.println("系统信息:");
  Serial.print("  芯片型号: ");
  Serial.println(ESP.getChipModel());
  Serial.print("  可用内存: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
  Serial.print("  CPU频率: ");
  Serial.print(getCpuFrequencyMhz());
  Serial.println(" MHz");
  Serial.println();
  
  // 初始化蓝牙
  Serial.println("初始化蓝牙...");
  
  if (SerialBT.begin(BT_DEVICE_NAME)) {
    Serial.println("✓ 蓝牙初始化成功");
    Serial.println("设备名称: " + String(BT_DEVICE_NAME));
    Serial.println("配对密码: " + String(BT_PIN));
    
    // 设置配对密码
    SerialBT.setPin(BT_PIN, 4);
    
    Serial.println();
    Serial.println("蓝牙设备已启动，正在等待连接...");
    Serial.println("连接步骤:");
    Serial.println("1. 打开手机蓝牙设置");
    Serial.println("2. 搜索设备: " + String(BT_DEVICE_NAME));
    Serial.println("3. 配对密码: " + String(BT_PIN));
    Serial.println("4. 连接成功后开始测试");
    Serial.println("==========================================");
  } else {
    Serial.println("✗ 蓝牙初始化失败");
    Serial.println("请检查蓝牙硬件");
  }
}

void loop() {
  // 检查蓝牙连接状态
  checkBluetoothConnection();
  
  // 处理接收的数据
  handleBluetoothData();
  
  // 发送心跳
  sendHeartbeat();
  
  // 显示状态信息
  showStatus();
  
  delay(100);
}

// 检查蓝牙连接状态
void checkBluetoothConnection() {
  bool currentlyConnected = SerialBT.hasClient();
  
  if (currentlyConnected != bluetoothConnected) {
    bluetoothConnected = currentlyConnected;
    
    if (bluetoothConnected) {
      Serial.println("✓ 蓝牙连接已建立");
      Serial.println("可以开始发送测试命令");
      Serial.println("支持的命令: test, status, ping, echo:消息");
    } else {
      Serial.println("✗ 蓝牙连接已断开");
    }
  }
}

// 处理蓝牙接收的数据
void handleBluetoothData() {
  if (SerialBT.available()) {
    String data = SerialBT.readStringUntil('\n');
    data.trim();
    
    if (data.length() > 0) {
      Serial.println("收到数据: " + data);
      commandCount++;
      
      // 处理命令
      if (data == "test") {
        sendTestResponse();
      } else if (data == "status") {
        sendStatusResponse();
      } else if (data == "ping") {
        sendPingResponse();
      } else if (data.startsWith("echo:")) {
        sendEchoResponse(data.substring(5));
      } else {
        sendHelpResponse();
      }
    }
  }
}

// 发送心跳
void sendHeartbeat() {
  if (millis() - lastHeartbeat > 5000) { // 每5秒发送一次
    lastHeartbeat = millis();
    
    if (bluetoothConnected) {
      String heartbeat = "heartbeat:" + String(millis()) + ":" + String(commandCount);
      SerialBT.println(heartbeat);
      Serial.println("发送心跳: " + heartbeat);
    }
  }
}

// 发送测试响应
void sendTestResponse() {
  String response = "test_response:success:通信正常:" + String(millis());
  SerialBT.println(response);
  Serial.println("发送测试响应: " + response);
}

// 发送状态响应
void sendStatusResponse() {
  String response = "status_response:connected:" + String(bluetoothConnected) + 
                   ":commands:" + String(commandCount) + 
                   ":memory:" + String(ESP.getFreeHeap()) + 
                   ":time:" + String(millis());
  SerialBT.println(response);
  Serial.println("发送状态响应: " + response);
}

// 发送Ping响应
void sendPingResponse() {
  String response = "pong:" + String(millis());
  SerialBT.println(response);
  Serial.println("发送Pong响应: " + response);
}

// 发送回显响应
void sendEchoResponse(String message) {
  String response = "echo:" + message + ":" + String(millis());
  SerialBT.println(response);
  Serial.println("发送回显响应: " + response);
}

// 发送帮助响应
void sendHelpResponse() {
  String response = "help:支持的命令:test,status,ping,echo:消息";
  SerialBT.println(response);
  Serial.println("发送帮助响应: " + response);
}

// 显示状态信息
void showStatus() {
  static unsigned long lastStatusDisplay = 0;
  
  if (millis() - lastStatusDisplay > 10000) { // 每10秒显示一次
    lastStatusDisplay = millis();
    
    Serial.println("系统状态:");
    Serial.println("  蓝牙连接: " + String(bluetoothConnected ? "已连接" : "未连接"));
    Serial.println("  命令计数: " + String(commandCount));
    Serial.println("  可用内存: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("  运行时间: " + String(millis() / 1000) + " 秒");
    
    if (!bluetoothConnected) {
      Serial.println();
      Serial.println("等待蓝牙连接...");
      Serial.println("设备名称: " + String(BT_DEVICE_NAME));
      Serial.println("配对密码: " + String(BT_PIN));
    }
    Serial.println();
  }
}
