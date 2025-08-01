/*
 * 中断扩展方案 - MCP23017 I/O扩展器
 * 使用MCP23017扩展16个输入引脚，通过1个中断处理所有输入变化
 * 成本：8-15元，扩展能力：16个输入 → 1个中断
 */

#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include "FBLed.h"
#include <Servo.h>
#include "myButton.h"
#include <KeyValueEEPROM.h>
#include <EEPROM.h>

// 硬件定义
Servo myservo;
FBLed led1;
FBLed led2;
FBLed led3;
Adafruit_MCP23017 mcp;

// 系统常量定义
#define MIN_ANGLE 0
#define MAX_ANGLE 180
#define MID 1550
#define MIN_MID 1100
#define MAX_MID 2000
#define ENCODER_COUNT 7

// 按键引脚定义 (使用MCP23017)
#define MCP_BT1 0   // record param
#define MCP_BT2 1   // steer sw
#define MCP_BT3 2   // 
#define MCP_BT4 3   // 
#define MCP_BT5 4   // motor sw
#define MCP_BT6 5   // motor sw
#define MCP_BT7 6   // motor sw

// 编码器引脚定义 (使用MCP23017)
#define MCP_CLK0 8
#define MCP_DT0  9
#define MCP_CLK1 10
#define MCP_DT1  11
#define MCP_CLK2 12
#define MCP_DT2  13
#define MCP_CLK3 14
#define MCP_DT3  15

// 电机引脚定义 (Arduino Mega)
#define MotorPwmPin 7
#define MotorPwmPin2 8
#define MotorPwmPin3 10
#define MotorIN1Pin 6
#define MotorIN2Pin 5
#define MotorIN3Pin 4
#define SteerPwmPin 9

// 中断引脚 (Arduino Mega)
#define MCP_INTERRUPT_PIN 2

float k = 0.25; // 时长系数

// 编码器结构体
typedef struct {
  int clkPin;
  int dtPin;
  int clkValue;
  int dtValue;
  int count;
  int maxValue;
  int minValue;
  bool isEnabled;
} Encoder_t;

// 编码器数组 (使用MCP23017引脚)
Encoder_t encoders[ENCODER_COUNT] = {
  {MCP_CLK0, MCP_DT0, 1, 1, 0, 99 * 2, 0, true},      // 舵机摆动幅度
  {MCP_CLK1, MCP_DT1, 1, 1, 0, 99 * 4, 0, true},      // 舵机摆动速度
  {MCP_CLK2, MCP_DT2, 1, 1, 0, 99 * 2, 0, true},      // 舵机停留时间
  {MCP_CLK3, MCP_DT3, 1, 1, 0, 900, 0, true},         // 舵机中位点
  {MCP_CLK0, MCP_DT0, 1, 1, 0, 99 * 2, 0, true},      // 电机1速度 (复用引脚)
  {MCP_CLK1, MCP_DT1, 1, 1, 0, 99 * 2, 0, true},      // 电机2速度 (复用引脚)
  {MCP_CLK2, MCP_DT2, 1, 1, 0, 99 * 2, 0, true}       // 电机3速度 (复用引脚)
};

// 电机控制结构体
typedef struct {
  int pwmPin;
  int in1Pin;
  int in2Pin;
  int in3Pin;
  bool isEnabled;
  int speed;
  int flag;
} Motor_t;

// 电机数组
Motor_t motors[3] = {
  {MotorPwmPin, MotorIN1Pin, MotorIN2Pin, MotorIN3Pin, false, 0, 0},
  {MotorPwmPin2, MotorIN1Pin, MotorIN2Pin, MotorIN3Pin, false, 0, 0},
  {MotorPwmPin3, MotorIN1Pin, MotorIN2Pin, MotorIN3Pin, false, 0, 0}
};

// 系统参数结构体
typedef struct {
  bool motorSw;
  bool steerSw;
  int SteerLeftTimeFlag;
  int motorSwFlag;
  int motorSpeed;
  bool motorSw5;
  int motorSwFlag5;
  int motorSpeed5;
  bool motorSw6;
  int motorSwFlag6;
  int motorSpeed6;
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
} param_t;

// 舵机控制状态枚举
typedef enum {
  LEFT_STOP = 0,
  RIGHT_TURN,
  RIGHT_STOP,
  LEFT_TURN,
} steerState_t;

// 全局变量
param_t Val;
steerState_t steerState;
char ledBuf[8];

// 中断相关变量
volatile bool mcpInterruptFlag = false;
volatile uint16_t lastMCPState = 0;
volatile unsigned long lastInterruptTime = 0;
const unsigned long INTERRUPT_DEBOUNCE = 5; // 5ms防抖

// MCP23017中断处理函数
void mcpInterruptHandler() {
  unsigned long currentTime = millis();
  
  // 防抖处理
  if (currentTime - lastInterruptTime < INTERRUPT_DEBOUNCE) {
    return;
  }
  
  lastInterruptTime = currentTime;
  mcpInterruptFlag = true;
}

// 处理MCP23017输入变化
void processMCPInputs() {
  if (!mcpInterruptFlag) return;
  
  // 读取当前状态
  uint16_t currentState = mcp.readGPIOAB();
  uint16_t stateChange = currentState ^ lastMCPState;
  
  if (stateChange == 0) {
    mcpInterruptFlag = false;
    return;
  }
  
  // 处理编码器变化
  processEncoderChanges(stateChange, currentState);
  
  // 处理按钮变化
  processButtonChanges(stateChange, currentState);
  
  // 更新状态
  lastMCPState = currentState;
  mcpInterruptFlag = false;
}

// 处理编码器变化
void processEncoderChanges(uint16_t stateChange, uint16_t currentState) {
  // 检查每个编码器的CLK引脚变化
  for (int i = 0; i < ENCODER_COUNT; i++) {
    if (!encoders[i].isEnabled) continue;
    
    int clkBit = 1 << encoders[i].clkPin;
    if (stateChange & clkBit) {
      // CLK引脚发生变化，处理编码器
      handleEncoderChange(i, currentState);
    }
  }
}

// 处理编码器变化
void handleEncoderChange(int encoderIndex, uint16_t currentState) {
  Encoder_t* encoder = &encoders[encoderIndex];
  
  // 读取当前CLK和DT状态
  int clkValue = (currentState >> encoder->clkPin) & 1;
  int dtValue = (currentState >> encoder->dtPin) & 1;
  
  // 去重处理
  if (clkValue == encoder->clkValue && dtValue == encoder->dtValue) {
    return;
  }
  
  // 更新计数值
  if (clkValue == dtValue) {
    encoder->count++;
    if (encoder->count > encoder->maxValue) {
      encoder->count = encoder->maxValue;
    }
  } else {
    encoder->count--;
    if (encoder->count < encoder->minValue) {
      encoder->count = encoder->minValue;
    }
  }
  
  // 更新状态
  encoder->clkValue = clkValue;
  encoder->dtValue = dtValue;
  
  // 打印调试信息
  Serial.print("编码器");
  Serial.print(encoderIndex);
  Serial.print(": count=");
  Serial.println(encoder->count);
}

// 处理按钮变化
void processButtonChanges(uint16_t stateChange, uint16_t currentState) {
  // 检查按钮按下 (低电平有效)
  uint8_t buttonPins[] = {MCP_BT1, MCP_BT2, MCP_BT3, MCP_BT4, MCP_BT5, MCP_BT6, MCP_BT7};
  
  for (int i = 0; i < 7; i++) {
    int buttonBit = 1 << buttonPins[i];
    if (stateChange & buttonBit) {
      // 按钮状态发生变化
      bool buttonPressed = !((currentState >> buttonPins[i]) & 1);
      
      if (buttonPressed) {
        handleButtonPress(i);
      }
    }
  }
}

// 处理按钮按下
void handleButtonPress(int buttonIndex) {
  Serial.print("按钮");
  Serial.print(buttonIndex + 1);
  Serial.println("按下");
  
  switch (buttonIndex) {
    case 0: // BT1 - 记录参数
      recordParameters();
      break;
      
    case 1: // BT2 - 舵机控制
      Val.steerAngle_show = (Val.steerAngle_show + 10) % 99;
      Val.steerAngle = map(Val.steerAngle_show, 0, 99, MIN_ANGLE, MAX_ANGLE);
      break;
      
    case 4: // BT5 - 电机前进
      Val.motorSwFlag = 1;
      break;
      
    case 5: // BT6 - 电机停止
      Val.motorSwFlag = 0;
      break;
      
    default:
      break;
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("中断扩展系统启动...");
  
  // 初始化I2C
  Wire.begin();
  
  // 初始化MCP23017
  mcp.begin();
  
  // 配置MCP23017引脚
  setupMCP23017();
  
  // 初始化硬件
  setupHardware();
  
  // 初始化编码器
  CoderInit();
  
  Serial.println("中断扩展系统初始化完成");
}

void setupMCP23017() {
  // 配置按钮引脚为输入，启用内部上拉电阻
  for (int i = 0; i < 7; i++) {
    mcp.pinMode(i, INPUT_PULLUP);
  }
  
  // 配置编码器引脚为输入，启用内部上拉电阻
  for (int i = 8; i < 16; i++) {
    mcp.pinMode(i, INPUT_PULLUP);
  }
  
  // 配置中断
  mcp.setupInterrupts(true, false, LOW);
  
  // 读取初始状态
  lastMCPState = mcp.readGPIOAB();
  
  // 配置Arduino中断引脚
  pinMode(MCP_INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(MCP_INTERRUPT_PIN), mcpInterruptHandler, FALLING);
  
  Serial.println("MCP23017配置完成");
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
  
  // 初始化LED
  led1.begin(22);
  led2.begin(24);
  led3.begin(26);
  
  Serial.println("硬件初始化完成");
}

void CoderInit(void) {
  // 读取MCP23017的初始状态
  uint16_t initialState = mcp.readGPIOAB();
  
  // 初始化编码器状态
  for (int i = 0; i < ENCODER_COUNT; i++) {
    encoders[i].clkValue = (initialState >> encoders[i].clkPin) & 1;
    encoders[i].dtValue = (initialState >> encoders[i].dtPin) & 1;
  }
  
  Serial.println("编码器初始化完成");
}

void loop() {
  // 处理MCP23017中断
  processMCPInputs();
  
  // 更新系统参数
  updateSystemParameters();
  
  // 控制电机
  MotorLoop();
  
  // 控制舵机
  SteerLoop();
  
  // 更新LED
  led1.update();
  led2.update();
  led3.update();
  
  delay(10);
}

void updateSystemParameters() {
  // 更新舵机参数
  updateSteerParameters();
  
  // 更新电机参数
  updateMotorParameters();
}

void updateSteerParameters() {
  // 舵机摆动幅度
  int sensorValue = encoders[0].count;
  Val.steerAngle_show = map(sensorValue, 0, 198, 0, 99);
  Val.steerAngle = map(sensorValue, 0, 198, MIN_ANGLE, MAX_ANGLE);
  
  // 舵机摆动速度
  Val.steerSpeed = encoders[1].count / 4;
  
  // 舵机停留时间
  sensorValue = encoders[2].count;
  if (Val.SteerLeftTimeFlag == 0) {
    Val.steerLTime = map(sensorValue, 0, 198, 0, 25);
    Val.steerLTimeAction = Val.steerLTime * 100;
  } else if (Val.SteerLeftTimeFlag == 1) {
    Val.steerRTime = map(sensorValue, 0, 198, 0, 25);
    Val.steerRTimeAction = Val.steerRTime * 100;
  }
  
  // 舵机中位点
  sensorValue = encoders[3].count;
  Val.steerMidVal = sensorValue + MIN_MID;
  
  // 计算左右摆动范围
  Val.steerLVal = Val.steerMidVal - Val.steerAngle;
  Val.steerRVal = Val.steerMidVal + Val.steerAngle;
  
  // 限制范围
  Val.steerLVal = constrain(Val.steerLVal, MIN_MID, MAX_MID);
  Val.steerRVal = constrain(Val.steerRVal, MIN_MID, MAX_MID);
}

void updateMotorParameters() {
  Val.motorSpeed = encoders[4].count / 2;
  Val.motorSpeed5 = encoders[5].count / 2;
  Val.motorSpeed6 = encoders[6].count / 2;
}

void MotorLoop(void) {
  // 更新电机速度
  motors[0].speed = Val.motorSpeed;
  motors[1].speed = Val.motorSpeed5;
  motors[2].speed = Val.motorSpeed6;
  
  // 更新电机状态
  motors[0].flag = Val.motorSwFlag;
  motors[1].flag = Val.motorSwFlag5;
  motors[2].flag = Val.motorSwFlag6;
  
  // 控制所有电机
  for (int i = 0; i < 3; i++) {
    controlMotor(i);
  }
}

void controlMotor(int motorIndex) {
  if (motorIndex < 0 || motorIndex >= 3) return;
  
  Motor_t* motor = &motors[motorIndex];
  int outputValue;
  
  if (motor->flag == 1) {
    digitalWrite(motor->in1Pin, HIGH);
    digitalWrite(motor->in2Pin, LOW);
    digitalWrite(motor->in3Pin, HIGH);
    outputValue = map(motor->speed, 0, 99, 255, 0);
    analogWrite(motor->pwmPin, outputValue);
  } else {
    digitalWrite(motor->in1Pin, LOW);
    digitalWrite(motor->in2Pin, LOW);
    digitalWrite(motor->in3Pin, LOW);
    analogWrite(motor->pwmPin, 0);
  }
}

void SteerLoop(void) {
  static unsigned long _pre_time = 0;
  unsigned long _time = millis();
  
  if (_time - _pre_time >= Val.steerSpeed) {
    _pre_time = _time;
    
    switch (steerState) {
      case LEFT_STOP:
        myservo.writeMicroseconds(Val.steerLVal);
        steerState = RIGHT_TURN;
        break;
        
      case RIGHT_TURN:
        if (_time - _pre_time >= Val.steerRTimeAction) {
          steerState = RIGHT_STOP;
        }
        break;
        
      case RIGHT_STOP:
        myservo.writeMicroseconds(Val.steerRVal);
        steerState = LEFT_TURN;
        break;
        
      case LEFT_TURN:
        if (_time - _pre_time >= Val.steerLTimeAction) {
          steerState = LEFT_STOP;
        }
        break;
    }
  }
}

void recordParameters() {
  // 记录当前参数到EEPROM
  KeyValueEEPROM::put("servo_angle", Val.steerAngle_show);
  KeyValueEEPROM::put("motor_speed", Val.motorSpeed);
  KeyValueEEPROM::put("steer_speed", Val.steerSpeed);
  Serial.println("参数已记录到EEPROM");
}

// 调试函数：打印MCP23017状态
void printMCPStatus() {
  uint16_t state = mcp.readGPIOAB();
  Serial.print("MCP23017状态: 0x");
  Serial.println(state, HEX);
  
  // 打印按钮状态
  Serial.print("按钮状态: ");
  for (int i = 0; i < 7; i++) {
    bool pressed = !((state >> i) & 1);
    Serial.print(pressed ? "1" : "0");
  }
  Serial.println();
  
  // 打印编码器状态
  Serial.print("编码器状态: ");
  for (int i = 8; i < 16; i++) {
    bool high = (state >> i) & 1;
    Serial.print(high ? "1" : "0");
  }
  Serial.println();
} 