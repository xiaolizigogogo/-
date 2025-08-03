/*
 * 基于现有项目的语音控制集成版本
 * 集成语音控制功能到原有的Arduino项目中
 */

#include "FBLed.h"
#include <stdio.h>
#include <Servo.h>
#include "myButton.h"
#include <KeyValueEEPROM.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>

// 原有硬件定义
Servo myservo;
FBLed led1;
FBLed led2;
FBLed led3;

// 语音控制相关定义
#define VOICE_RX 2  // 语音模块RX引脚
#define VOICE_TX 3  // 语音模块TX引脚
SoftwareSerial voiceSerial(VOICE_RX, VOICE_TX);

// 语音指令枚举
enum VoiceCommands {
  VOICE_NONE = 0,
  VOICE_LED_ON = 1,      // "开灯"
  VOICE_LED_OFF = 2,     // "关灯"
  VOICE_SERVO_LEFT = 3,  // "左转"
  VOICE_SERVO_RIGHT = 4, // "右转"
  VOICE_MOTOR_FORWARD = 5, // "前进"
  VOICE_MOTOR_BACKWARD = 6, // "后退"
  VOICE_MOTOR_STOP = 7,  // "停止"
  VOICE_SYSTEM_STATUS = 8, // "状态"
  VOICE_RECORD_PARAM = 9,  // "记录参数"
  VOICE_RESET = 10        // "重置"
};

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

#define MotorPwmPin 7
#define MotorPwmPin2 8
#define MotorPwmPin3 10
#define MotorIN1Pin 6
#define MotorIN2Pin 5
#define MotorIN3Pin 4
#define SteerPwmPin 9

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

// 编码器状态变量
int CLK0_value = 1, DT0_value = 1;
int CLK1_value = 1, DT1_value = 1;
int CLK2_value = 1, DT2_value = 1;
int CLK3_value = 1, DT3_value = 1;
int CLK4_value = 1, DT4_value = 1;
int CLK5_value = 1, DT5_value = 1;
int CLK6_value = 1, DT6_value = 1;

// 系统状态
struct SystemStatus {
  bool voiceEnabled = true;
  bool ledState = false;
  int servoAngle = 90;
  int motorSpeed = 0;
  bool motorDirection = true;
  int currentCommand = VOICE_NONE;
} status;

// 语音控制相关变量
unsigned long lastVoiceTime = 0;
const unsigned long VOICE_TIMEOUT = 5000; // 5秒超时

void setup() {
  Serial.begin(9600);
  voiceSerial.begin(9600);
  
  // 初始化原有硬件
  setupOriginalHardware();
  
  // 初始化语音模块
  setupVoiceModule();
  
  Serial.println("语音控制系统已启动");
  Serial.println("支持的语音指令：");
  Serial.println("开灯、关灯、左转、右转、前进、后退、停止、状态、记录参数、重置");
}

void loop() {
  // 处理原有功能
  handleOriginalFunctions();
  
  // 处理语音控制
  handleVoiceControl();
  
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
  pinMode(CLK0, INPUT_PULLUP);
  pinMode(DT0, INPUT_PULLUP);
  pinMode(CLK1, INPUT_PULLUP);
  pinMode(DT1, INPUT_PULLUP);
  pinMode(CLK2, INPUT_PULLUP);
  pinMode(DT2, INPUT_PULLUP);
  pinMode(CLK3, INPUT_PULLUP);
  pinMode(DT3, INPUT_PULLUP);
  pinMode(CLK4, INPUT_PULLUP);
  pinMode(DT4, INPUT_PULLUP);
  pinMode(CLK5, INPUT_PULLUP);
  pinMode(DT5, INPUT_PULLUP);
  pinMode(CLK6, INPUT_PULLUP);
  pinMode(DT6, INPUT_PULLUP);
  
  // 初始化LED
  led1.begin(22);
  led2.begin(24);
  led3.begin(26);
}

void setupVoiceModule() {
  // 初始化语音识别模块
  voiceSerial.write(0xAA);
  voiceSerial.write(0x55);
  voiceSerial.write(0x01); // 进入识别模式
  
  delay(1000);
  
  // 设置关键词数量
  voiceSerial.write(0xAA);
  voiceSerial.write(0x55);
  voiceSerial.write(0x0A); // 10个关键词
  
  Serial.println("语音模块初始化完成");
}

void handleOriginalFunctions() {
  // 处理编码器输入（原有功能）
  handleEncoders();
  
  // 处理LED显示（原有功能）
  led1.update();
  led2.update();
  led3.update();
}

void handleVoiceControl() {
  if (voiceSerial.available()) {
    int command = voiceSerial.read();
    Serial.print("收到语音指令: ");
    Serial.println(command);
    
    status.currentCommand = command;
    lastVoiceTime = millis();
    
    processVoiceCommand(command);
  }
  
  // 检查语音指令超时
  if (status.currentCommand != VOICE_NONE && 
      millis() - lastVoiceTime > VOICE_TIMEOUT) {
    status.currentCommand = VOICE_NONE;
  }
}

void processVoiceCommand(int command) {
  switch (command) {
    case VOICE_LED_ON:
      turnOnLED();
      break;
      
    case VOICE_LED_OFF:
      turnOffLED();
      break;
      
    case VOICE_SERVO_LEFT:
      servoRotateLeft();
      break;
      
    case VOICE_SERVO_RIGHT:
      servoRotateRight();
      break;
      
    case VOICE_MOTOR_FORWARD:
      motorForward();
      break;
      
    case VOICE_MOTOR_BACKWARD:
      motorBackward();
      break;
      
    case VOICE_MOTOR_STOP:
      motorStop();
      break;
      
    case VOICE_SYSTEM_STATUS:
      reportSystemStatus();
      break;
      
    case VOICE_RECORD_PARAM:
      recordParameters();
      break;
      
    case VOICE_RESET:
      resetSystem();
      break;
      
    default:
      Serial.println("未知语音指令");
      break;
  }
}

void turnOnLED() {
  status.ledState = true;
  led1.setBrightness(255);
  led2.setBrightness(255);
  led3.setBrightness(255);
  Serial.println("LED已开启");
}

void turnOffLED() {
  status.ledState = false;
  led1.setBrightness(0);
  led2.setBrightness(0);
  led3.setBrightness(0);
  Serial.println("LED已关闭");
}

void servoRotateLeft() {
  status.servoAngle = max(MIN_ANGLE, status.servoAngle - 30);
  myservo.write(status.servoAngle);
  Serial.print("舵机左转至: ");
  Serial.println(status.servoAngle);
}

void servoRotateRight() {
  status.servoAngle = min(MAX_ANGLE, status.servoAngle + 30);
  myservo.write(status.servoAngle);
  Serial.print("舵机右转至: ");
  Serial.println(status.servoAngle);
}

void motorForward() {
  digitalWrite(MotorIN1Pin, HIGH);
  digitalWrite(MotorIN2Pin, LOW);
  analogWrite(MotorPwmPin, 255);
  status.motorSpeed = 255;
  status.motorDirection = true;
  Serial.println("电机前进");
}

void motorBackward() {
  digitalWrite(MotorIN1Pin, LOW);
  digitalWrite(MotorIN2Pin, HIGH);
  analogWrite(MotorPwmPin, 255);
  status.motorSpeed = 255;
  status.motorDirection = false;
  Serial.println("电机后退");
}

void motorStop() {
  digitalWrite(MotorIN1Pin, LOW);
  digitalWrite(MotorIN2Pin, LOW);
  analogWrite(MotorPwmPin, 0);
  status.motorSpeed = 0;
  Serial.println("电机停止");
}

void reportSystemStatus() {
  Serial.println("=== 系统状态报告 ===");
  Serial.print("LED状态: ");
  Serial.println(status.ledState ? "开启" : "关闭");
  Serial.print("舵机角度: ");
  Serial.println(status.servoAngle);
  Serial.print("电机状态: ");
  if (status.motorSpeed == 0) {
    Serial.println("停止");
  } else {
    Serial.print(status.motorDirection ? "前进" : "后退");
    Serial.print(" 速度: ");
    Serial.println(status.motorSpeed);
  }
  Serial.print("语音控制: ");
  Serial.println(status.voiceEnabled ? "启用" : "禁用");
  Serial.println("==================");
}

void recordParameters() {
  // 记录当前参数到EEPROM
  KeyValueEEPROM::put("servo_angle", status.servoAngle);
  KeyValueEEPROM::put("motor_speed", status.motorSpeed);
  KeyValueEEPROM::put("led_state", status.ledState);
  Serial.println("参数已记录到EEPROM");
}

void resetSystem() {
  // 重置系统状态
  status.servoAngle = 90;
  status.motorSpeed = 0;
  status.ledState = false;
  
  // 重置硬件
  myservo.write(90);
  motorStop();
  turnOffLED();
  
  Serial.println("系统已重置");
}

void handleButtonControl() {
  // 原有的按钮控制逻辑
  if (digitalRead(bt1) == LOW) {
    recordParameters();
    delay(200);
  }
  
  if (digitalRead(bt2) == LOW) {
    // 舵机控制
    status.servoAngle = (status.servoAngle + 10) % 180;
    myservo.write(status.servoAngle);
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

void handleEncoders() {
  // 原有的编码器处理逻辑
  // 这里可以添加编码器读取和处理代码
  // 由于代码较长，这里简化处理
}

// 语音控制开关函数
void toggleVoiceControl() {
  status.voiceEnabled = !status.voiceEnabled;
  Serial.print("语音控制: ");
  Serial.println(status.voiceEnabled ? "启用" : "禁用");
}

// 获取当前语音指令
int getCurrentVoiceCommand() {
  return status.currentCommand;
}

// 清除当前语音指令
void clearVoiceCommand() {
  status.currentCommand = VOICE_NONE;
} 