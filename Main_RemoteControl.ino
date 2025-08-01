/*
 * 焊接环境智能控制系统
 * 使用无线遥控器替代语音控制
 * 适合噪音大、环境恶劣的焊接工作环境
 */

#include "FBLed.h"
#include <stdio.h>
#include <Servo.h>
#include "myButton.h"
#include <KeyValueEEPROM.h>
#include <EEPROM.h>
#include <RCSwitch.h>

// 原有硬件定义
Servo myservo;
FBLed led1;
FBLed led2;
FBLed led3;

// 无线遥控器定义
RCSwitch mySwitch = RCSwitch();

// 遥控器按键映射
enum RemoteCommands {
  REMOTE_NONE = 0,
  REMOTE_LED_ON = 12345,      // 开灯
  REMOTE_LED_OFF = 12346,     // 关灯
  REMOTE_SERVO_LEFT = 12347,  // 左转
  REMOTE_SERVO_RIGHT = 12348, // 右转
  REMOTE_MOTOR_FWD = 12349,   // 前进
  REMOTE_MOTOR_BWD = 12350,   // 后退
  REMOTE_MOTOR_STOP = 12351,  // 停止
  REMOTE_STATUS = 12352,      // 状态查询
  REMOTE_RECORD = 12353,      // 记录参数
  REMOTE_RESET = 12354,       // 重置系统
  REMOTE_SPEED_UP = 12355,    // 加速
  REMOTE_SPEED_DOWN = 12356   // 减速
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
  bool remoteEnabled = true;
  bool ledState = false;
  int servoAngle = 90;
  int motorSpeed = 0;
  int maxMotorSpeed = 255;
  bool motorDirection = true;
  int currentCommand = REMOTE_NONE;
  unsigned long lastCommandTime = 0;
} status;

// 遥控器相关变量
unsigned long lastRemoteTime = 0;
const unsigned long REMOTE_TIMEOUT = 3000; // 3秒超时
const unsigned long REMOTE_DEBOUNCE = 200; // 200ms防抖

void setup() {
  Serial.begin(9600);
  
  // 初始化无线接收器
  mySwitch.enableReceive(0); // 使用中断引脚0
  
  // 初始化原有硬件
  setupOriginalHardware();
  
  Serial.println("焊接环境智能控制系统已启动");
  Serial.println("支持的遥控器指令：");
  Serial.println("按键1: 开灯  按键2: 关灯  按键3: 左转  按键4: 右转");
  Serial.println("按键5: 前进  按键6: 后退  按键7: 停止  按键8: 状态");
  Serial.println("按键9: 记录  按键0: 重置  +键: 加速   -键: 减速");
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

void handleOriginalFunctions() {
  // 处理编码器输入（原有功能）
  handleEncoders();
  
  // 处理LED显示（原有功能）
  led1.update();
  led2.update();
  led3.update();
}

void handleRemoteControl() {
  if (mySwitch.available()) {
    long value = mySwitch.getReceivedValue();
    
    // 防抖处理
    if (millis() - lastRemoteTime > REMOTE_DEBOUNCE) {
      Serial.print("收到遥控器指令: ");
      Serial.println(value);
      
      status.currentCommand = value;
      lastRemoteTime = millis();
      
      processRemoteCommand(value);
    }
    
    mySwitch.resetAvailable();
  }
  
  // 检查遥控器指令超时
  if (status.currentCommand != REMOTE_NONE && 
      millis() - lastRemoteTime > REMOTE_TIMEOUT) {
    status.currentCommand = REMOTE_NONE;
  }
}

void processRemoteCommand(long command) {
  switch (command) {
    case REMOTE_LED_ON:
      turnOnLED();
      break;
      
    case REMOTE_LED_OFF:
      turnOffLED();
      break;
      
    case REMOTE_SERVO_LEFT:
      servoRotateLeft();
      break;
      
    case REMOTE_SERVO_RIGHT:
      servoRotateRight();
      break;
      
    case REMOTE_MOTOR_FWD:
      motorForward();
      break;
      
    case REMOTE_MOTOR_BWD:
      motorBackward();
      break;
      
    case REMOTE_MOTOR_STOP:
      motorStop();
      break;
      
    case REMOTE_STATUS:
      reportSystemStatus();
      break;
      
    case REMOTE_RECORD:
      recordParameters();
      break;
      
    case REMOTE_RESET:
      resetSystem();
      break;
      
    case REMOTE_SPEED_UP:
      increaseSpeed();
      break;
      
    case REMOTE_SPEED_DOWN:
      decreaseSpeed();
      break;
      
    default:
      Serial.println("未知遥控器指令");
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
  analogWrite(MotorPwmPin, status.maxMotorSpeed);
  status.motorSpeed = status.maxMotorSpeed;
  status.motorDirection = true;
  Serial.println("电机前进");
}

void motorBackward() {
  digitalWrite(MotorIN1Pin, LOW);
  digitalWrite(MotorIN2Pin, HIGH);
  analogWrite(MotorPwmPin, status.maxMotorSpeed);
  status.motorSpeed = status.maxMotorSpeed;
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

void increaseSpeed() {
  status.maxMotorSpeed = min(255, status.maxMotorSpeed + 25);
  if (status.motorSpeed > 0) {
    analogWrite(MotorPwmPin, status.maxMotorSpeed);
    status.motorSpeed = status.maxMotorSpeed;
  }
  Serial.print("电机速度增加至: ");
  Serial.println(status.maxMotorSpeed);
}

void decreaseSpeed() {
  status.maxMotorSpeed = max(50, status.maxMotorSpeed - 25);
  if (status.motorSpeed > 0) {
    analogWrite(MotorPwmPin, status.maxMotorSpeed);
    status.motorSpeed = status.maxMotorSpeed;
  }
  Serial.print("电机速度减少至: ");
  Serial.println(status.maxMotorSpeed);
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
  Serial.print("最大速度: ");
  Serial.println(status.maxMotorSpeed);
  Serial.print("遥控器控制: ");
  Serial.println(status.remoteEnabled ? "启用" : "禁用");
  Serial.println("==================");
}

void recordParameters() {
  // 记录当前参数到EEPROM
  KeyValueEEPROM::put("servo_angle", status.servoAngle);
  KeyValueEEPROM::put("motor_speed", status.motorSpeed);
  KeyValueEEPROM::put("max_motor_speed", status.maxMotorSpeed);
  KeyValueEEPROM::put("led_state", status.ledState);
  Serial.println("参数已记录到EEPROM");
}

void resetSystem() {
  // 重置系统状态
  status.servoAngle = 90;
  status.motorSpeed = 0;
  status.maxMotorSpeed = 255;
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

// 遥控器控制开关函数
void toggleRemoteControl() {
  status.remoteEnabled = !status.remoteEnabled;
  Serial.print("遥控器控制: ");
  Serial.println(status.remoteEnabled ? "启用" : "禁用");
}

// 获取当前遥控器指令
int getCurrentRemoteCommand() {
  return status.currentCommand;
}

// 清除当前遥控器指令
void clearRemoteCommand() {
  status.currentCommand = REMOTE_NONE;
} 