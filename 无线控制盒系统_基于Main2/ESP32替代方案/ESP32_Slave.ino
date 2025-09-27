/*
 * ESP32子控制器 - 替代nRF24L01方案
 * 使用ESP32内置蓝牙功能接收控制指令
 * 
 * 硬件连接：
 * - 电机、舵机连接方式保持不变
 * - 无需额外的无线模块
 */

#include <BluetoothSerial.h>
#include <ArduinoJson.h>

// 蓝牙串口
BluetoothSerial SerialBT;

// 系统参数
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
unsigned long lastDataTime = 0;
const unsigned long CONNECTION_TIMEOUT = 5000; // 5秒连接超时

// 引脚定义
#define MotorPwmPin 7
#define MotorIN1Pin 6
#define MotorIN2Pin 5
#define MotorIN3Pin 4
#define SteerPwmPin 9

void setup() {
  Serial.begin(115200);
  
  // 初始化蓝牙
  SerialBT.begin("焊接小车子控"); // 蓝牙设备名称
  Serial.println("ESP32子控制器启动");
  Serial.println("蓝牙设备名称: 焊接小车子控");
  
  // 初始化其他组件
  LedInit();
  MotorInit();
  SteerInit();
  
  Serial.println("=== ESP32子控制器启动完成 ===");
  Serial.println("等待主控制器连接...");
  
  // 自动连接主控制器
  connectToMaster();
}

void loop() {
  // 处理蓝牙通信
  handleBluetoothCommunication();
  
  // 检查连接状态
  checkConnectionStatus();
  
  // 处理控制逻辑
  LedShow();
  SteerLoop();
  
  // 状态指示
  static unsigned long lastStatusTime = 0;
  if (millis() - lastStatusTime > 10000) {
    lastStatusTime = millis();
    if (isConnected) {
      Serial.println("状态：已连接，等待控制指令");
    } else {
      Serial.println("状态：未连接，尝试重连...");
      connectToMaster();
    }
  }
}

// 连接到主控制器
void connectToMaster() {
  Serial.println("正在连接主控制器...");
  // 这里可以实现自动连接逻辑
  // 或者等待主控制器主动连接
}

// 处理蓝牙通信
void handleBluetoothCommunication() {
  if (SerialBT.available()) {
    String message = SerialBT.readString();
    message.trim();
    
    if (message == "CONNECTED") {
      isConnected = true;
      lastDataTime = millis();
      Serial.println("已连接到主控制器");
      SerialBT.println("CONNECT");
    } else if (message.startsWith("DATA:")) {
      // 处理控制数据
      String jsonData = message.substring(5); // 移除"DATA:"前缀
      processControlData(jsonData);
      lastDataTime = millis();
    }
  }
}

// 处理控制数据
void processControlData(String jsonData) {
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, jsonData);
  
  if (error) {
    Serial.println("JSON解析错误: " + String(error.c_str()));
    return;
  }
  
  // 更新控制参数
  Val.motorSw = doc["motorSw"];
  Val.steerSw = doc["steerSw"];
  Val.SteerLeftTimeFlag = doc["SteerLeftTimeFlag"];
  Val.motorSwFlag = doc["motorSwFlag"];
  Val.motorSpeed = doc["motorSpeed"];
  Val.steerSpeed = doc["steerSpeed"];
  Val.steerAngle = doc["steerAngle"];
  Val.steerLVal = doc["steerLVal"];
  Val.steerRVal = doc["steerRVal"];
  Val.steerNowVal = doc["steerNowVal"];
  Val.steerLTime = doc["steerLTime"];
  Val.steerRTime = doc["steerRTime"];
  Val.steerMidVal = doc["steerMidVal"];
  Val.steerAngle_show = doc["steerAngle_show"];
  
  // 更新编码器数据
  JsonArray ecArray = doc["ecCnt"];
  for (int i = 0; i < 5 && i < ecArray.size(); i++) {
    Val.ecCnt[i] = ecArray[i];
  }
  
  Serial.println("控制数据已更新");
  
  // 发送状态反馈
  sendStatusFeedback();
}

// 发送状态反馈
void sendStatusFeedback() {
  DynamicJsonDocument statusDoc(256);
  statusDoc["status"] = "OK";
  statusDoc["motorSw"] = Val.motorSw;
  statusDoc["steerSw"] = Val.steerSw;
  statusDoc["timestamp"] = millis();
  
  String statusJson;
  serializeJson(statusDoc, statusJson);
  SerialBT.println("STATUS:" + statusJson);
}

// 检查连接状态
void checkConnectionStatus() {
  if (isConnected && (millis() - lastDataTime > CONNECTION_TIMEOUT)) {
    isConnected = false;
    Serial.println("连接超时，已断开");
  }
}

// 其他函数保持与原系统相同
void LedInit() {
  // 数码管初始化代码
}

void MotorInit() {
  // 电机初始化代码
  Val.motorSw = false;
  Val.motorSwFlag = 0;
  pinMode(MotorPwmPin, OUTPUT);
  pinMode(MotorIN1Pin, OUTPUT);
  pinMode(MotorIN2Pin, OUTPUT);
  pinMode(MotorIN3Pin, OUTPUT);
}

void SteerInit() {
  // 舵机初始化代码
  Val.steerSw = false;
  // 舵机初始化代码
}

void LedShow() {
  // 数码管显示代码
}

void SteerLoop() {
  // 舵机控制代码
  static unsigned long preTime = 0;
  unsigned long nowTime = millis();
  
  if (nowTime - preTime >= 32) {
    preTime = nowTime;
    
    if (Val.steerSw) {
      // 舵机控制逻辑
      // 这里实现具体的舵机控制代码
    }
    
    // 电机控制
    MotorLoop();
  }
}

void MotorLoop() {
  int outputValue;
  if (Val.motorSwFlag == 1) {
    digitalWrite(MotorIN1Pin, HIGH);
    digitalWrite(MotorIN2Pin, LOW);
    digitalWrite(MotorIN3Pin, HIGH);
    outputValue = map(Val.motorSpeed, 0, 99, 255, 0);
    analogWrite(MotorPwmPin, outputValue);
  } else {
    digitalWrite(MotorIN1Pin, LOW);
    digitalWrite(MotorIN2Pin, LOW);
    digitalWrite(MotorIN3Pin, LOW);
    analogWrite(MotorPwmPin, 0);
  }
}





