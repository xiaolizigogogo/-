/*
 * ESP32作为无线模块
 * 通过串口与Arduino Mega2560通信
 * 仅需要4-5个接口连接
 */

#include <BluetoothSerial.h>

// 蓝牙串口
BluetoothSerial SerialBT;

// 串口通信
HardwareSerial MegaSerial(2); // 使用ESP32的Serial2

// 设备角色
bool isMaster = true; // true=主控制器，false=子控制器

// 通信状态
bool isConnected = false;
unsigned long lastDataTime = 0;

void setup() {
  // 初始化串口
  Serial.begin(115200);
  MegaSerial.begin(9600, SERIAL_8N1, 16, 17); // RX=16, TX=17
  
  // 初始化蓝牙
  String deviceName = isMaster ? "焊接小车主控" : "焊接小车子控";
  SerialBT.begin(deviceName);
  
  Serial.println("=== ESP32无线模块启动 ===");
  Serial.println("设备角色: " + String(isMaster ? "主控制器" : "子控制器"));
  Serial.println("蓝牙名称: " + deviceName);
  Serial.println("串口通信: 9600bps");
  
  if (isMaster) {
    Serial.println("等待子控制器连接...");
  } else {
    Serial.println("等待主控制器连接...");
  }
}

void loop() {
  // 处理蓝牙通信
  handleBluetoothCommunication();
  
  // 处理串口通信
  handleSerialCommunication();
  
  // 检查连接状态
  checkConnectionStatus();
  
  delay(10);
}

// 处理蓝牙通信
void handleBluetoothCommunication() {
  if (SerialBT.available()) {
    String message = SerialBT.readString();
    message.trim();
    
    if (message == "CONNECT") {
      isConnected = true;
      lastDataTime = millis();
      Serial.println("设备已连接");
      SerialBT.println("CONNECTED");
      
      // 通知Mega2560连接状态
      MegaSerial.println("BT_CONNECTED");
      
    } else if (message == "DISCONNECT") {
      isConnected = false;
      Serial.println("设备已断开");
      
      // 通知Mega2560断开状态
      MegaSerial.println("BT_DISCONNECTED");
      
    } else if (message.startsWith("DATA:")) {
      // 转发数据到Mega2560
      String data = message.substring(5);
      MegaSerial.println("BT_DATA:" + data);
      lastDataTime = millis();
    }
  }
}

// 处理串口通信
void handleSerialCommunication() {
  if (MegaSerial.available()) {
    String message = MegaSerial.readString();
    message.trim();
    
    if (message.startsWith("SEND:")) {
      // Mega2560请求发送数据
      String data = message.substring(5);
      if (isConnected) {
        SerialBT.println("DATA:" + data);
        Serial.println("转发数据: " + data);
      } else {
        Serial.println("未连接，无法发送数据");
      }
      
    } else if (message == "GET_STATUS") {
      // Mega2560请求连接状态
      String status = isConnected ? "CONNECTED" : "DISCONNECTED";
      MegaSerial.println("STATUS:" + status);
      
    } else if (message.startsWith("CONFIG:")) {
      // Mega2560发送配置
      String config = message.substring(7);
      Serial.println("收到配置: " + config);
      // 这里可以处理配置信息
    }
  }
}

// 检查连接状态
void checkConnectionStatus() {
  static unsigned long lastStatusTime = 0;
  if (millis() - lastStatusTime > 5000) {
    lastStatusTime = millis();
    
    if (isConnected) {
      // 检查数据超时
      if (millis() - lastDataTime > 10000) {
        isConnected = false;
        Serial.println("连接超时，已断开");
        MegaSerial.println("BT_DISCONNECTED");
      } else {
        Serial.println("状态：已连接，通信正常");
      }
    } else {
      Serial.println("状态：未连接，等待配对...");
    }
  }
}







