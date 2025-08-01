#include "FBLed.h"
#include <stdio.h>
#include <Servo.h>
#include "myButton.h"
#include <KeyValueEEPROM.h>
#include <EEPROM.h>
#include <PinChangeInterrupt.h>  // 添加中断库

Servo myservo;

FBLed led1;
FBLed led2;
FBLed led3;

// 系统常量定义
#define MIN_ANGLE 0
#define MAX_ANGLE 180
#define MID 1550
#define MIN_MID 1100
#define MAX_MID 2000
#define ENCODER_COUNT 7

// 按键引脚定义
#define bt1 31  // record param
#define bt2 33  // steer sw
#define bt3 35  // 
#define bt4 37
#define bt5 39  // motor sw
#define bt6 41  // motor sw
#define bt7 43  // motor sw

// 电机引脚定义
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

// 编码器数组
Encoder_t encoders[ENCODER_COUNT] = {
  {CLK0, DT0, 1, 1, 0, 99 * 2, 0, true},      // 舵机摆动幅度
  {CLK1, DT1, 1, 1, 0, 99 * 4, 0, true},      // 舵机摆动速度
  {CLK2, DT2, 1, 1, 0, 99 * 2, 0, true},      // 舵机停留时间
  {CLK3, DT3, 1, 1, 0, 900, 0, true},         // 舵机中位点
  {CLK4, DT4, 1, 1, 0, 99 * 2, 0, true},      // 电机1速度
  {CLK5, DT5, 1, 1, 0, 99 * 2, 0, true},      // 电机2速度
  {CLK6, DT6, 1, 1, 0, 99 * 2, 0, true}       // 电机3速度
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

// 按键定义
GpioButton myBt1(bt1);
GpioButton myBt2(bt2);
GpioButton myBt3(bt3);
GpioButton myBt4(bt4);
GpioButton myBt5(bt5);
GpioButton myBt6(bt6);
GpioButton myBt7(bt7);

// 编码器中断处理函数（使用PinChangeInterrupt）
void handleEncoderInterrupt(int encoderIndex) {
  if (encoderIndex < 0 || encoderIndex >= ENCODER_COUNT) return;
  if (!encoders[encoderIndex].isEnabled) return;
  
  Encoder_t* encoder = &encoders[encoderIndex];
  int clkValue = digitalRead(encoder->clkPin);
  int dtValue = digitalRead(encoder->dtPin);
  
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
}

// 编码器中断回调函数
void encoder0Interrupt() { handleEncoderInterrupt(0); }
void encoder1Interrupt() { handleEncoderInterrupt(1); }
void encoder2Interrupt() { handleEncoderInterrupt(2); }
void encoder3Interrupt() { handleEncoderInterrupt(3); }
void encoder4Interrupt() { handleEncoderInterrupt(4); }
void encoder5Interrupt() { handleEncoderInterrupt(5); }
void encoder6Interrupt() { handleEncoderInterrupt(6); }

// 编码器初始化（使用PinChangeInterrupt）
void CoderInit(void) {
  // 初始化所有编码器引脚
  for (int i = 0; i < ENCODER_COUNT; i++) {
    pinMode(encoders[i].clkPin, INPUT_PULLUP);
    pinMode(encoders[i].dtPin, INPUT_PULLUP);
  }

  // 使用PinChangeInterrupt设置中断
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(encoders[0].clkPin), encoder0Interrupt, CHANGE);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(encoders[1].clkPin), encoder1Interrupt, CHANGE);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(encoders[2].clkPin), encoder2Interrupt, CHANGE);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(encoders[3].clkPin), encoder3Interrupt, CHANGE);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(encoders[4].clkPin), encoder4Interrupt, CHANGE);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(encoders[5].clkPin), encoder5Interrupt, CHANGE);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(encoders[6].clkPin), encoder6Interrupt, CHANGE);
}

// 统一的电机控制函数
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

// 电机循环控制
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

// 更新舵机参数
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

// 更新电机参数
void updateMotorParameters() {
  Val.motorSpeed = encoders[4].count / 2;
  Val.motorSpeed5 = encoders[5].count / 2;
  Val.motorSpeed6 = encoders[6].count / 2;
}

// 按键循环查询和系统参数更新
void BtLoop(void) {
  static unsigned long _pre_time = 0;
  unsigned long _time = millis();
  if (_time - _pre_time >= 100) // 100ms更新一次
  {
    _pre_time = _time;

    // 处理所有按键
    myBt1.loop();
    myBt2.loop();
    myBt3.loop();
    myBt4.loop();
    myBt5.loop();
    myBt6.loop();
    myBt7.loop();
    
    // 更新舵机参数
    updateSteerParameters();
    
    // 更新电机参数
    updateMotorParameters();
  }
}

// 刷新显示数码管的内容
void LedShow(void) {
  // 数码管2：显示摆动幅度和速度
  snprintf(ledBuf, sizeof(ledBuf), "%2d%2d", Val.steerAngle_show, Val.steerSpeed);
  led2.ledShow(ledBuf);

  // 数码管1：显示左右停留时间
  snprintf(ledBuf, sizeof(ledBuf), "%1d.%1d%1d.%1d", 
           Val.steerLTime / 10, Val.steerLTime % 10, 
           Val.steerRTime / 10, Val.steerRTime % 10);
  led1.ledShow(ledBuf);

  // 数码管3：显示电机速度
  snprintf(ledBuf, sizeof(ledBuf), "%2d", Val.motorSpeed);
  led3.ledShow(ledBuf);
}

// 电机初始化
void MotorInit(void) {
  // 初始化电机状态
  for (int i = 0; i < 3; i++) {
    motors[i].isEnabled = false;
    motors[i].flag = 0;
    motors[i].speed = 0;
  }
  
  // 初始化电机引脚
  for (int i = 0; i < 3; i++) {
    pinMode(motors[i].pwmPin, OUTPUT);
  }
  pinMode(MotorIN1Pin, OUTPUT);
  pinMode(MotorIN2Pin, OUTPUT);
  pinMode(MotorIN3Pin, OUTPUT);
}

// 数码管初始化
void LedInit(void) {
  led1.begin(36, 34, 32); // 初始化数码管1
  led2.begin(26, 24, 22); // 初始化数码管2
  led3.begin(46, 44, 42); // 初始化数码管3
}

// 按键初始化
void BtInit(void) {
  myBt1.BindBtnPress([]() {
    // 参数保存逻辑
    Serial.println("Parameters saved!");
  });
  
  myBt4.BindBtnPress([]() {
    if (Val.steerSw) {
      Val.steerSw = false;
    } else {
      Val.steerSw = true;
    }
  });

  myBt3.BindBtnPress([]() {
    if (Val.SteerLeftTimeFlag == 0) {
      Val.SteerLeftTimeFlag = 1;
      encoders[2].count = map(Val.steerRTime, 0, 25, 0, 198);
    } else if (Val.SteerLeftTimeFlag == 1) {
      Val.SteerLeftTimeFlag = 2;
      encoders[2].count = map(Val.steerLTime, 0, 25, 0, 198);
    } else if (Val.SteerLeftTimeFlag == 2) {
      Val.SteerLeftTimeFlag = 0;
    } else {
      Val.SteerLeftTimeFlag = 0;
    }
  });
  
  myBt5.BindBtnPress([]() {
    Val.motorSwFlag++;
    if (Val.motorSwFlag > 1) {
      Val.motorSwFlag = 0;
    }
    if (Val.motorSw) {
      Val.motorSw = false;
    } else {
      Val.motorSw = true;
    }
  });
  
  myBt6.BindBtnPress([]() {
    Val.motorSwFlag5++;
    if (Val.motorSwFlag5 > 1) {
      Val.motorSwFlag5 = 0;
    }
    if (Val.motorSw5) {
      Val.motorSw5 = false;
    } else {
      Val.motorSw5 = true;
    }
  });
  
  myBt7.BindBtnPress([]() {
    Val.motorSwFlag6++;
    if (Val.motorSwFlag6 > 1) {
      Val.motorSwFlag6 = 0;
    }
    if (Val.motorSw6) {
      Val.motorSw6 = false;
    } else {
      Val.motorSw6 = true;
    }
  });
}

// 舵机初始化
void SteerInit(void) {
  Val.steerSw = false;
  myservo.attach(SteerPwmPin);
  Val.SteerLeftTimeFlag = 0;
  
  // 设置默认值
  Val.steerNowVal = MID;
  Val.steerLTimeAction = Val.steerLTime * 100;
  Val.steerRTimeAction = Val.steerRTime * 100;
}

// 舵机循环控制
void SteerLoop(void) {
  static unsigned long preTime = 0;
  static unsigned long lastSTime = 0;
  unsigned long nowTime = millis();
  
  if (nowTime - preTime >= 32) {
    preTime = nowTime;
    
    if (Val.steerSw) {
      if (Val.steerSpeed) {
        // 总时间
        float t = (100 - Val.steerSpeed) * 20 * k;
        // 总次数
        float count = (100 - Val.steerSpeed) * k;
        // 速度
        float v = Val.steerAngle / count;
        
        switch (steerState) {
          case LEFT_STOP:
            if (nowTime - lastSTime >= Val.steerLTimeAction) {
              steerState = RIGHT_TURN;
            }
            break;
            
          case LEFT_TURN:
            Val.steerNowVal = Val.steerNowVal - v;
            if (Val.steerNowVal <= Val.steerLVal || Val.steerNowVal >= MAX_MID || Val.steerNowVal <= MIN_MID) {
              steerState = LEFT_STOP;
              Val.steerNowVal = Val.steerLVal;
              lastSTime = nowTime;
            }
            break;
            
          case RIGHT_STOP:
            if (nowTime - lastSTime >= Val.steerRTimeAction) {
              steerState = LEFT_TURN;
            }
            break;
            
          case RIGHT_TURN:
            Val.steerNowVal = Val.steerNowVal + v;
            if (Val.steerNowVal >= Val.steerRVal || Val.steerNowVal <= MIN_MID || Val.steerNowVal >= MAX_MID) {
              steerState = RIGHT_STOP;
              Val.steerNowVal = Val.steerRVal;
              lastSTime = nowTime;
            }
            break;
        }
        // 输出当前角度
        myservo.writeMicroseconds(Val.steerNowVal);
      } else {
        myservo.writeMicroseconds(Val.steerMidVal);
      }
    } else {
      myservo.writeMicroseconds(Val.steerMidVal);
    }

    MotorLoop();
  }
}

// 系统初始化
void setup() {
  Serial.begin(115200);
  LedInit();
  BtInit();
  MotorInit();
  SteerInit();
  CoderInit();
  Serial.println("System initialized with PinChangeInterrupt!");
}

// 系统循环处理
void loop() {
  BtLoop();
  LedShow();
  SteerLoop();
} 