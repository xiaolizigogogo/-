#include "FBLed.h"

#include <stdio.h>
#include <Servo.h>
#include "myButton.h"
#include <KeyValueEEPROM.h>
#include <EEPROM.h>
Servo myservo;

FBLed led1;
FBLed led2;
FBLed led3;


#define MIN_ANGLE 0
#define MAX_ANGLE  180


//这里对转动的数值进行修改 进行角度变换
#define MID 1550
#define MIN_MID  1100
#define MAX_MID 2000

#define bt1 31  // record param
#define bt2 33  // steer sw
#define bt3 35  //
#define bt4 37
#define bt5 39 // motor sw
//改
#define bt6 41 // motor sw
#define bt7 43 // motor sw

#define now_count 0
#define total_count 100*k



#define MotorPwmPin 7  // 电机pwm输出引脚
//改
#define MotorPwmPin2 8  // 电机pwm输出引脚
#define MotorPwmPin3 10  // 电机pwm输出引脚
#define MotorIN1Pin 6  // 电机IN1控制引脚
#define MotorIN2Pin 5  // 电机IN2控制引脚
#define MotorIN3Pin 4  // 电机IN2控制引脚

#define SteerPwmPin 9  // 舵机pwm输出引脚

//编码器 0 引脚定义
int CLK0 = 2;
int DT0 = 11;

float k = 0.25; //时长系数
//编码器 1 引脚定义
int CLK1 = 3;
int DT1 = 12;



//编码器 2 引脚定义
int CLK2 = 21;
int DT2 = 13;



//编码器 3 引脚定义
int CLK3 = 20;
int DT3 = 14;



//编码器 4 引脚定义
int CLK4 = 19;
int DT4 = 15;
//改
//编码器 5 引脚定义
int CLK5 = 18;
int DT5 = 16;

//编码器 6 引脚定义
int CLK6 = 23;
int DT6 = 17;


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
//改
int CLK5_value = 1;
int DT5_value = 1;
int CLK6_value = 1;
int DT6_value = 1;
// 编码器数量常量
#define ENCODER_COUNT 7

// 编码器引脚定义结构体
typedef struct {
  int clkPin;
  int dtPin;
  int clkValue;
  int dtValue;
  int count;
  int maxValue;
  int minValue;
} Encoder_t;

// 编码器数组
Encoder_t encoders[ENCODER_COUNT] = {
  {CLK0, DT0, 1, 1, 0, 99 * 2, 0},      // 舵机摆动幅度
  {CLK1, DT1, 1, 1, 0, 99 * 4, 0},      // 舵机摆动速度
  {CLK2, DT2, 1, 1, 0, 99 * 2, 0},      // 舵机停留时间
  {CLK3, DT3, 1, 1, 0, 900, 0},         // 舵机中位点
  {CLK4, DT4, 1, 1, 0, 99 * 2, 0},      // 电机1速度
  {CLK5, DT5, 1, 1, 0, 99 * 2, 0},      // 电机2速度
  {CLK6, DT6, 1, 1, 0, 99 * 2, 0}       // 电机3速度
};
int addr1 = 0;
int addr2 = 1;
int addr3 = 2;
int addr4 = 3;
int addr5 = 4;
int addr6 = 5;
int addr7 = 6;
int addr8 = 7;
int addr9 = 8;
int addr10 = 9;
int addr101 = 9;
int addr11 = 10;
int addr111 = 11;
int addr12 = 12;
int addr121 = 13;
int addr13 = 14;
int addr131 = 15;
int addr14 = 16;
int addr141 = 17;
int addr15 = 18;
int addr151 = 19;
int addr16 = 20;
int addr161 = 21;
int addr17 = 22;
int addr18 = 23;

// 系统相关的参数表
typedef struct
{
  bool motorSw;
  bool steerSw;

  int SteerLeftTimeFlag;
  
  bool motorSw;
  int motorSwFlag;
  int motorSpeed;
  
  bool motorSw5;
  int motorSwFlag5;
  int motorSpeed5;

  bool motorSw6;
  int motorSwFlag6;
  int motorSpeed6;
  
  int steerSpeed = 0;
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


////舵机控制状态值
typedef enum
{
  LEFT_STOP = 0,
  RIGHT_TURN,
  RIGHT_STOP,
  LEFT_TURN,
} steerStaue_t;
steerStaue_t steerStaue;

param_t Val;

char ledBuf[8];

// 按键定义
GpioButton myBt1(bt1);  // record param
GpioButton myBt2(bt2); // steer sw
GpioButton myBt3(bt3);
GpioButton myBt4(bt4);
GpioButton myBt5(bt5);// motor sw
//改
GpioButton myBt6(bt6);// motor sw
GpioButton myBt7(bt7);// motor sw

//编码器初始化
void CoderInit(void)
{
  // 初始化所有编码器引脚
  for (int i = 0; i < ENCODER_COUNT; i++) {
    pinMode(encoders[i].clkPin, INPUT_PULLUP);
    pinMode(encoders[i].dtPin, INPUT_PULLUP);
  }

  // 设置中断
  attachInterrupt(0, ClockChanged0, CHANGE);
  attachInterrupt(1, ClockChanged1, CHANGE);
  attachInterrupt(2, ClockChanged2, CHANGE);
  attachInterrupt(3, ClockChanged3, CHANGE);
  attachInterrupt(4, ClockChanged4, CHANGE);
  attachInterrupt(5, ClockChanged5, CHANGE);
  attachInterrupt(6, ClockChanged6, CHANGE);
}




//按键初始化
void BtInit(void)
{
  myBt1.BindBtnPress([]() {
    saveParameters();
  });
  myBt4.BindBtnPress([]() {
    if (Val.steerSw) {
      Val.steerSw = false;
    } else {
      Val.steerSw = true;
    }
    //    Serial.print("steer sw :");
    //    Serial.println(Val.steerSw);
  });

  myBt3.BindBtnPress([]() {

    if (Val.SteerLeftTimeFlag == 0) {
      Val.SteerLeftTimeFlag = 1;
      ecCnt[2] = map(Val.steerRTime, 0, 25, 0, 198);
    } else if (Val.SteerLeftTimeFlag == 1) {
      Val.SteerLeftTimeFlag = 2;
      ecCnt[2] = map(Val.steerLTime, 0, 25, 0, 198);
    } else if (Val.SteerLeftTimeFlag == 2) {
      Val.SteerLeftTimeFlag = 0;
    } else {
      Val.SteerLeftTimeFlag = 0;
    }

  });
  myBt5.BindBtnPress([]() {
    Val.motorSwFlag++;
    if (Val.motorSwFlag > 1)
    {
      Val.motorSwFlag = 0;
    }

    if (Val.motorSw) {
      Val.motorSw = false;
    } else {
      Val.motorSw = true;
    }
    //    Serial.print("motor swflag :");
    //    Serial.println(Val.motorSwFlag);
  });
    myBt6.BindBtnPress([]() {
    Val.motorSwFlag5++;
    if (Val.motorSwFlag5 > 1)
    {
      Val.motorSwFlag5 = 0;
    }

    if (Val.motorSw5) {
      Val.motorSw5 = false;
    } else {
      Val.motorSw5 = true;
    }
    //    Serial.print("motor swflag :");
    //    Serial.println(Val.motorSwFlag);
  });
    myBt7.BindBtnPress([]() {
    Val.motorSwFlag6++;
    if (Val.motorSwFlag6 > 1)
    {
      Val.motorSwFlag6 = 0;
    }

    if (Val.motorSw6) {
      Val.motorSw6 = false;
    } else {
      Val.motorSw6 = true;
    }
    //    Serial.print("motor swflag :");
    //    Serial.println(Val.motorSwFlag);
  });
}


//按键循环查询 以及 系统参数更新
void BtLoop(void)
{
  static unsigned long _pre_time = 0;
  unsigned long _time = millis();
  if (_time - _pre_time >= 100) //100ms更新一次
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

// 数码管输出画
void LedInit(void)
{

  led1.begin(36, 34, 32); // 初始化数码管1
  led2.begin(26, 24, 22); // 初始化数码管2
  led3.begin(46, 44, 42); // 初始化数码管3
}

//刷新显示数码管的内容
void LedShow(void)
{
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
void MotorInit(void)
{
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

//电机循环控制
void MotorLoop(void)
{
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


//舵机初始化
void SteerInit(void)
{
  Val.steerSw = false;
  myservo.attach(SteerPwmPin);
  Val.SteerLeftTimeFlag = 0;

  // 加载保存的参数
  loadParameters();
}


//舵机循环控制
void SteerLoop(void)
{
  static unsigned long preTime = 0;
  static unsigned long lastSTime = 0;
  unsigned long nowTime = millis();
  if (nowTime - preTime >= 32)
  {
    preTime = nowTime;
    if (Val.steerSw)
    {
      if (Val.steerSpeed)
      {
        //总时间
        float t = (100 - Val.steerSpeed) * 20 * k;
        //总次数
        float count = (100 - Val.steerSpeed) * k;
        //速度
        float v = Val.steerAngle / count;
        //        Serial.print("当前状态：");
        //        Serial.print(Val.steerAngle);
        //        Serial.print("总时间:");
        //        Serial.print(t);
        //        Serial.print(",");
        //        Serial.print("速度:");
        //        Serial.println(v);
        switch (steerStaue)
        {
          case LEFT_STOP:
            //如果当前时间与最后截止时间差值大于停留时间  状态变更
            if (nowTime - lastSTime >= Val.steerLTimeAction)
            {
              steerStaue = RIGHT_TURN;
            }
            break;
          //如果当前状态向左
          case LEFT_TURN:
            //当前的角度减去 速度 10=1度 20=2度  90-1=89度
            Val.steerNowVal = Val.steerNowVal - v;
            if (Val.steerNowVal <= Val.steerLVal || Val.steerNowVal >= MAX_MID || Val.steerNowVal <= MIN_MID)
            {
              //            Serial.print("当前角度:");
              //            Serial.println(Val.steerNowVal);
              //如果当前的角度 小于等于左边最大的摆幅
              //停止左转
              steerStaue = LEFT_STOP;
              //设置当前角度
              Val.steerNowVal = Val.steerLVal;
              //设置时间
              lastSTime = nowTime;
            }
            break;
          case RIGHT_STOP:
            if (nowTime - lastSTime >= Val.steerRTimeAction)
            {
              steerStaue = LEFT_TURN;
            }
            break;
          case RIGHT_TURN:
            Val.steerNowVal = Val.steerNowVal + v;

            if (Val.steerNowVal >= Val.steerRVal || Val.steerNowVal <= MIN_MID || Val.steerNowVal >= MAX_MID)
            {
              //              Serial.print("当前角度:");
              //              Serial.println(Val.steerNowVal);
              steerStaue = RIGHT_STOP;
              Val.steerNowVal = Val.steerRVal;
              lastSTime = nowTime;
            }
            break;
        }
        //输出当前角度
        myservo.writeMicroseconds(Val.steerNowVal);
      }
      else
      {
        myservo.writeMicroseconds(Val.steerMidVal);
      }
    } else
    {
      myservo.writeMicroseconds(Val.steerMidVal);
    }

    MotorLoop();
  }
}

//系统初始化
void setup()
{
  Serial.begin(115200);
  LedInit();
  BtInit();
  MotorInit();
  SteerInit();
  CoderInit();
  //  Serial.println("Init ok");

}

//系统循环处理
void loop()
{

  BtLoop();
  LedShow();
  //MotorLoop();
  SteerLoop();
}



// 统一的编码器中断处理函数
void handleEncoderInterrupt(int encoderIndex) {
  if (encoderIndex < 0 || encoderIndex >= ENCODER_COUNT) return;
  
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

//中断处理函数
void ClockChanged0() { handleEncoderInterrupt(0); }


void ClockChanged1() { handleEncoderInterrupt(1); }


void ClockChanged2() { handleEncoderInterrupt(2); }

void ClockChanged3() { handleEncoderInterrupt(3); }

void ClockChanged4() { handleEncoderInterrupt(4); }
void ClockChanged5() { handleEncoderInterrupt(5); }
void ClockChanged6() { handleEncoderInterrupt(6); }

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
  
  // 输出保存确认
  Serial.println("Parameters saved successfully!");
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
    
    Serial.println("Parameters loaded successfully!");
  } else {
    // 使用默认值
    Val.steerNowVal = MID;
    Val.steerLTimeAction = Val.steerLTime * 100;
    Val.steerRTimeAction = Val.steerRTime * 100;
    Serial.println("Using default parameters");
  }
}

