/*
 * ESP32蓝牙通信简化版
 * 无需WiFi，仅使用蓝牙功能
 * 
 * 使用方法：
 * 1. 上传此程序到两个ESP32
 * 2. 一个作为主控制器，一个作为子控制器
 * 3. 通过蓝牙自动配对通信
 */

#include <BluetoothSerial.h>

// 蓝牙串口
BluetoothSerial SerialBT;

// 设备角色：true=主控制器，false=子控制器
bool isMaster = true; // 修改这里来设置设备角色

// 通信状态
bool isConnected = false;
unsigned long lastDataTime = 0;

// 控制参数
struct ControlData {
  bool motorSw;
  bool steerSw;
  int motorSpeed;
  int steerSpeed;
  float steerAngle;
  unsigned long timestamp;
};

ControlData controlData;

void setup() {
  Serial.begin(115200);
  
  // 初始化蓝牙
  String deviceName = isMaster ? "焊接小车主控" : "焊接小车子控";
  SerialBT.begin(deviceName);
  
  Serial.println("=== ESP32蓝牙通信系统 ===");
  Serial.println("设备角色: " + String(isMaster ? "主控制器" : "子控制器"));
  Serial.println("蓝牙名称: " + deviceName);
  
  // 初始化控制参数
  controlData.motorSw = false;
  controlData.steerSw = false;
  controlData.motorSpeed = 0;
  controlData.steerSpeed = 0;
  controlData.steerAngle = 0;
  
  if (isMaster) {
    Serial.println("等待子控制器连接...");
  } else {
    Serial.println("等待主控制器连接...");
  }
}

void loop() {
  if (isMaster) {
    // 主控制器逻辑
    masterLoop();
  } else {
    // 子控制器逻辑
    slaveLoop();
  }
  
  // 检查连接状态
  checkConnection();
  
  delay(100);
}

// 主控制器循环
void masterLoop() {
  // 模拟控制输入（实际使用时替换为您的编码器和按钮）
  static unsigned long lastInputTime = 0;
  if (millis() - lastInputTime > 1000) {
    lastInputTime = millis();
    
    // 模拟控制参数变化
    controlData.motorSpeed = (controlData.motorSpeed + 1) % 100;
    controlData.steerAngle = sin(millis() / 1000.0) * 50;
    
    // 发送控制数据
    if (isConnected) {
      sendControlData();
    }
  }
  
  // 处理接收到的数据
  if (SerialBT.available()) {
    String message = SerialBT.readString();
    message.trim();
    
    if (message == "CONNECT") {
      isConnected = true;
      Serial.println("子控制器已连接");
      SerialBT.println("CONNECTED");
    } else if (message.startsWith("STATUS:")) {
      Serial.println("收到状态: " + message);
    }
  }
}

// 子控制器循环
void slaveLoop() {
  // 处理接收到的控制数据
  if (SerialBT.available()) {
    String message = SerialBT.readString();
    message.trim();
    
    if (message == "CONNECTED") {
      isConnected = true;
      Serial.println("已连接到主控制器");
      SerialBT.println("CONNECT");
    } else if (message.startsWith("CONTROL:")) {
      processControlData(message);
    }
  }
  
  // 发送状态反馈
  static unsigned long lastStatusTime = 0;
  if (isConnected && millis() - lastStatusTime > 2000) {
    lastStatusTime = millis();
    sendStatusFeedback();
  }
}

// 发送控制数据
void sendControlData() {
  String data = "CONTROL:";
  data += controlData.motorSw ? "1" : "0";
  data += ",";
  data += controlData.steerSw ? "1" : "0";
  data += ",";
  data += String(controlData.motorSpeed);
  data += ",";
  data += String(controlData.steerSpeed);
  data += ",";
  data += String(controlData.steerAngle);
  data += ",";
  data += String(millis());
  
  SerialBT.println(data);
  Serial.println("发送控制数据: " + data);
}

// 处理控制数据
void processControlData(String data) {
  // 解析控制数据
  int firstComma = data.indexOf(',', 8);
  int secondComma = data.indexOf(',', firstComma + 1);
  int thirdComma = data.indexOf(',', secondComma + 1);
  int fourthComma = data.indexOf(',', thirdComma + 1);
  int fifthComma = data.indexOf(',', fourthComma + 1);
  
  if (firstComma > 0 && secondComma > 0 && thirdComma > 0 && fourthComma > 0 && fifthComma > 0) {
    controlData.motorSw = data.substring(8, firstComma) == "1";
    controlData.steerSw = data.substring(firstComma + 1, secondComma) == "1";
    controlData.motorSpeed = data.substring(secondComma + 1, thirdComma).toInt();
    controlData.steerSpeed = data.substring(thirdComma + 1, fourthComma).toInt();
    controlData.steerAngle = data.substring(fourthComma + 1, fifthComma).toFloat();
    controlData.timestamp = data.substring(fifthComma + 1).toInt();
    
    Serial.println("收到控制数据:");
    Serial.println("  电机开关: " + String(controlData.motorSw));
    Serial.println("  舵机开关: " + String(controlData.steerSw));
    Serial.println("  电机速度: " + String(controlData.motorSpeed));
    Serial.println("  舵机速度: " + String(controlData.steerSpeed));
    Serial.println("  舵机角度: " + String(controlData.steerAngle));
    
    // 这里添加实际的控制逻辑
    executeControl();
  }
}

// 执行控制指令
void executeControl() {
  // 电机控制
  if (controlData.motorSw) {
    // 启动电机，速度 = controlData.motorSpeed
    Serial.println("执行电机控制: 速度=" + String(controlData.motorSpeed));
  } else {
    // 停止电机
    Serial.println("停止电机");
  }
  
  // 舵机控制
  if (controlData.steerSw) {
    // 控制舵机，角度 = controlData.steerAngle
    Serial.println("执行舵机控制: 角度=" + String(controlData.steerAngle));
  } else {
    // 舵机回到中位
    Serial.println("舵机回到中位");
  }
}

// 发送状态反馈
void sendStatusFeedback() {
  String status = "STATUS:";
  status += controlData.motorSw ? "1" : "0";
  status += ",";
  status += controlData.steerSw ? "1" : "0";
  status += ",";
  status += String(millis());
  
  SerialBT.println(status);
  Serial.println("发送状态反馈: " + status);
}

// 检查连接状态
void checkConnection() {
  static unsigned long lastStatusTime = 0;
  if (millis() - lastStatusTime > 5000) {
    lastStatusTime = millis();
    
    if (isConnected) {
      Serial.println("状态：已连接，通信正常");
    } else {
      Serial.println("状态：未连接，等待配对...");
    }
  }
}





