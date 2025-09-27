/*
 * 主设备 - 基于Main(2).ino的业务逻辑 + ESP32无线通信
 * 功能：控制舵机、电机、编码器，并通过WiFi向子设备同步EEPROM数据
 */

#include "FBLed.h"
#include <stdio.h>
#include <Servo.h>
#include "myButton.h"
#include <KeyValueEEPROM.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <WiFiUDP.h>
#include <ArduinoJson.h>

Servo myservo;

FBLed led1;
FBLed led2;
FBLed led3;

// 硬件引脚定义
#define MIN_ANGLE 0
#define MAX_ANGLE  300
#define MID 1550
#define MIN_MID  1100
#define MAX_MID 2000

#define bt1 31  // record param
#define bt2 33  // steer sw
#define bt3 35  //
#define bt4 37
#define bt5 39 // motor sw

#define MotorPwmPin 7  // 电机pwm输出引脚
#define MotorIN1Pin 6  // 电机IN1控制引脚
#define MotorIN2Pin 5  // 电机IN2控制引脚
#define MotorIN3Pin 4  // 电机IN2控制引脚
#define SteerPwmPin 9  // 舵机pwm输出引脚

//编码器引脚定义
int CLK0 = 2, DT0 = 11;
int CLK1 = 3, DT1 = 12;
int CLK2 = 21, DT2 = 13;
int CLK3 = 20, DT3 = 14;
int CLK4 = 19, DT20 = 15;

float k = 0.30; //时长系数

// 编码器状态变量
int CLK0_value = 1, DT0_value = 1;
int CLK1_value = 1, DT1_value = 1;
int CLK2_value = 1, DT2_value = 1;
int CLK3_value = 1, DT3_value = 1;
int CLK4_value = 1, DT4_value = 1;

// 5个编码器的脉冲计数数值
int ecCnt[5];

// EEPROM地址定义
int addr1 = 0, addr2 = 1, addr3 = 2, addr4 = 3, addr5 = 4;
int addr6 = 5, addr7 = 6, addr8 = 7, addr9 = 8, addr10 = 9;
int addr101 = 9, addr11 = 10, addr111 = 11, addr12 = 12;
int addr121 = 13, addr13 = 14, addr131 = 15, addr14 = 16;
int addr141 = 17, addr15 = 18, addr151 = 19, addr16 = 20;
int addr161 = 21, addr17 = 22, addr18 = 23;

// 系统参数结构体
typedef struct {
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

// 舵机控制状态值
typedef enum {
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

// WiFi和UDP配置
const char* ap_ssid = "MainController";
const char* ap_password = "12345678";
WiFiUDP udp;
const int udp_port = 1234;
IPAddress target_ip(192, 168, 4, 2); // 子设备IP

// 数据同步相关
unsigned long lastDataSync = 0;
const unsigned long SYNC_INTERVAL = 2000; // 2秒同步一次
bool dataChanged = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("==========================================");
  Serial.println("主设备启动 - 业务逻辑 + 无线通信");
  Serial.println("==========================================");
  
  // 初始化业务逻辑组件
  LedInit();
  BtInit();
  MotorInit();
  SteerInit();
  CoderInit();
  
  // 初始化WiFi AP模式
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress IP = WiFi.softAPIP();
  
  Serial.print("热点名称: ");
  Serial.println(ap_ssid);
  Serial.print("热点密码: ");
  Serial.println(ap_password);
  Serial.print("AP IP地址: ");
  Serial.println(IP);
  Serial.print("UDP端口: ");
  Serial.println(udp_port);
  Serial.println("等待子设备连接...");
  Serial.println("==========================================");
  
  // 启动UDP服务器
  udp.begin(udp_port);
  
  Serial.println("系统初始化完成！");
}

void loop() {
  // 执行业务逻辑
  BtLoop();
  LedShow();
  SteerLoop();
  
  // 处理无线通信
  handleWirelessCommunication();
  
  // 定时同步数据
  if (millis() - lastDataSync > SYNC_INTERVAL) {
    syncDataToSlave();
    lastDataSync = millis();
  }
  
  delay(10);
}

// 处理无线通信
void handleWirelessCommunication() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char buffer[512];
    int len = udp.read(buffer, 512);
    if (len > 0) {
      buffer[len] = '\0';
      String receivedData = String(buffer);
      
      // 处理来自子设备的请求
      if (receivedData.startsWith("REQUEST_DATA")) {
        Serial.println("收到子设备数据请求");
        sendCurrentData();
      } else if (receivedData.startsWith("CONFIRM_RECEIVED")) {
        Serial.println("子设备确认收到数据");
      }
    }
  }
}

// 同步数据到子设备
void syncDataToSlave() {
  if (dataChanged) {
    sendCurrentData();
    dataChanged = false;
  }
}

// 发送当前数据到子设备
void sendCurrentData() {
  // 创建JSON数据包
  DynamicJsonDocument doc(1024);
  
  // 系统参数
  doc["type"] = "SYSTEM_DATA";
  doc["timestamp"] = millis();
  doc["steerAngle"] = Val.steerAngle;
  doc["steerSpeed"] = Val.steerSpeed;
  doc["steerLTime"] = Val.steerLTime;
  doc["steerRTime"] = Val.steerRTime;
  doc["motorSpeed"] = Val.motorSpeed;
  doc["steerMidVal"] = Val.steerMidVal;
  doc["steerAngle_show"] = Val.steerAngle_show;
  doc["SteerLeftTimeFlag"] = Val.SteerLeftTimeFlag;
  doc["motorSw"] = Val.motorSw;
  doc["steerSw"] = Val.steerSw;
  doc["motorSwFlag"] = Val.motorSwFlag;
  
  // 编码器数据
  JsonArray encoders = doc.createNestedArray("encoders");
  for (int i = 0; i < 5; i++) {
    encoders.add(ecCnt[i]);
  }
  
  // 序列化JSON
  String jsonString;
  serializeJson(doc, jsonString);
  
  // 发送UDP数据包
  udp.beginPacket(target_ip, udp_port);
  udp.print(jsonString);
  udp.endPacket();
  
  Serial.print("发送数据到子设备: ");
  Serial.println(jsonString);
}

// 编码器初始化
void CoderInit(void) {
  pinMode(CLK0, INPUT_PULLUP);
  pinMode(DT0, INPUT_PULLUP);
  pinMode(CLK1, INPUT_PULLUP);
  pinMode(DT1, INPUT_PULLUP);
  pinMode(CLK2, INPUT_PULLUP);
  pinMode(DT2, INPUT_PULLUP);
  pinMode(CLK3, INPUT_PULLUP);
  pinMode(DT3, INPUT_PULLUP);
  pinMode(CLK4, INPUT_PULLUP);
  pinMode(DT20, INPUT_PULLUP);

  attachInterrupt(0, ClockChanged0, CHANGE);
  attachInterrupt(1, ClockChanged1, CHANGE);
  attachInterrupt(2, ClockChanged2, CHANGE);
  attachInterrupt(3, ClockChanged3, CHANGE);
  attachInterrupt(4, ClockChanged4, CHANGE);
}

// 按键初始化
void BtInit(void) {
  myBt1.BindBtnPress([]() {
    // 保存参数到EEPROM
    EEPROM.write(addr1, Val.steerAngle);
    EEPROM.write(addr2, Val.steerSpeed);
    EEPROM.write(addr3, Val.steerLTime);
    EEPROM.write(addr4, Val.steerRTime);
    EEPROM.write(addr5, Val.motorSpeed);
    EEPROM.write(addr6, ecCnt[0]);
    EEPROM.write(addr7, ecCnt[1]);
    EEPROM.write(addr8, ecCnt[2]);
    EEPROM.write(addr9, ecCnt[4]);
    
    int a1 = ecCnt[3] % 100;
    int a2 = ecCnt[3] / 100;
    EEPROM.write(addr10, a1);
    EEPROM.write(addr101, a2);
    
    int a3 = Val.steerMidVal % 100;
    int a4 = Val.steerMidVal / 100;
    EEPROM.write(addr11, a3);
    EEPROM.write(addr111, a4);
    EEPROM.write(addr12, Val.SteerLeftTimeFlag);
    EEPROM.write(addr18, Val.steerAngle_show);
    EEPROM.write(200, 1);
    
    Serial.println("参数已保存到EEPROM");
    dataChanged = true; // 标记数据已更改
  });

  myBt4.BindBtnPress([]() {
    Val.steerSw = !Val.steerSw;
    dataChanged = true;
  });

  myBt3.BindBtnPress([]() {
    if (Val.SteerLeftTimeFlag == 0) {
      Val.SteerLeftTimeFlag = 1;
      ecCnt[2] = map(Val.steerRTime, 0, 25, 0, 198);
    } else if (Val.SteerLeftTimeFlag == 1) {
      Val.SteerLeftTimeFlag = 2;
      ecCnt[2] = map(Val.steerLTime, 0, 25, 0, 198);
    } else {
      Val.SteerLeftTimeFlag = 0;
    }
    dataChanged = true;
  });

  myBt5.BindBtnPress([]() {
    Val.motorSwFlag++;
    if (Val.motorSwFlag > 1) {
      Val.motorSwFlag = 0;
    }
    Val.motorSw = !Val.motorSw;
    dataChanged = true;
  });
}

// 按键循环查询和系统参数更新
void BtLoop(void) {
  static unsigned long _pre_time = 0;
  unsigned long _time = millis();
  if (_time - _pre_time >= 100) {
    _pre_time = _time;

    myBt1.loop();
    myBt2.loop();
    myBt3.loop();
    myBt4.loop();
    myBt5.loop();

    // 更新系统参数
    int sensorValue = ecCnt[0];
    Val.steerAngle_show = map(sensorValue, 0, 198, 0, 99);
    Val.steerAngle = map(sensorValue, 0, 198, MIN_ANGLE, MAX_ANGLE);
    Val.steerSpeed = ecCnt[1] / 4;

    if (Val.SteerLeftTimeFlag == 0) {
      sensorValue = ecCnt[2];
      Val.steerLTime = map(sensorValue, 0, 198, 0, 25);
      Val.steerLTimeAction = Val.steerLTime * 100;
    } else if (Val.SteerLeftTimeFlag == 1) {
      sensorValue = ecCnt[2];
      Val.steerRTime = map(sensorValue, 0, 198, 0, 25);
      Val.steerRTimeAction = Val.steerRTime * 100;
    }

    sensorValue = ecCnt[3];
    Val.steerMidVal = sensorValue + MIN_MID;
    Val.steerLVal = Val.steerMidVal - Val.steerAngle;
    Val.steerRVal = Val.steerMidVal + Val.steerAngle;

    if (Val.steerLVal <= MIN_MID) {
      Val.steerLVal = MIN_MID;
    }
    if (Val.steerRVal >= MAX_MID) {
      Val.steerRVal = MAX_MID;
    }

    Val.motorSpeed = ecCnt[4] / 2;
  }
}

// 数码管初始化
void LedInit(void) {
  led1.begin(36, 34, 32);
  led2.begin(26, 24, 22);
  led3.begin(46, 44, 42);
}

// 刷新显示数码管内容
void LedShow(void) {
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
void MotorInit(void) {
  Val.motorSw = false;
  Val.motorSwFlag = 0;
  pinMode(MotorPwmPin, OUTPUT);
  pinMode(MotorIN1Pin, OUTPUT);
  pinMode(MotorIN2Pin, OUTPUT);
  pinMode(MotorIN3Pin, OUTPUT);
}

// 电机循环控制
void MotorLoop(void) {
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

// 舵机初始化
void SteerInit(void) {
  Val.steerSw = false;
  myservo.attach(SteerPwmPin);
  Val.SteerLeftTimeFlag = 0;

  if (EEPROM.read(200) != 0) {
    // 从EEPROM加载参数
    Val.steerAngle = EEPROM.read(addr1);
    Val.steerSpeed = EEPROM.read(addr2);
    Val.steerLTime = EEPROM.read(addr3);
    Val.steerRTime = EEPROM.read(addr4);
    Val.motorSpeed = EEPROM.read(addr5);
    
    int a3 = EEPROM.read(addr11);
    int a4 = EEPROM.read(addr111);
    Val.steerMidVal = a4 * 100 + a3;
    
    Val.steerLTimeAction = Val.steerLTime * 100;
    Val.steerRTimeAction = Val.steerRTime * 100;
    Val.steerAngle_show = EEPROM.read(addr18);
    
    ecCnt[0] = EEPROM.read(addr6);
    ecCnt[1] = EEPROM.read(addr7);
    ecCnt[2] = EEPROM.read(addr8);
    ecCnt[4] = EEPROM.read(addr9);
    
    int a1 = EEPROM.read(addr10);
    int a2 = EEPROM.read(addr101);
    ecCnt[3] = a2 * 100 + a1;
    Val.SteerLeftTimeFlag = EEPROM.read(addr12);
    
    Serial.println("从EEPROM加载参数成功");
  } else {
    Val.steerNowVal = MID;
    Val.steerLVal = MID - Val.steerAngle;
    Val.steerRVal = MID + Val.steerAngle;
    Val.steerLTimeAction = Val.steerLTime * 100;
    Val.steerRTimeAction = Val.steerRTime * 100;
  }
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
        float t = (100 - Val.steerSpeed) * 20 * k;
        float count = (100 - Val.steerSpeed) * k;
        float v = Val.steerAngle / count;
        
        switch (steerStaue) {
          case LEFT_STOP:
            if (nowTime - lastSTime >= Val.steerLTimeAction) {
              steerStaue = RIGHT_TURN;
            }
            break;
          case LEFT_TURN:
            Val.steerNowVal = Val.steerNowVal - v;
            if (Val.steerNowVal <= Val.steerLVal || Val.steerNowVal >= MAX_MID || Val.steerNowVal <= MIN_MID) {
              steerStaue = LEFT_STOP;
              Val.steerNowVal = Val.steerLVal;
              lastSTime = nowTime;
            }
            break;
          case RIGHT_STOP:
            if (nowTime - lastSTime >= Val.steerRTimeAction) {
              steerStaue = LEFT_TURN;
            }
            break;
          case RIGHT_TURN:
            Val.steerNowVal = Val.steerNowVal + v;
            if (Val.steerNowVal >= Val.steerRVal || Val.steerNowVal <= MIN_MID || Val.steerNowVal >= MAX_MID) {
              steerStaue = RIGHT_STOP;
              Val.steerNowVal = Val.steerRVal;
              lastSTime = nowTime;
            }
            break;
        }
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

// 中断处理函数
void ClockChanged0() {
  int clkValue = digitalRead(CLK0);
  int dtValue = digitalRead(DT0);
  if (clkValue == CLK0_value && DT0_value == dtValue) {
    return;
  } else if (clkValue == dtValue) {
    ecCnt[0]++;
    if (ecCnt[0] > 99 * 2) ecCnt[0] = 99 * 2;
  } else {
    ecCnt[0]--;
    if (ecCnt[0] <= 0) ecCnt[0] = 0;
  }
  CLK0_value = clkValue;
  DT0_value = dtValue;
  dataChanged = true;
}

void ClockChanged1() {
  int clkValue = digitalRead(CLK1);
  int dtValue = digitalRead(DT1);
  if (clkValue == CLK1_value && DT1_value == dtValue) {
    return;
  } else if (clkValue == dtValue) {
    ecCnt[1]++;
    if (ecCnt[1] > 99 * 4) ecCnt[1] = 99 * 4;
  } else {
    ecCnt[1]--;
    if (ecCnt[1] <= 0) ecCnt[1] = 0;
  }
  CLK1_value = clkValue;
  DT1_value = dtValue;
  dataChanged = true;
}

void ClockChanged2() {
  int clkValue = digitalRead(CLK2);
  int dtValue = digitalRead(DT2);
  if (clkValue == CLK2_value && DT2_value == dtValue) {
    return;
  } else if (clkValue == dtValue) {
    ecCnt[2]++;
    if (ecCnt[2] > 99 * 2) ecCnt[2] = 99 * 2;
  } else {
    ecCnt[2]--;
    if (ecCnt[2] <= 0) ecCnt[2] = 0;
  }
  CLK2_value = clkValue;
  DT2_value = dtValue;
  dataChanged = true;
}

void ClockChanged3() {
  int clkValue = digitalRead(CLK3);
  int dtValue = digitalRead(DT3);
  if (clkValue == CLK3_value && DT3_value == dtValue) {
    return;
  } else if (clkValue == dtValue) {
    ecCnt[3]++;
    if (ecCnt[3] > 900) ecCnt[3] = 900;
  } else {
    ecCnt[3]--;
    if (ecCnt[3] <= 0) ecCnt[3] = 0;
  }
  CLK3_value = clkValue;
  DT3_value = dtValue;
  dataChanged = true;
}

void ClockChanged4() {
  int clkValue = digitalRead(CLK4);
  int dtValue = digitalRead(DT20);
  if (clkValue == CLK4_value && DT4_value == dtValue) {
    return;
  } else if (clkValue == dtValue) {
    ecCnt[4]++;
    if (ecCnt[4] > 99 * 2) ecCnt[4] = 99 * 2;
  } else {
    ecCnt[4]--;
    if (ecCnt[4] <= 0) ecCnt[4] = 0;
  }
  CLK4_value = clkValue;
  DT4_value = dtValue;
  dataChanged = true;
}
