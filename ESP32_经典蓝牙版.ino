/*
 * ESP32经典蓝牙版
 * 功能：使用经典蓝牙进行ESP32间通信
 * 特点：不依赖WiFi，使用经典蓝牙
 */

#include "BluetoothSerial.h"

// 蓝牙配置
BluetoothSerial SerialBT;
const char* deviceName = "ESP32_ClassicBT";
const char* targetDeviceName = "ESP32_ClassicBT_Target";

// 全局变量
bool bluetoothConnected = false;
unsigned long lastHeartbeat = 0;
int messageCount = 0;

void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(2000);
  
  Serial.println();
  Serial.println("==========================================");
  Serial.println("ESP32经典蓝牙版");
  Serial.println("==========================================");
  
  // 显示系统信息
  displaySystemInfo();
  
  // 初始化GPIO
  pinMode(2, OUTPUT); // 内置LED
  
  // 初始化经典蓝牙
  initClassicBluetooth();
  
  Serial.println("==========================================");
  Serial.println("经典蓝牙系统初始化完成！");
  Serial.println("设备名称: " + String(deviceName));
  Serial.println("==========================================");
}

void loop() {
  // 处理蓝牙连接
  handleBluetoothConnection();
  
  // 处理蓝牙数据
  handleBluetoothData();
  
  // 发送心跳
  sendHeartbeat();
  
  // 发送测试消息
  sendTestMessages();
  
  // 闪烁LED
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 1000) {
    lastBlink = millis();
    digitalWrite(2, !digitalRead(2));
  }
  
  // 显示状态信息
  showStatus();
  
  delay(100);
}

// 显示系统信息
void displaySystemInfo() {
  Serial.println("系统信息:");
  Serial.print("  芯片型号: ");
  Serial.println(ESP.getChipModel());
  Serial.print("  可用内存: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
  Serial.print("  CPU温度: ");
  Serial.print(temperatureRead());
  Serial.println("°C");
  Serial.println();
}

// 初始化经典蓝牙
void initClassicBluetooth() {
  Serial.println("初始化经典蓝牙...");
  
  // 检查蓝牙是否可用
  if (!SerialBT.begin(deviceName)) {
    Serial.println("✗ 经典蓝牙初始化失败");
    return;
  }
  
  Serial.println("✓ 经典蓝牙初始化成功");
  Serial.println("设备名称: " + String(deviceName));
  Serial.println("等待连接...");
  
  // 设置蓝牙为可发现模式
  SerialBT.enableSSP();
}

// 处理蓝牙连接
void handleBluetoothConnection() {
  if (SerialBT.hasClient()) {
    if (!bluetoothConnected) {
      bluetoothConnected = true;
      Serial.println("✓ 蓝牙客户端已连接");
    }
  } else {
    if (bluetoothConnected) {
      bluetoothConnected = false;
      Serial.println("✗ 蓝牙客户端已断开");
    }
  }
}

// 处理蓝牙数据
void handleBluetoothData() {
  if (SerialBT.available()) {
    String receivedData = SerialBT.readString();
    receivedData.trim();
    
    if (receivedData.length() > 0) {
      Serial.println("收到数据: " + receivedData);
      messageCount++;
      
      // 处理不同类型的消息
      if (receivedData.startsWith("heartbeat:")) {
        Serial.println("收到心跳: " + receivedData);
      } else if (receivedData.startsWith("test:")) {
        Serial.println("收到测试消息: " + receivedData);
        // 发送回复
        SerialBT.println("test_response: " + receivedData + " from " + deviceName);
      } else if (receivedData.startsWith("ping:")) {
        Serial.println("收到Ping: " + receivedData);
        // 发送Pong
        SerialBT.println("pong: " + receivedData + " from " + deviceName);
      } else if (receivedData.startsWith("status:")) {
        Serial.println("收到状态请求: " + receivedData);
        // 发送状态信息
        String status = "status_response: memory=" + String(ESP.getFreeHeap()) + 
                       ", temp=" + String(temperatureRead()) + 
                       ", uptime=" + String(millis() / 1000);
        SerialBT.println(status);
      } else {
        Serial.println("收到未知消息: " + receivedData);
        // 回显消息
        SerialBT.println("echo: " + receivedData);
      }
    }
  }
}

// 发送心跳
void sendHeartbeat() {
  if (bluetoothConnected && millis() - lastHeartbeat > 5000) { // 每5秒发送一次
    lastHeartbeat = millis();
    
    String heartbeat = "heartbeat:" + String(millis()) + ":" + deviceName;
    SerialBT.println(heartbeat);
    Serial.println("发送心跳: " + heartbeat);
  }
}

// 发送测试消息
void sendTestMessages() {
  static unsigned long lastTest = 0;
  if (bluetoothConnected && millis() - lastTest > 10000) { // 每10秒发送一次
    lastTest = millis();
    
    static int testIndex = 0;
    String testMessages[] = {"test: message " + String(testIndex), 
                            "ping: " + String(millis()),
                            "status: request"};
    
    String testMessage = testMessages[testIndex % 3];
    SerialBT.println(testMessage);
    Serial.println("发送测试消息: " + testMessage);
    testIndex++;
  }
}

// 显示状态信息
void showStatus() {
  static unsigned long lastStatusDisplay = 0;
  
  if (millis() - lastStatusDisplay > 15000) { // 每15秒显示一次
    lastStatusDisplay = millis();
    
    Serial.println();
    Serial.println("==========================================");
    Serial.println("经典蓝牙状态");
    Serial.println("==========================================");
    
    Serial.println("蓝牙状态:");
    Serial.println("  连接状态: " + String(bluetoothConnected ? "已连接" : "未连接"));
    Serial.println("  设备名称: " + String(deviceName));
    
    Serial.println("通信状态:");
    Serial.println("  消息计数: " + String(messageCount));
    
    Serial.println("系统状态:");
    Serial.println("  可用内存: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("  CPU温度: " + String(temperatureRead()) + "°C");
    Serial.println("  运行时间: " + String(millis() / 1000) + " 秒");
    
    if (!bluetoothConnected) {
      Serial.println();
      Serial.println("等待蓝牙连接...");
      Serial.println("设备名称: " + String(deviceName));
    }
    
    Serial.println("==========================================");
    Serial.println();
  }
}
