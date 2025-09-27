/*
 * Arduino Mega2560主控制器
 * 通过串口与ESP32无线模块通信
 * 保持所有原有功能不变
 */

#include <SoftwareSerial.h>

// ESP32串口通信
SoftwareSerial ESP32Serial(18, 19); // RX=18, TX=19

// 原有系统参数保持不变
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

// 原有引脚定义保持不变
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
  
  // 初始化ESP32串口
  ESP32Serial.begin(9600);
  
  // 初始化原有系统
  LedInit();
  BtInit();
  MotorInit();
  SteerInit();
  CoderInit();
  
  Serial.println("=== Arduino Mega2560主控制器启动 ===");
  Serial.println("ESP32无线模块已连接");
  Serial.println("等待子控制器连接...");
  
  // 请求连接状态
  ESP32Serial.println("GET_STATUS");
}

void loop() {
  // 处理ESP32通信
  handleESP32Communication();
  
  // 原有系统循环
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
      Serial.println("状态：未连接，等待重试...");
    }
  }
}

// 处理ESP32通信
void handleESP32Communication() {
  if (ESP32Serial.available()) {
    String message = ESP32Serial.readString();
    message.trim();
    
    if (message == "STATUS:CONNECTED") {
      isConnected = true;
      Serial.println("子控制器已连接");
    } else if (message == "STATUS:DISCONNECTED") {
      isConnected = false;
      Serial.println("子控制器已断开");
    } else if (message == "BT_CONNECTED") {
      isConnected = true;
      Serial.println("蓝牙连接已建立");
    } else if (message == "BT_DISCONNECTED") {
      isConnected = false;
      Serial.println("蓝牙连接已断开");
    } else if (message.startsWith("BT_DATA:")) {
      // 处理接收到的数据
      String data = message.substring(8);
      Serial.println("收到数据: " + data);
    }
  }
}

// 发送同步数据
void sendSyncData() {
  String data = "";
  data += Val.motorSw ? "1" : "0";
  data += ",";
  data += Val.steerSw ? "1" : "0";
  data += ",";
  data += String(Val.SteerLeftTimeFlag);
  data += ",";
  data += String(Val.motorSwFlag);
  data += ",";
  data += String(Val.motorSpeed);
  data += ",";
  data += String(Val.steerSpeed);
  data += ",";
  data += String(Val.steerAngle);
  data += ",";
  data += String(Val.steerLVal);
  data += ",";
  data += String(Val.steerRVal);
  data += ",";
  data += String(Val.steerNowVal);
  data += ",";
  data += String(Val.steerLTime);
  data += ",";
  data += String(Val.steerRTime);
  data += ",";
  data += String(Val.steerMidVal);
  data += ",";
  data += String(Val.steerAngle_show);
  data += ",";
  
  // 添加编码器数据
  for (int i = 0; i < 5; i++) {
    data += String(ecCnt[i]);
    if (i < 4) data += ",";
  }
  
  data += ",";
  data += String(millis());
  
  // 发送数据
  ESP32Serial.println("SEND:" + data);
  lastSyncTime = millis();
}

// 触发数据同步
void triggerDataSync() {
  if (isConnected) {
    sendSyncData();
  }
}

// 原有函数保持不变
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





