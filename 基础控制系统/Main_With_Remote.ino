#include "FBLed.h"
#include <stdio.h>
#include <Servo.h>
#include "myButton.h"
#include <KeyValueEEPROM.h>
#include <EEPROM.h>
#include <RCSwitch.h>  // 添加遥控器库

Servo myservo;
FBLed led1;
FBLed led2;
FBLed led3;

// 无线遥控器定义
RCSwitch mySwitch = RCSwitch();

// 遥控器按键映射（需要根据实际遥控器调整）
enum RemoteCommands {
  REMOTE_NONE = 0,
  REMOTE_MOTOR_FWD = 12345,    // 电机前进
  REMOTE_MOTOR_BWD = 12346,    // 电机后退
  REMOTE_MOTOR_STOP = 12347,   // 电机停止
  REMOTE_SERVO_LEFT = 12348,   // 舵机左转
  REMOTE_SERVO_RIGHT = 12349,  // 舵机右转
  REMOTE_SERVO_CENTER = 12350, // 舵机回中
  REMOTE_SPEED_UP = 12351,     // 加速
  REMOTE_SPEED_DOWN = 12352,   // 减速
  REMOTE_SAVE_PARAM = 12353,   // 保存参数
  REMOTE_LOAD_PARAM = 12354,   // 加载参数
  REMOTE_STATUS = 12355,       // 状态查询
  REMOTE_RESET = 12356         // 系统重置
};

// 遥控器相关变量
unsigned long lastRemoteTime = 0;
const unsigned long REMOTE_DEBOUNCE = 200; // 200ms防抖
bool remoteEnabled = true;

// 原有引脚定义
#define MIN_ANGLE 0
#define MAX_ANGLE  180
#define MID 1550
#define MIN_MID  1100
#define MAX_MID 2000

#define bt1 31  // record param
#define bt2 33  // steer sw
#define bt3 35  //
#define bt4 37
#define bt5 39 // motor sw
#define bt6 41 // motor sw
#define bt7 43 // motor sw

#define now_count 0
#define total_count 100*k

#define MotorPwmPin 7  // 电机pwm输出引脚
#define MotorPwmPin2 8  // 电机pwm输出引脚
#define MotorPwmPin3 10  // 电机pwm输出引脚
#define MotorIN1Pin 6  // 电机IN1控制引脚
#define MotorIN2Pin 5  // 电机IN2控制引脚
#define MotorIN3Pin 4  // 电机IN2控制引脚

#define SteerPwmPin 9  // 舵机pwm输出引脚

// 编码器引脚定义
int CLK0 = 2;
int DT0 = 11;
int CLK1 = 3;
int DT1 = 12;
int CLK2 = 21;
int DT2 = 13;
int CLK3 = 20;
int DT3 = 14;
int CLK4 = 19;
int DT4 = 15;
int CLK5 = 18;
int DT5 = 16;
int CLK6 = 23;
int DT6 = 17;

float k = 0.25; //时长系数

// 编码器状态变量
int CLK3_value = 1;
int DT3_value = 1;
int CLK1_value = 1;
int DT1_value = 1;
int CLK0_value = 1;
int DT0_value = 1;
int CLK2_value = 1;
int DT2_value = 1;
int CLK4_value = 1;
int DT4_value = 1;
int CLK5_value = 1;
int DT5_value = 1;
int CLK6_value = 1;
int DT6_value = 1;

// 编码器数量常量
const int ENCODER_COUNT = 7;

// 编码器结构体
struct Encoder {
  int clkPin;
  int dtPin;
  int count;
  int lastClkState;
};

// 编码器数组
Encoder encoders[ENCODER_COUNT] = {
  {CLK0, DT0, 0, HIGH},
  {CLK1, DT1, 0, HIGH},
  {CLK2, DT2, 0, HIGH},
  {CLK3, DT3, 0, HIGH},
  {CLK4, DT4, 0, HIGH},
  {CLK5, DT5, 0, HIGH},
  {CLK6, DT6, 0, HIGH}
};

// 参数结构体
struct Parameters {
  int steerAngle;
  int steerSpeed;
  int steerLTime;
  int steerRTime;
  int steerLTimeAction;
  int steerRTimeAction;
  int steerNowVal;
  int steerMidVal;
  int steerAngle_show;
  int motorSpeed;
  int SteerLeftTimeFlag;
} Val;

// EEPROM地址定义
const int addr1 = 0;
const int addr2 = 1;
const int addr3 = 2;
const int addr4 = 3;
const int addr5 = 4;
const int addr6 = 5;
const int addr7 = 6;
const int addr8 = 7;
const int addr9 = 8;
const int addr10 = 9;
const int addr11 = 10;
const int addr12 = 11;
const int addr18 = 12;
const int addr101 = 13;
const int addr111 = 14;

void setup() {
  Serial.begin(9600);
  
  // 初始化无线接收器
  mySwitch.enableReceive(0); // 使用中断引脚0
  
  // 初始化原有硬件
  setupOriginalHardware();
  
  // 加载参数
  loadParameters();
  
  Serial.println("焊接小车遥控系统已启动");
  Serial.println("支持的遥控器指令：");
  Serial.println("按键1: 电机前进  按键2: 电机后退  按键3: 电机停止");
  Serial.println("按键4: 舵机左转  按键5: 舵机右转  按键6: 舵机回中");
  Serial.println("按键7: 加速      按键8: 减速      按键9: 保存参数");
  Serial.println("按键0: 加载参数  A键: 状态查询   B键: 系统重置");
}

void loop() {
  // 处理原有功能
  handleOriginalFunctions();
  
  // 处理遥控器控制
  handleRemoteControl();
  
  // 处理按钮控制（原有功能）
  handleButtonControl();
  
  delay(10);
}

void setupOriginalHardware() {
  // 初始化舵机
  myservo.attach(SteerPwmPin);
  myservo.write(90);
  
  // 初始化电机引脚
  pinMode(MotorPwmPin, OUTPUT);
  pinMode(MotorPwmPin2, OUTPUT);
  pinMode(MotorPwmPin3, OUTPUT);
  pinMode(MotorIN1Pin, OUTPUT);
  pinMode(MotorIN2Pin, OUTPUT);
  pinMode(MotorIN3Pin, OUTPUT);
  
  // 初始化按钮引脚
  pinMode(bt1, INPUT_PULLUP);
  pinMode(bt2, INPUT_PULLUP);
  pinMode(bt3, INPUT_PULLUP);
  pinMode(bt4, INPUT_PULLUP);
  pinMode(bt5, INPUT_PULLUP);
  pinMode(bt6, INPUT_PULLUP);
  pinMode(bt7, INPUT_PULLUP);
  
  // 初始化编码器引脚
  for (int i = 0; i < ENCODER_COUNT; i++) {
    pinMode(encoders[i].clkPin, INPUT_PULLUP);
    pinMode(encoders[i].dtPin, INPUT_PULLUP);
    encoders[i].lastClkState = digitalRead(encoders[i].clkPin);
  }
  
  // 初始化LED
  led1.begin(22);
  led2.begin(24);
  led3.begin(26);
  
  // 设置中断
  attachInterrupt(digitalPinToInterrupt(CLK0), ClockChanged0, CHANGE);
  attachInterrupt(digitalPinToInterrupt(CLK1), ClockChanged1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(CLK2), ClockChanged2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(CLK3), ClockChanged3, CHANGE);
  attachInterrupt(digitalPinToInterrupt(CLK4), ClockChanged4, CHANGE);
  attachInterrupt(digitalPinToInterrupt(CLK5), ClockChanged5, CHANGE);
  attachInterrupt(digitalPinToInterrupt(CLK6), ClockChanged6, CHANGE);
}

void handleOriginalFunctions() {
  // 处理LED显示（原有功能）
  led1.update();
  led2.update();
  led3.update();
  
  // 处理舵机控制
  handleServoControl();
  
  // 处理电机控制
  handleMotorControl();
}

void handleRemoteControl() {
  if (mySwitch.available()) {
    long value = mySwitch.getReceivedValue();
    
    // 防抖处理
    if (millis() - lastRemoteTime > REMOTE_DEBOUNCE) {
      Serial.print("收到遥控器指令: ");
      Serial.println(value);
      
      processRemoteCommand(value);
      lastRemoteTime = millis();
    }
    
    mySwitch.resetAvailable();
  }
}

void processRemoteCommand(long command) {
  switch (command) {
    case REMOTE_MOTOR_FWD:
      motorForward();
      break;
      
    case REMOTE_MOTOR_BWD:
      motorBackward();
      break;
      
    case REMOTE_MOTOR_STOP:
      motorStop();
      break;
      
    case REMOTE_SERVO_LEFT:
      servoLeft();
      break;
      
    case REMOTE_SERVO_RIGHT:
      servoRight();
      break;
      
    case REMOTE_SERVO_CENTER:
      servoCenter();
      break;
      
    case REMOTE_SPEED_UP:
      increaseSpeed();
      break;
      
    case REMOTE_SPEED_DOWN:
      decreaseSpeed();
      break;
      
    case REMOTE_SAVE_PARAM:
      saveParameters();
      break;
      
    case REMOTE_LOAD_PARAM:
      loadParameters();
      break;
      
    case REMOTE_STATUS:
      reportStatus();
      break;
      
    case REMOTE_RESET:
      resetSystem();
      break;
      
    default:
      Serial.println("未知遥控器指令");
      break;
  }
}

void motorForward() {
  digitalWrite(MotorIN1Pin, HIGH);
  digitalWrite(MotorIN2Pin, LOW);
  analogWrite(MotorPwmPin, Val.motorSpeed);
  Serial.println("电机前进");
}

void motorBackward() {
  digitalWrite(MotorIN1Pin, LOW);
  digitalWrite(MotorIN2Pin, HIGH);
  analogWrite(MotorPwmPin, Val.motorSpeed);
  Serial.println("电机后退");
}

void motorStop() {
  digitalWrite(MotorIN1Pin, LOW);
  digitalWrite(MotorIN2Pin, LOW);
  analogWrite(MotorPwmPin, 0);
  Serial.println("电机停止");
}

void servoLeft() {
  Val.steerAngle = max(MIN_ANGLE, Val.steerAngle - 10);
  myservo.write(Val.steerAngle);
  Serial.print("舵机左转至: ");
  Serial.println(Val.steerAngle);
}

void servoRight() {
  Val.steerAngle = min(MAX_ANGLE, Val.steerAngle + 10);
  myservo.write(Val.steerAngle);
  Serial.print("舵机右转至: ");
  Serial.println(Val.steerAngle);
}

void servoCenter() {
  Val.steerAngle = 90;
  myservo.write(Val.steerAngle);
  Serial.println("舵机回中");
}

void increaseSpeed() {
  Val.motorSpeed = min(255, Val.motorSpeed + 25);
  Serial.print("电机速度增加至: ");
  Serial.println(Val.motorSpeed);
}

void decreaseSpeed() {
  Val.motorSpeed = max(0, Val.motorSpeed - 25);
  Serial.print("电机速度减少至: ");
  Serial.println(Val.motorSpeed);
}

void reportStatus() {
  Serial.println("=== 系统状态报告 ===");
  Serial.print("舵机角度: ");
  Serial.println(Val.steerAngle);
  Serial.print("电机速度: ");
  Serial.println(Val.motorSpeed);
  Serial.print("舵机中位值: ");
  Serial.println(Val.steerMidVal);
  Serial.print("遥控器控制: ");
  Serial.println(remoteEnabled ? "启用" : "禁用");
  Serial.println("==================");
}

void resetSystem() {
  // 重置系统状态
  Val.steerAngle = 90;
  Val.motorSpeed = 128;
  Val.steerMidVal = MID;
  
  // 重置硬件
  myservo.write(90);
  motorStop();
  
  Serial.println("系统已重置");
}

void handleButtonControl() {
  // 原有的按钮控制逻辑
  if (digitalRead(bt1) == LOW) {
    saveParameters();
    delay(200);
  }
  
  if (digitalRead(bt2) == LOW) {
    // 舵机控制
    Val.steerAngle = (Val.steerAngle + 10) % 180;
    myservo.write(Val.steerAngle);
    delay(200);
  }
  
  if (digitalRead(bt5) == LOW) {
    motorForward();
    delay(200);
  }
  
  if (digitalRead(bt6) == LOW) {
    motorStop();
    delay(200);
  }
}

void handleServoControl() {
  // 原有的舵机控制逻辑
  // 这里可以添加自动控制逻辑
}

void handleMotorControl() {
  // 原有的电机控制逻辑
  // 这里可以添加自动控制逻辑
}

// 中断处理函数
void ClockChanged0() { handleEncoderInterrupt(0); }
void ClockChanged1() { handleEncoderInterrupt(1); }
void ClockChanged2() { handleEncoderInterrupt(2); }
void ClockChanged3() { handleEncoderInterrupt(3); }
void ClockChanged4() { handleEncoderInterrupt(4); }
void ClockChanged5() { handleEncoderInterrupt(5); }
void ClockChanged6() { handleEncoderInterrupt(6); }

void handleEncoderInterrupt(int encoderIndex) {
  Encoder* encoder = &encoders[encoderIndex];
  int currentClkState = digitalRead(encoder->clkPin);
  
  if (currentClkState != encoder->lastClkState) {
    int dtState = digitalRead(encoder->dtPin);
    
    if (dtState != currentClkState) {
      encoder->count++;
    } else {
      encoder->count--;
    }
    
    encoder->lastClkState = currentClkState;
  }
}

// 参数保存函数
void saveParameters() {
  // 保存舵机参数
  EEPROM.write(addr1, Val.steerAngle);
  EEPROM.write(addr2, Val.steerSpeed);
  EEPROM.write(addr3, Val.steerLTime);
  EEPROM.write(addr4, Val.steerRTime);
  EEPROM.write(addr18, Val.steerAngle_show);
  
  // 保存电机参数
  EEPROM.write(addr5, Val.motorSpeed);
  
  // 保存编码器值
  for (int i = 0; i < ENCODER_COUNT; i++) {
    if (i == 3) { // 编码器3需要16位存储
      int a1 = encoders[i].count % 100;
      int a2 = encoders[i].count / 100;
      EEPROM.write(addr10, a1);
      EEPROM.write(addr101, a2);
    } else {
      EEPROM.write(addr6 + i, encoders[i].count);
    }
  }
  
  // 保存中位点（16位）
  int a3 = Val.steerMidVal % 100;
  int a4 = Val.steerMidVal / 100;
  EEPROM.write(addr11, a3);
  EEPROM.write(addr111, a4);
  
  // 保存其他参数
  EEPROM.write(addr12, Val.SteerLeftTimeFlag);
  EEPROM.write(200, 1); // 保存标志
  
  Serial.println("参数已保存到EEPROM");
}

// 参数加载函数
void loadParameters() {
  if (EEPROM.read(200) != 0) {
    // 加载舵机参数
    Val.steerAngle = EEPROM.read(addr1);
    Val.steerSpeed = EEPROM.read(addr2);
    Val.steerLTime = EEPROM.read(addr3);
    Val.steerRTime = EEPROM.read(addr4);
    Val.steerAngle_show = EEPROM.read(addr18);
    
    // 加载电机参数
    Val.motorSpeed = EEPROM.read(addr5);
    
    // 加载编码器值
    for (int i = 0; i < ENCODER_COUNT; i++) {
      if (i == 3) { // 编码器3需要16位读取
        int a1 = EEPROM.read(addr10);
        int a2 = EEPROM.read(addr101);
        encoders[i].count = a2 * 100 + a1;
      } else {
        encoders[i].count = EEPROM.read(addr6 + i);
      }
    }
    
    // 加载中位点（16位）
    int a3 = EEPROM.read(addr11);
    int a4 = EEPROM.read(addr111);
    Val.steerMidVal = a4 * 100 + a3;
    
    // 加载其他参数
    Val.SteerLeftTimeFlag = EEPROM.read(addr12);
    
    // 计算派生参数
    Val.steerLTimeAction = Val.steerLTime * 100;
    Val.steerRTimeAction = Val.steerRTime * 100;
    
    Serial.println("参数已从EEPROM加载");
  } else {
    // 使用默认值
    Val.steerAngle = 90;
    Val.motorSpeed = 128;
    Val.steerMidVal = MID;
    Val.steerLTimeAction = Val.steerLTime * 100;
    Val.steerRTimeAction = Val.steerRTime * 100;
    Serial.println("使用默认参数");
  }
}

// 遥控器控制开关函数
void toggleRemoteControl() {
  remoteEnabled = !remoteEnabled;
  Serial.print("遥控器控制: ");
  Serial.println(remoteEnabled ? "启用" : "禁用");
} 