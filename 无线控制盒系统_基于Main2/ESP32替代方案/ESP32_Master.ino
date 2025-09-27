/*
 * ESP32主控制器 - 替代nRF24L01方案
 * 使用ESP32内置WiFi/蓝牙功能
 * 
 * 硬件连接：
 * - 编码器、按钮、舵机、电机连接方式保持不变
 * - 无需额外的无线模块
 */

#include <WiFi.h>
#include <BluetoothSerial.h>
#include <ArduinoJson.h>

// 蓝牙串口
BluetoothSerial SerialBT;

// 系统参数（与原系统相同）
typedef struct {
  bool motorSw;
  bool steerSw;
  int SteerLeftTimeFlag;
  int motorSwFlag;
  int motorSpeed;
  int steerSpeed;
  int steerInterval;
  float steerAngle;
  float steerLVal;
  float steerRVal;
  float steerNowVal;
  int steerLTime;
  int steerRTime;
  int steerLTimeAction;
  int steerRTimeAction;
  int steerMidVal;
  int steerAngle_show;
  int ecCnt[5];
} param_t;

param_t Val;

// 通信状态
bool isConnected = false;
unsigned long lastSyncTime = 0;
const unsigned long SYNC_INTERVAL = 1000; // 1秒同步一次

// 引脚定义（与原系统相同）
#define bt1 31
#define bt2 33
#define bt3 35
#define bt4 37
#define bt5 39

#define MotorPwmPin 7
#define MotorIN1Pin 6
#define MotorIN2Pin 5
#define MotorIN3Pin 4
#define SteerPwmPin 9

// 编码器引脚
int CLK0 = 2, DT0 = 22;
int CLK1 = 3, DT1 = 23;
int CLK2 = 21, DT2 = 24;
int CLK3 = 20, DT3 = 14;
int CLK4 = 19, DT20 = 15;

int ecCnt[5];
int CLK0_value = 1, DT0_value = 1;
int CLK1_value = 1, DT1_value = 1;
int CLK2_value = 1, DT2_value = 1;
int CLK3_value = 1, DT3_value = 1;
int CLK4_value = 1, DT4_value = 1;

void setup() {
  Serial.begin(115200);
  
  // 初始化蓝牙
  SerialBT.begin("焊接小车主控"); // 蓝牙设备名称
  Serial.println("ESP32主控制器启动");
  Serial.println("蓝牙设备名称: 焊接小车主控");
  
  // 初始化其他组件
  LedInit();
  BtInit();
  MotorInit();
  SteerInit();
  CoderInit();
  
  Serial.println("=== ESP32主控制器启动完成 ===");
  Serial.println("等待子控制器连接...");
}

void loop() {
  // 处理蓝牙连接
  handleBluetoothConnection();
  
  // 处理控制逻辑
  BtLoop();
  LedShow();
  SteerLoop();
  
  // 数据同步
  if (isConnected && (millis() - lastSyncTime > SYNC_INTERVAL)) {
    sendSyncData();
  }
  
  // 状态指示
  static unsigned long lastStatusTime = 0;
  if (millis() - lastStatusTime > 10000) {
    lastStatusTime = millis();
    if (isConnected) {
      Serial.println("状态：已连接，数据同步正常");
    } else {
      Serial.println("状态：等待连接...");
    }
  }
}

// 处理蓝牙连接
void handleBluetoothConnection() {
  if (SerialBT.available()) {
    String message = SerialBT.readString();
    message.trim();
    
    if (message == "CONNECT") {
      isConnected = true;
      Serial.println("子控制器已连接");
      SerialBT.println("CONNECTED");
    } else if (message == "DISCONNECT") {
      isConnected = false;
      Serial.println("子控制器已断开");
    } else if (message.startsWith("STATUS:")) {
      // 处理状态反馈
      Serial.println("收到状态: " + message);
    }
  }
}

// 发送同步数据
void sendSyncData() {
  // 创建JSON数据包
  DynamicJsonDocument doc(1024);
  
  doc["motorSw"] = Val.motorSw;
  doc["steerSw"] = Val.steerSw;
  doc["SteerLeftTimeFlag"] = Val.SteerLeftTimeFlag;
  doc["motorSwFlag"] = Val.motorSwFlag;
  doc["motorSpeed"] = Val.motorSpeed;
  doc["steerSpeed"] = Val.steerSpeed;
  doc["steerAngle"] = Val.steerAngle;
  doc["steerLVal"] = Val.steerLVal;
  doc["steerRVal"] = Val.steerRVal;
  doc["steerNowVal"] = Val.steerNowVal;
  doc["steerLTime"] = Val.steerLTime;
  doc["steerRTime"] = Val.steerRTime;
  doc["steerMidVal"] = Val.steerMidVal;
  doc["steerAngle_show"] = Val.steerAngle_show;
  
  // 添加编码器数据
  JsonArray ecArray = doc.createNestedArray("ecCnt");
  for (int i = 0; i < 5; i++) {
    ecArray.add(ecCnt[i]);
  }
  
  doc["timestamp"] = millis();
  
  // 发送数据
  String jsonString;
  serializeJson(doc, jsonString);
  SerialBT.println("DATA:" + jsonString);
  
  lastSyncTime = millis();
}

// 触发数据同步
void triggerDataSync() {
  if (isConnected) {
    sendSyncData();
  }
}

// 其他函数保持与原系统相同
void LedInit() {
  // 数码管初始化代码
}

void BtInit() {
  // 按钮初始化代码
}

void MotorInit() {
  // 电机初始化代码
}

void SteerInit() {
  // 舵机初始化代码
}

void CoderInit() {
  // 编码器初始化代码
}

void BtLoop() {
  // 按钮循环处理代码
}

void LedShow() {
  // 数码管显示代码
}

void SteerLoop() {
  // 舵机控制代码
}

// 中断处理函数
void ClockChanged0() {
  // 编码器0中断处理
  triggerDataSync();
}

void ClockChanged1() {
  // 编码器1中断处理
  triggerDataSync();
}

void ClockChanged2() {
  // 编码器2中断处理
  triggerDataSync();
}

void ClockChanged3() {
  // 编码器3中断处理
  triggerDataSync();
}

void ClockChanged4() {
  // 编码器4中断处理
  triggerDataSync();
}





