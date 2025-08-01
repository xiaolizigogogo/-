/*
 * 多设备焊接环境智能控制系统
 * 支持设备ID隔离，避免信号冲突
 * 适合多设备同时工作的焊接环境
 */

#include "FBLed.h"
#include "DeviceConfig.h"
#include <stdio.h>
#include <Servo.h>
#include "myButton.h"
#include <KeyValueEEPROM.h>
#include <EEPROM.h>
#include <RCSwitch.h>

// 硬件定义
Servo myservo;
FBLed led1;
FBLed led2;
FBLed led3;
RCSwitch mySwitch = RCSwitch();

// 设备配置管理器
DeviceConfigManager deviceConfig;

// 系统状态
struct SystemStatus {
  bool remoteEnabled = true;
  bool ledState = false;
  int servoAngle = 90;
  int motorSpeed = 0;
  int maxMotorSpeed = 255;
  bool motorDirection = true;
  uint32_t currentCommand = 0;
  unsigned long lastCommandTime = 0;
  uint8_t deviceId = DEVICE_ID_1;
} status;

// 遥控器相关变量
unsigned long lastRemoteTime = 0;
const unsigned long REMOTE_TIMEOUT = 3000; // 3秒超时
const unsigned long REMOTE_DEBOUNCE = 200; // 200ms防抖

// 引脚定义
#define MIN_ANGLE 0
#define MAX_ANGLE 180
#define MID 1550
#define MIN_MID 1100
#define MAX_MID 2000

#define bt1 31  // record param
#define bt2 33  // steer sw
#define bt3 35
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

void setup() {
  Serial.begin(9600);
  
  // 初始化设备配置
  setupDeviceConfig();
  
  // 初始化无线接收器
  mySwitch.enableReceive(0); // 使用中断引脚0
  
  // 初始化硬件
  setupHardware();
  
  // 打印设备信息
  printDeviceInfo();
  
  Serial.println("多设备焊接环境智能控制系统已启动");
  Serial.println("支持的遥控器指令：");
  Serial.println("设备1: 按键1-4 设备2: 按键5-8 设备3: 按键9-12");
  Serial.println("通用指令: 紧急停止(按键0) 系统重置(按键*)");
}

void setupDeviceConfig() {
  // 从EEPROM读取设备ID，如果没有则使用默认值
  uint8_t savedDeviceId = KeyValueEEPROM::get("device_id", DEVICE_ID_1);
  deviceConfig.setDeviceId(savedDeviceId);
  status.deviceId = savedDeviceId;
  
  // 保存设备ID到EEPROM
  KeyValueEEPROM::put("device_id", status.deviceId);
}

void setupHardware() {
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

void printDeviceInfo() {
  Serial.println("=== 设备信息 ===");
  Serial.print("设备ID: ");
  Serial.println(status.deviceId);
  Serial.print("设备名称: ");
  Serial.println(deviceConfig.getDeviceName());
  Serial.print("工作频率: ");
  Serial.println("433MHz");
  Serial.print("遥控器控制: ");
  Serial.println(status.remoteEnabled ? "启用" : "禁用");
  Serial.println("================");
}

void loop() {
  // 处理原有功能
  handleOriginalFunctions();
  
  // 处理遥控器控制
  handleRemoteControl();
  
  // 处理按钮控制
  handleButtonControl();
  
  delay(10);
}

void handleOriginalFunctions() {
  // 处理编码器输入
  handleEncoders();
  
  // 处理LED显示
  led1.update();
  led2.update();
  led3.update();
}

void handleRemoteControl() {
  if (mySwitch.available()) {
    long value = mySwitch.getReceivedValue();
    
    // 防抖处理
    if (millis() - lastRemoteTime > REMOTE_DEBOUNCE) {
      Serial.print("收到遥控器指令: 0x");
      Serial.println(value, HEX);
      
      // 检查指令是否针对本设备
      if (deviceConfig.isCommandForThisDevice(value)) {
        Serial.print("指令针对本设备(");
        Serial.print(deviceConfig.getDeviceName());
        Serial.println(")，执行指令");
        
        status.currentCommand = value;
        lastRemoteTime = millis();
        
        processRemoteCommand(value);
      } else {
        uint8_t targetDeviceId = (value >> 16) & 0xFF;
        Serial.print("指令针对设备ID: ");
        Serial.print(targetDeviceId);
        Serial.println("，忽略指令");
      }
    }
    
    mySwitch.resetAvailable();
  }
  
  // 检查遥控器指令超时
  if (status.currentCommand != 0 && 
      millis() - lastRemoteTime > REMOTE_TIMEOUT) {
    status.currentCommand = 0;
  }
}

void processRemoteCommand(uint32_t command) {
  uint8_t commandType = deviceConfig.getCommandType(command);
  
  switch (commandType) {
    case CMD_LED_ON:
      turnOnLED();
      break;
      
    case CMD_LED_OFF:
      turnOffLED();
      break;
      
    case CMD_SERVO_LEFT:
      servoRotateLeft();
      break;
      
    case CMD_SERVO_RIGHT:
      servoRotateRight();
      break;
      
    case CMD_MOTOR_FWD:
      motorForward();
      break;
      
    case CMD_MOTOR_BWD:
      motorBackward();
      break;
      
    case CMD_MOTOR_STOP:
      motorStop();
      break;
      
    case CMD_STATUS:
      reportSystemStatus();
      break;
      
    case CMD_RECORD:
      recordParameters();
      break;
      
    case CMD_RESET:
      resetSystem();
      break;
      
    case CMD_SPEED_UP:
      increaseSpeed();
      break;
      
    case CMD_SPEED_DOWN:
      decreaseSpeed();
      break;
      
    case CMD_EMERGENCY_STOP:
      emergencyStop();
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
  Serial.print(deviceConfig.getDeviceName());
  Serial.println(" - LED已开启");
}

void turnOffLED() {
  status.ledState = false;
  led1.setBrightness(0);
  led2.setBrightness(0);
  led3.setBrightness(0);
  Serial.print(deviceConfig.getDeviceName());
  Serial.println(" - LED已关闭");
}

void servoRotateLeft() {
  status.servoAngle = max(MIN_ANGLE, status.servoAngle - 30);
  myservo.write(status.servoAngle);
  Serial.print(deviceConfig.getDeviceName());
  Serial.print(" - 舵机左转至: ");
  Serial.println(status.servoAngle);
}

void servoRotateRight() {
  status.servoAngle = min(MAX_ANGLE, status.servoAngle + 30);
  myservo.write(status.servoAngle);
  Serial.print(deviceConfig.getDeviceName());
  Serial.print(" - 舵机右转至: ");
  Serial.println(status.servoAngle);
}

void motorForward() {
  digitalWrite(MotorIN1Pin, HIGH);
  digitalWrite(MotorIN2Pin, LOW);
  analogWrite(MotorPwmPin, status.maxMotorSpeed);
  status.motorSpeed = status.maxMotorSpeed;
  status.motorDirection = true;
  Serial.print(deviceConfig.getDeviceName());
  Serial.println(" - 电机前进");
}

void motorBackward() {
  digitalWrite(MotorIN1Pin, LOW);
  digitalWrite(MotorIN2Pin, HIGH);
  analogWrite(MotorPwmPin, status.maxMotorSpeed);
  status.motorSpeed = status.maxMotorSpeed;
  status.motorDirection = false;
  Serial.print(deviceConfig.getDeviceName());
  Serial.println(" - 电机后退");
}

void motorStop() {
  digitalWrite(MotorIN1Pin, LOW);
  digitalWrite(MotorIN2Pin, LOW);
  analogWrite(MotorPwmPin, 0);
  status.motorSpeed = 0;
  Serial.print(deviceConfig.getDeviceName());
  Serial.println(" - 电机停止");
}

void emergencyStop() {
  motorStop();
  turnOffLED();
  myservo.write(90);
  Serial.print(deviceConfig.getDeviceName());
  Serial.println(" - 紧急停止执行");
}

void increaseSpeed() {
  status.maxMotorSpeed = min(255, status.maxMotorSpeed + 25);
  if (status.motorSpeed > 0) {
    analogWrite(MotorPwmPin, status.maxMotorSpeed);
    status.motorSpeed = status.maxMotorSpeed;
  }
  Serial.print(deviceConfig.getDeviceName());
  Serial.print(" - 电机速度增加至: ");
  Serial.println(status.maxMotorSpeed);
}

void decreaseSpeed() {
  status.maxMotorSpeed = max(50, status.maxMotorSpeed - 25);
  if (status.motorSpeed > 0) {
    analogWrite(MotorPwmPin, status.maxMotorSpeed);
    status.motorSpeed = status.maxMotorSpeed;
  }
  Serial.print(deviceConfig.getDeviceName());
  Serial.print(" - 电机速度减少至: ");
  Serial.println(status.maxMotorSpeed);
}

void reportSystemStatus() {
  Serial.println("=== 系统状态报告 ===");
  Serial.print("设备: ");
  Serial.println(deviceConfig.getDeviceName());
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
  Serial.print(deviceConfig.getDeviceName());
  Serial.println(" - 参数已记录到EEPROM");
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
  
  Serial.print(deviceConfig.getDeviceName());
  Serial.println(" - 系统已重置");
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
}

// 设备ID设置函数
void setDeviceId(uint8_t newDeviceId) {
  if (newDeviceId >= 1 && newDeviceId <= 255) {
    deviceConfig.setDeviceId(newDeviceId);
    status.deviceId = newDeviceId;
    KeyValueEEPROM::put("device_id", newDeviceId);
    Serial.print("设备ID已设置为: ");
    Serial.println(newDeviceId);
    printDeviceInfo();
  }
}

// 获取当前设备ID
uint8_t getCurrentDeviceId() {
  return status.deviceId;
}

// 遥控器控制开关函数
void toggleRemoteControl() {
  status.remoteEnabled = !status.remoteEnabled;
  Serial.print(deviceConfig.getDeviceName());
  Serial.print(" - 遥控器控制: ");
  Serial.println(status.remoteEnabled ? "启用" : "禁用");
} 