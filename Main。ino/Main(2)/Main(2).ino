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
#define MAX_ANGLE  300


//这里对转动的数值进行修改 进行角度变换
#define MID 1550
#define MIN_MID  1100
#define MAX_MID 2000

#define bt1 31  // record param
#define bt2 33  // steer sw
#define bt3 35  //
#define bt4 37
#define bt5 39 // motor sw

#define now_count 0
#define total_count 100*k



#define MotorPwmPin 7  // 电机pwm输出引脚
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

int CLK3_value= 1;
int DT3_value= 1;

int CLK1_value= 1;
int DT1_value= 1;
int CLK0_value= 1;
int DT0_value= 1;
int CLK2_value= 1;
int DT2_value= 1;
int CLK4_value= 1;
int DT4_value= 1;
// 5个编码器的脉冲技术数值
int ecCnt[5];
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
int addr11 =10;
int addr111 =11;
int addr12 =12;
int addr121 =13;
int addr13 =14;
int addr131 =15;
int addr14 =16;
int addr141 =17;
int addr15 =18;
int addr151 =19;
int addr16 =20;
int addr161 =21;
int addr17 =22;
int addr18 =23;

// 系统相关的参数表
typedef struct
{
  bool motorSw;
  bool steerSw;

  int SteerLeftTimeFlag;
  int motorSwFlag;

  int motorSpeed;
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

//编码器初始化
void CoderInit(void)
{
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

  attachInterrupt(0, ClockChanged0, CHANGE);//设置中断0的处理函数，电平变化触发
  attachInterrupt(1, ClockChanged1, CHANGE);//设置中断0的处理函数，电平变化触发
  attachInterrupt(2, ClockChanged2, CHANGE);//设置中断0的处理函数，电平变化触发
  attachInterrupt(3, ClockChanged3, CHANGE);//设置中断0的处理函数，电平变化触发
  attachInterrupt(4, ClockChanged4, CHANGE);//设置中断0的处理函数，电平变化触发

  //memset(ecCnt,0,5);
}




//按键初始化
void BtInit(void)
{
  myBt1.BindBtnPress([]() {
    EEPROM.write(addr1, Val.steerAngle);
    EEPROM.write(addr2, Val.steerSpeed);
    EEPROM.write(addr3, Val.steerLTime);
    EEPROM.write(addr4, Val.steerRTime);
    EEPROM.write(addr5, Val.motorSpeed);
    EEPROM.write(addr6, ecCnt[0]);
    EEPROM.write(addr7, ecCnt[1]);
    EEPROM.write(addr8, ecCnt[2]);
    EEPROM.write(addr9, ecCnt[4]);
    int a1 = ecCnt[3]%100;
    int a2 = ecCnt[3]/100;
     EEPROM.write(addr10, a1);
    EEPROM.write(addr101, a2);
  int a3 = Val.steerMidVal % 100;
  int a4 = Val.steerMidVal  /100;
      EEPROM.write(addr11, a3);
    EEPROM.write(addr111, a4);
    EEPROM.write(addr12, Val.SteerLeftTimeFlag);
     Serial.println( EEPROM.read(addr1));
  Serial.println( EEPROM.read(addr2));
  Serial.println( EEPROM.read(addr3));
  Serial.println( EEPROM.read(addr4));
  Serial.println( EEPROM.read(addr5));
  Serial.println( EEPROM.read(addr6));
  Serial.println( EEPROM.read(addr7));
  Serial.println( EEPROM.read(addr8));
  Serial.println( EEPROM.read(addr9));
  Serial.println( EEPROM.read(addr10));
  Serial.println( EEPROM.read(addr101));
  Serial.println( EEPROM.read(addr11));
  Serial.println( EEPROM.read(addr111));
  Serial.println( EEPROM.read(200));

//     EEPROM.write(addr13, Val.steerInterval);

//     EEPROM.write(addr16, Val.steerLTimeAction);
//     EEPROM.write(addr17, Val.steerRTime);
          EEPROM.write(addr18, Val.steerAngle_show);

    EEPROM.write(200, 1);
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
    } else{
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
}


//按键循环查询 以及 系统参数更新
void BtLoop(void)
{
  static unsigned long _pre_time = 0;
  unsigned long _time = millis();
  if (_time - _pre_time >= 100) //10ms更新一次
  {
    int sensorValue;
    int outputValue;
    _pre_time = _time;

    myBt1.loop();
    myBt2.loop();
    myBt3.loop();
    myBt4.loop();
    myBt5.loop();
    sensorValue = ecCnt[0];
    Val.steerAngle_show = map(sensorValue, 0, 198, 0, 99);
    //摆幅数值映射


    Val.steerAngle = map(sensorValue, 0, 198, MIN_ANGLE, MAX_ANGLE);
    //摆动速度是加速度除以4
    Val.steerSpeed = ecCnt[1] / 4;

    if (Val.SteerLeftTimeFlag == 0)
    {
      sensorValue = ecCnt[2];
      Val.steerLTime = map(sensorValue, 0, 198, 0, 25);
      Val.steerLTimeAction = Val.steerLTime * 100;
    }
    else if (Val.SteerLeftTimeFlag == 1)
    {
      sensorValue = ecCnt[2];
      Val.steerRTime = map(sensorValue, 0, 198
                           , 0, 25);
      Val.steerRTimeAction = Val.steerRTime * 100;
    }
    //        Serial.print("左：");
    //        Serial.print(Val.steerLTimeAction);
    //        Serial.print("右:");
    //        Serial.println(Val.steerRTimeAction);
    sensorValue = ecCnt[3];

    //中位点位置是旋钮值除以2+最小值

    Val.steerMidVal = sensorValue + MIN_MID;

    Val.steerLVal = Val.steerMidVal - Val.steerAngle;
    Val.steerRVal = Val.steerMidVal + Val.steerAngle;

    if (Val.steerLVal <= MIN_MID) {
      Val.steerLVal = MIN_MID;
    }
    if (Val.steerRVal >= MAX_MID) {
      Val.steerRVal = MAX_MID;
    }
    //电机速度
    Val.motorSpeed = ecCnt[4] / 2;
  }

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
  int sensorValue;
  memset(ledBuf, 0, sizeof(ledBuf));

  sprintf(ledBuf, "%2d%2d", Val.steerAngle_show, Val.steerSpeed);
  led2.ledShow(ledBuf);

  memset(ledBuf, 0, sizeof(ledBuf));
  sprintf(ledBuf, "%1d.%1d%1d.%1d", Val.steerLTime / 10, Val.steerLTime % 10, Val.steerRTime / 10, Val.steerRTime % 10);
  led1.ledShow(ledBuf);

  memset(ledBuf, 0, sizeof(ledBuf));
  sprintf(ledBuf, "%2d", Val.motorSpeed);
  led3.ledShow(ledBuf);
}

// 电机初始化
void MotorInit(void)
{
  Val.motorSw = false;
  Val.motorSwFlag = 0;
  pinMode(MotorPwmPin, OUTPUT);
  pinMode(MotorIN1Pin, OUTPUT);
  pinMode(MotorIN2Pin, OUTPUT);
  pinMode(MotorIN3Pin, OUTPUT);
}


//电机循环控制
void MotorLoop(void)
{
  int outputValue;
  if (Val.motorSwFlag == 1)
  {
    digitalWrite(MotorIN1Pin, HIGH);
    digitalWrite(MotorIN2Pin, LOW);
    digitalWrite(MotorIN3Pin, HIGH);
    outputValue = map(Val.motorSpeed, 0, 99, 255, 0);
    analogWrite(MotorPwmPin, outputValue); // 500Hz
  }
  else
  {
    digitalWrite(MotorIN1Pin, LOW);
    digitalWrite(MotorIN2Pin, LOW);
    digitalWrite(MotorIN3Pin, LOW);
    analogWrite(MotorPwmPin, outputValue); // 500Hz
  }
}


//舵机初始化
void SteerInit(void)
{
  float sensorValue;
  Val.steerSw = false;
  myservo.attach(SteerPwmPin);
  Val.SteerLeftTimeFlag = 0;

  Serial.println( EEPROM.read(addr1));
  Serial.println( EEPROM.read(addr2));
  Serial.println( EEPROM.read(addr3));
  Serial.println( EEPROM.read(addr4));
  Serial.println( EEPROM.read(addr5));
  Serial.println( EEPROM.read(addr6));
  Serial.println( EEPROM.read(addr7));
  Serial.println( EEPROM.read(addr8));
  Serial.println( EEPROM.read(addr9));
  Serial.println( EEPROM.read(addr10));
  Serial.println( EEPROM.read(11));
  Serial.println( EEPROM.read(12));
  Serial.println( EEPROM.read(13));
  Serial.println( EEPROM.read(14));
  Serial.println( EEPROM.read(200));
  if (EEPROM.read(200) != 0)
  {
    Val.steerAngle = EEPROM.read(addr1);
    Val.steerSpeed = EEPROM.read(addr2);
    Val.steerLTime = EEPROM.read(addr3);
    Val.steerRTime = EEPROM.read(addr4);
    Val.motorSpeed = EEPROM.read(addr5);
    int a3 = EEPROM.read(addr11);
    int a4 = EEPROM.read(addr111);
    Val.steerMidVal = a4*100+a3;
//    Val.steerNowVal= EEPROM.read(addr12);
//    Val.steerInterval= EEPROM.read(addr13);
//    Val.steerLVal= EEPROM.read(addr14);
//    Val.steerRVal= EEPROM.read(addr15);
    Val.steerLTimeAction= Val.steerLTime*100;
    Val.steerRTimeAction= Val.steerRTime*100;
    Val.steerAngle_show= EEPROM.read(addr18);
    ecCnt[0] = EEPROM.read(addr6);
    ecCnt[1] = EEPROM.read(addr7);
    ecCnt[2] = EEPROM.read(addr8);
    ecCnt[4] = EEPROM.read(addr9);
    int a1 = EEPROM.read(addr10);
    int a2 =  EEPROM.read(addr101);
     ecCnt[3] = a2*100+a1;
     Val.SteerLeftTimeFlag = EEPROM.read(addr12);
    
     Serial.print("a1:");
     Serial.println(a1);
      Serial.print("a2:");
     Serial.println(a2);
     Serial.println(ecCnt[3]);
     Serial.println(Val.steerNowVal);
     Serial.println(Val.steerMidVal);
  }else{
     Val.steerNowVal = MID;
       Val.steerLVal = MID - Val.steerAngle;
  Val.steerRVal = MID + Val.steerAngle;
  //停留时间
  Val.steerLTimeAction = Val.steerLTime * 100;
  Val.steerRTimeAction = Val.steerRTime * 100;
    }

}


//舵机循环控制
void SteerLoop(void)
{
  static unsigned long preTime = 0;
  static unsigned long lastSTime = 0;
  unsigned long nowTime = millis();
  if (nowTime - preTime >= 20)
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



//中断处理函数
void ClockChanged0()
{
  int clkValue = digitalRead(CLK0);//读取CLK引脚的电平
  int dtValue = digitalRead(DT0);//读取DT引脚的电平
  if(clkValue==CLK0_value&&DT0_value==dtValue){
    //去重
  }
  else if (clkValue == dtValue)
  {
    ecCnt[0]++;
    if (ecCnt[0] > 99 * 2)
    {
      ecCnt[0] = 99 * 2;
    }
  }
  else
  {
    ecCnt[0]--;
    if (ecCnt[0] <= 0)
    {
      ecCnt[0] = 0;
    }
  }
    CLK0_value=clkValue;
    DT0_value=dtValue;
//  Serial.print("0 -- ");
//  Serial.println(ecCnt[0]);

}


void ClockChanged1()
{
  int clkValue = digitalRead(CLK1);//读取CLK引脚的电平
  int dtValue = digitalRead(DT1);//读取DT引脚的电平
   if(clkValue==CLK1_value&&DT1_value==dtValue){
    //去重
  }
  else if (clkValue == dtValue)
  {
    ecCnt[1]++;
    if (ecCnt[1] > 99 * 4)
    {
      ecCnt[1] = 99 * 4;
    }
  }
  else
  {
    ecCnt[1]--;
    if (ecCnt[1] <= 0)
    {
      ecCnt[1] = 0;
    }
  }
    CLK1_value=clkValue;
    DT1_value=dtValue;
  
//  Serial.print("1 -- ");
//  Serial.println(ecCnt[1]);
}


void ClockChanged2()
{
  int clkValue = digitalRead(CLK2);//读取CLK引脚的电平
  int dtValue = digitalRead(DT2);//读取DT引脚的电平
 if(clkValue==CLK2_value&&DT2_value==dtValue){
    //去重
  }
  else if (clkValue == dtValue)
  {
    ecCnt[2]++;
    if (ecCnt[2] > 99 * 2)
    {
      ecCnt[2] = 99 * 2;
    }
  }
  else
  {
    ecCnt[2]--;
    if (ecCnt[2] <= 0)
    {
      ecCnt[2] = 0;
    }
  }
    CLK2_value=clkValue;
    DT2_value=dtValue;
//  Serial.print("2 -- ");
//  Serial.println(ecCnt[2]);

}

void ClockChanged3()
{
  int clkValue = digitalRead(CLK3);//读取CLK引脚的电平
  int dtValue = digitalRead(DT3);//读取DT引脚的电平
  if(clkValue==CLK3_value&&DT3_value==dtValue){
    //去重
  }
  else if (clkValue == dtValue)
    {
    ecCnt[3]++;
    if (ecCnt[3] > 900 )
    {
      ecCnt[3] = 900 ;
    }
    }
    else
    {
    ecCnt[3]--;
    if (ecCnt[3] <= 0)
    {
      ecCnt[3] = 0;
    }
    }
    CLK3_value=clkValue;
    DT3_value=dtValue;
  

}

void ClockChanged4()
{
  int clkValue = digitalRead(CLK4);//读取CLK引脚的电平
  int dtValue = digitalRead(DT4);//读取DT引脚的电平
  if(clkValue==CLK4_value&&DT4_value==dtValue){
    //去重
  }
  else if (clkValue == dtValue)
  {
    ecCnt[4]++;
    if (ecCnt[4] > 99 * 2)
    {
      ecCnt[4] = 99 * 2;
    }
  }
  else
  {
    ecCnt[4]--;
    if (ecCnt[4] <= 0)
    {
      ecCnt[4] = 0;
    }
  }
    CLK4_value=clkValue;
    DT4_value=dtValue;
//  Serial.print("4 -- ");
//  Serial.println(ecCnt[4]);
}