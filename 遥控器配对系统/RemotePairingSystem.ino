/*
 * 遥控器配对系统
 * 支持遥控器与设备一对一配对
 * 确保遥控器只能操作配对成功的设备
 */

#include "FBLed.h"
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

// 配对系统状态
enum PairingState {
  PAIRING_IDLE,        // 空闲状态
  PAIRING_WAITING,     // 等待配对
  PAIRING_SUCCESS,     // 配对成功
  PAIRING_FAILED       // 配对失败
};

// 系统状态结构
struct SystemStatus {
  bool remoteEnabled = false;      // 遥控器是否启用
  bool isPaired = false;           // 是否已配对
  uint32_t pairedRemoteId = 0;     // 配对遥控器ID
  uint8_t deviceId = 0x01;         // 设备ID
  PairingState pairingState = PAIRING_IDLE;
  
  // 设备控制状态
  bool ledState = false;
  int servoAngle = 90;
  int motorSpeed = 0;
  int maxMotorSpeed = 255;
  bool motorDirection = true;
  
  // 配对相关
  unsigned long pairingStartTime = 0;
  const unsigned long PAIRING_TIMEOUT = 10000; // 10秒配对超时
  unsigned long lastCommandTime = 0;
  const unsigned long COMMAND_TIMEOUT = 5000;  // 5秒指令超时
  
  // 新增：旋钮同步控制
  bool syncMode = false;           // 同步模式开关
  bool encoderSyncEnabled = true;  // 编码器同步启用
  unsigned long lastEncoderChange = 0;  // 最后编码器变化时间
  const unsigned long SYNC_TIMEOUT = 2000;  // 2秒同步超时
  
  // 编码器参数缓存（用于同步）
  int encoderValues[7] = {0, 0, 0, 0, 0, 0, 0};  // 当前编码器值
  int targetValues[7] = {0, 0, 0, 0, 0, 0, 0};   // 目标值（遥控器设置）
  bool valueChanged[7] = {false, false, false, false, false, false, false};  // 值是否被遥控器改变
} status;

// 遥控器指令定义
enum RemoteCommands {
  CMD_NONE = 0x00,
  CMD_PAIR_REQUEST = 0x01,     // 配对请求
  CMD_PAIR_CONFIRM = 0x02,     // 配对确认
  CMD_PAIR_CANCEL = 0x03,      // 取消配对
  CMD_LED_ON = 0x04,           // 开灯
  CMD_LED_OFF = 0x05,          // 关灯
  CMD_MOTOR_FWD = 0x06,        // 前进
  CMD_MOTOR_BWD = 0x07,        // 后退
  CMD_MOTOR_STOP = 0x08,       // 停止
  CMD_SERVO_LEFT = 0x09,       // 左转
  CMD_SERVO_RIGHT = 0x0A,      // 右转
  CMD_SPEED_UP = 0x0B,         // 加速
  CMD_SPEED_DOWN = 0x0C,       // 减速
  CMD_STATUS = 0x0D,           // 状态查询
  CMD_EMERGENCY_STOP = 0x0E    // 紧急停止
};

// 引脚定义
#define MIN_ANGLE 0
#define MAX_ANGLE 180
#define MID 1550
#define MIN_MID 1100
#define MAX_MID 2000

#define bt1 31  // 配对触发按键
#define bt2 33  // 舵机开关
#define bt3 35  // 取消配对
#define bt4 37  // 紧急停止
#define bt5 39  // 电机开关
#define bt6 41  // 电机开关
#define bt7 43  // 电机开关

#define MotorPwmPin 7
#define MotorPwmPin2 8
#define MotorPwmPin3 10
#define MotorIN1Pin 6
#define MotorIN2Pin 5
#define MotorIN3Pin 4
#define SteerPwmPin 9

// 编码器引脚定义
int CLK0 = 2, DT0 = 11;
int CLK1 = 3, DT1 = 12;
int CLK2 = 21, DT2 = 13;
int CLK3 = 20, DT3 = 14;
int CLK4 = 19, DT4 = 15;
int CLK5 = 18, DT5 = 16;
int CLK6 = 23, DT6 = 17;

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
  
  // 初始化硬件
  setupHardware();
  
  // 初始化配对系统
  setupPairingSystem();
  
  // 初始化无线接收器
  mySwitch.enableReceive(0);
  
  Serial.println("=== 遥控器配对系统启动 ===");
  Serial.print("设备ID: ");
  Serial.println(status.deviceId);
  printPairingStatus();
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

void setupPairingSystem() {
  // 从EEPROM读取配对信息
  status.isPaired = KeyValueEEPROM::get("is_paired", false);
  status.pairedRemoteId = KeyValueEEPROM::get("remote_id", 0);
  status.deviceId = KeyValueEEPROM::get("device_id", 0x01);
  
  if (status.isPaired && status.pairedRemoteId != 0) {
    status.remoteEnabled = true;
    status.pairingState = PAIRING_SUCCESS;
    Serial.println("已找到配对信息，遥控器已启用");
  } else {
    status.remoteEnabled = false;
    status.pairingState = PAIRING_IDLE;
    Serial.println("未找到配对信息，需要配对");
  }
}

void loop() {
  // 处理配对状态
  handlePairingState();
  
  // 处理遥控器指令
  handleRemoteControl();
  
  // 处理本地按钮
  handleLocalButtons();
  
  // 处理LED显示
  updateLEDStatus();
  
  // 处理编码器（原有功能）
  handleEncoders();
  
  delay(10);
}

void handlePairingState() {
  switch (status.pairingState) {
    case PAIRING_WAITING:
      // 检查配对超时
      if (millis() - status.pairingStartTime > status.PAIRING_TIMEOUT) {
        status.pairingState = PAIRING_FAILED;
        Serial.println("配对超时，配对失败");
        playPairingFailedSound();
      }
      break;
      
    case PAIRING_SUCCESS:
      // 配对成功状态，正常操作
      break;
      
    case PAIRING_FAILED:
      // 配对失败，等待重新配对
      break;
      
    default:
      break;
  }
}

void handleRemoteControl() {
  if (mySwitch.available()) {
    long value = mySwitch.getReceivedValue();
    Serial.print("收到遥控器信号: 0x");
    Serial.println(value, HEX);
    
    // 解析遥控器指令
    uint32_t remoteId = (value >> 16) & 0xFFFF;
    uint8_t command = value & 0xFF;
    
    // 检查配对状态
    if (!status.isPaired) {
      // 未配对，只处理配对相关指令
      if (command == CMD_PAIR_REQUEST) {
        handlePairingRequest(remoteId);
      }
    } else {
      // 已配对，检查遥控器ID
      if (remoteId == status.pairedRemoteId) {
        // 配对遥控器，处理指令
        handleRemoteCommand(command);
        status.lastCommandTime = millis();
      } else {
        Serial.println("非配对遥控器，忽略指令");
      }
    }
    
    mySwitch.resetAvailable();
  }
  
  // 检查指令超时
  if (status.isPaired && millis() - status.lastCommandTime > status.COMMAND_TIMEOUT) {
    // 可以在这里添加超时处理逻辑
  }
}

void handlePairingRequest(uint32_t remoteId) {
  Serial.print("收到配对请求，遥控器ID: 0x");
  Serial.println(remoteId, HEX);
  
  status.pairingState = PAIRING_WAITING;
  status.pairingStartTime = millis();
  
  // 开始配对流程
  startPairingProcess(remoteId);
}

void startPairingProcess(uint32_t remoteId) {
  Serial.println("开始配对流程...");
  
  // 保存配对信息
  status.pairedRemoteId = remoteId;
  status.isPaired = true;
  status.remoteEnabled = true;
  status.pairingState = PAIRING_SUCCESS;
  
  // 保存到EEPROM
  KeyValueEEPROM::put("is_paired", true);
  KeyValueEEPROM::put("remote_id", remoteId);
  
  Serial.println("配对成功！");
  playPairingSuccessSound();
  printPairingStatus();
}

void handleRemoteCommand(uint8_t command) {
  Serial.print("执行遥控器指令: 0x");
  Serial.println(command, HEX);
  
  switch (command) {
    case CMD_LED_ON:
      turnOnLED();
      break;
      
    case CMD_LED_OFF:
      turnOffLED();
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
      
    case CMD_SERVO_LEFT:
      servoRotateLeft();
      break;
      
    case CMD_SERVO_RIGHT:
      servoRotateRight();
      break;
      
    case CMD_SPEED_UP:
      increaseSpeed();
      break;
      
    case CMD_SPEED_DOWN:
      decreaseSpeed();
      break;
      
    case CMD_STATUS:
      reportSystemStatus();
      break;
      
    case CMD_EMERGENCY_STOP:
      emergencyStop();
      break;
      
    case CMD_PAIR_CANCEL:
      cancelPairing();
      break;
      
    default:
      Serial.println("未知指令");
      break;
  }
  
  // 标记参数已被遥控器修改
  markParameterChanged(command);
}

void markParameterChanged(uint8_t command) {
  switch (command) {
    case CMD_SPEED_UP:
    case CMD_SPEED_DOWN:
      // 电机速度相关
      status.valueChanged[4] = true; // 电机1
      status.valueChanged[5] = true; // 电机2
      status.valueChanged[6] = true; // 电机3
      break;
      
    case CMD_SERVO_LEFT:
    case CMD_SERVO_RIGHT:
      // 舵机角度相关
      status.valueChanged[0] = true; // 舵机摆动幅度
      status.valueChanged[1] = true; // 舵机摆动速度
      break;
  }
}

void handleLocalButtons() {
  // 配对触发按键 / 同步模式切换
  if (digitalRead(bt1) == LOW) {
    if (!status.isPaired) {
      Serial.println("手动触发配对模式");
      status.pairingState = PAIRING_WAITING;
      status.pairingStartTime = millis();
    } else {
      // 已配对时，切换同步模式
      status.syncMode = !status.syncMode;
      Serial.print("同步模式: ");
      Serial.println(status.syncMode ? "开启" : "关闭");
      if (status.syncMode) {
        showCurrentParameters();
      }
    }
    delay(200);
  }
  
  // 取消配对按键 / 重置同步
  if (digitalRead(bt3) == LOW) {
    if (status.syncMode) {
      // 在同步模式下，重置所有参数同步
      resetParameterSync();
    } else {
      // 正常模式下，取消配对
      cancelPairing();
    }
    delay(200);
  }
  
  // 紧急停止按键
  if (digitalRead(bt4) == LOW) {
    emergencyStop();
    delay(200);
  }
  
  // 其他按键（原有功能）
  if (digitalRead(bt2) == LOW) {
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

void cancelPairing() {
  Serial.println("取消配对");
  
  // 清除配对信息
  status.isPaired = false;
  status.remoteEnabled = false;
  status.pairedRemoteId = 0;
  status.pairingState = PAIRING_IDLE;
  
  // 清除EEPROM
  KeyValueEEPROM::put("is_paired", false);
  KeyValueEEPROM::put("remote_id", 0);
  
  // 停止所有设备
  emergencyStop();
  
  printPairingStatus();
}

void emergencyStop() {
  Serial.println("紧急停止执行");
  motorStop();
  turnOffLED();
  myservo.write(90);
}

// 设备控制函数
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
  Serial.print("设备ID: ");
  Serial.println(status.deviceId);
  Serial.print("配对状态: ");
  Serial.println(status.isPaired ? "已配对" : "未配对");
  if (status.isPaired) {
    Serial.print("配对遥控器ID: 0x");
    Serial.println(status.pairedRemoteId, HEX);
  }
  Serial.print("遥控器控制: ");
  Serial.println(status.remoteEnabled ? "启用" : "禁用");
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
  Serial.println("==================");
}

void updateLEDStatus() {
  // 根据配对状态更新LED
  switch (status.pairingState) {
    case PAIRING_IDLE:
      // 未配对：慢闪红灯
      led1.update();
      led2.setBrightness(0);
      led3.setBrightness(0);
      break;
      
    case PAIRING_WAITING:
      // 等待配对：快闪黄灯
      if (millis() % 500 < 250) {
        led1.setBrightness(255);
        led2.setBrightness(255);
        led3.setBrightness(0);
      } else {
        led1.setBrightness(0);
        led2.setBrightness(0);
        led3.setBrightness(0);
      }
      break;
      
    case PAIRING_SUCCESS:
      // 配对成功：常亮绿灯
      led1.setBrightness(0);
      led2.setBrightness(0);
      led3.setBrightness(255);
      break;
      
    case PAIRING_FAILED:
      // 配对失败：快闪红灯
      if (millis() % 200 < 100) {
        led1.setBrightness(255);
        led2.setBrightness(0);
        led3.setBrightness(0);
      } else {
        led1.setBrightness(0);
        led2.setBrightness(0);
        led3.setBrightness(0);
      }
      break;
  }
}

void playPairingSuccessSound() {
  // 配对成功音效（如果有蜂鸣器）
  // tone(pin, frequency, duration);
  Serial.println("配对成功音效");
}

void playPairingFailedSound() {
  // 配对失败音效
  Serial.println("配对失败音效");
}

void printPairingStatus() {
  Serial.println("=== 配对状态 ===");
  Serial.print("配对状态: ");
  switch (status.pairingState) {
    case PAIRING_IDLE:
      Serial.println("空闲");
      break;
    case PAIRING_WAITING:
      Serial.println("等待配对");
      break;
    case PAIRING_SUCCESS:
      Serial.println("配对成功");
      break;
    case PAIRING_FAILED:
      Serial.println("配对失败");
      break;
  }
  Serial.print("遥控器启用: ");
  Serial.println(status.remoteEnabled ? "是" : "否");
  Serial.print("已配对: ");
  Serial.println(status.isPaired ? "是" : "否");
  if (status.isPaired) {
    Serial.print("配对遥控器ID: 0x");
    Serial.println(status.pairedRemoteId, HEX);
  }
  Serial.print("同步模式: ");
  Serial.println(status.syncMode ? "开启" : "关闭");
  Serial.println("================");
}

// 新增：显示当前参数状态
void showCurrentParameters() {
  Serial.println("=== 当前参数状态 ===");
  Serial.print("舵机角度: ");
  Serial.println(status.servoAngle);
  Serial.print("电机速度: ");
  Serial.println(status.maxMotorSpeed);
  Serial.print("LED状态: ");
  Serial.println(status.ledState ? "开启" : "关闭");
  
  Serial.println("编码器值:");
  for (int i = 0; i < 7; i++) {
    Serial.print("  编码器");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(status.encoderValues[i]);
    if (status.valueChanged[i]) {
      Serial.print(" (遥控器已修改)");
    }
    Serial.println();
  }
  Serial.println("==================");
}

// 新增：重置参数同步
void resetParameterSync() {
  Serial.println("重置所有参数同步状态");
  for (int i = 0; i < 7; i++) {
    status.valueChanged[i] = false;
  }
  Serial.println("所有遥控器修改标记已清除");
}

// 新增：检查参数同步状态
void checkParameterSync() {
  if (!status.syncMode) return;
  
  Serial.println("=== 参数同步检查 ===");
  bool hasConflict = false;
  
  for (int i = 0; i < 7; i++) {
    if (status.valueChanged[i]) {
      Serial.print("编码器");
      Serial.print(i);
      Serial.println(": 遥控器已修改，需要手动同步");
      hasConflict = true;
    }
  }
  
  if (!hasConflict) {
    Serial.println("所有参数已同步");
  }
  Serial.println("==================");
}

// 编码器引脚定义（与原有系统兼容）
int CLK0 = 2, DT0 = 11;
int CLK1 = 3, DT1 = 12;
int CLK2 = 21, DT2 = 13;
int CLK3 = 20, DT3 = 14;
int CLK4 = 19, DT4 = 15;
int CLK5 = 18, DT5 = 16;
int CLK6 = 23, DT6 = 17;

// 编码器状态
int encoderCounts[7] = {0, 0, 0, 0, 0, 0, 0};
int lastEncoderCounts[7] = {0, 0, 0, 0, 0, 0, 0};

void handleEncoders() {
  // 读取所有编码器值
  readAllEncoders();
  
  // 处理编码器变化
  for (int i = 0; i < 7; i++) {
    if (encoderCounts[i] != lastEncoderCounts[i]) {
      handleEncoderChange(i, encoderCounts[i]);
      lastEncoderCounts[i] = encoderCounts[i];
      status.lastEncoderChange = millis();
    }
  }
}

void readAllEncoders() {
  // 读取编码器0（舵机摆动幅度）
  encoderCounts[0] = readEncoder(CLK0, DT0);
  
  // 读取编码器1（舵机摆动速度）
  encoderCounts[1] = readEncoder(CLK1, DT1);
  
  // 读取编码器2（舵机停留时间）
  encoderCounts[2] = readEncoder(CLK2, DT2);
  
  // 读取编码器3（舵机中位点）
  encoderCounts[3] = readEncoder(CLK3, DT3);
  
  // 读取编码器4（电机1速度）
  encoderCounts[4] = readEncoder(CLK4, DT4);
  
  // 读取编码器5（电机2速度）
  encoderCounts[5] = readEncoder(CLK5, DT5);
  
  // 读取编码器6（电机3速度）
  encoderCounts[6] = readEncoder(CLK6, DT6);
}

int readEncoder(int clkPin, int dtPin) {
  static int lastClkState[7] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
  static int encoderCount[7] = {0, 0, 0, 0, 0, 0, 0};
  
  int encoderIndex = -1;
  if (clkPin == CLK0) encoderIndex = 0;
  else if (clkPin == CLK1) encoderIndex = 1;
  else if (clkPin == CLK2) encoderIndex = 2;
  else if (clkPin == CLK3) encoderIndex = 3;
  else if (clkPin == CLK4) encoderIndex = 4;
  else if (clkPin == CLK5) encoderIndex = 5;
  else if (clkPin == CLK6) encoderIndex = 6;
  
  if (encoderIndex == -1) return 0;
  
  int clkState = digitalRead(clkPin);
  int dtState = digitalRead(dtPin);
  
  if (clkState != lastClkState[encoderIndex]) {
    if (dtState != clkState) {
      encoderCount[encoderIndex]++;
    } else {
      encoderCount[encoderIndex]--;
    }
    lastClkState[encoderIndex] = clkState;
  }
  
  return encoderCount[encoderIndex];
}

void handleEncoderChange(int encoderIndex, int newValue) {
  // 更新当前编码器值
  status.encoderValues[encoderIndex] = newValue;
  
  // 如果在同步模式下，检查是否需要更新参数
  if (status.syncMode) {
    updateParameterFromEncoder(encoderIndex, newValue);
  } else {
    // 正常模式下，检查是否与遥控器设置的值不同
    if (status.valueChanged[encoderIndex]) {
      // 如果旋钮被手动调节，清除遥控器修改标记
      status.valueChanged[encoderIndex] = false;
      Serial.print("编码器");
      Serial.print(encoderIndex);
      Serial.println("已手动调节，清除遥控器修改标记");
    }
  }
}

void updateParameterFromEncoder(int encoderIndex, int value) {
  switch (encoderIndex) {
    case 0: // 舵机摆动幅度
      status.servoAngle = map(value, 0, 198, 0, 180);
      Serial.print("同步模式：舵机摆动幅度设置为 ");
      Serial.println(status.servoAngle);
      break;
      
    case 1: // 舵机摆动速度
      status.maxMotorSpeed = map(value, 0, 396, 50, 255);
      Serial.print("同步模式：舵机摆动速度设置为 ");
      Serial.println(status.maxMotorSpeed);
      break;
      
    case 2: // 舵机停留时间
      // 这里可以根据需要设置停留时间参数
      Serial.print("同步模式：舵机停留时间设置为 ");
      Serial.println(value);
      break;
      
    case 3: // 舵机中位点
      // 这里可以根据需要设置中位点参数
      Serial.print("同步模式：舵机中位点设置为 ");
      Serial.println(value);
      break;
      
    case 4: // 电机1速度
      status.maxMotorSpeed = map(value, 0, 198, 50, 255);
      Serial.print("同步模式：电机1速度设置为 ");
      Serial.println(status.maxMotorSpeed);
      break;
      
    case 5: // 电机2速度
      status.maxMotorSpeed = map(value, 0, 198, 50, 255);
      Serial.print("同步模式：电机2速度设置为 ");
      Serial.println(status.maxMotorSpeed);
      break;
      
    case 6: // 电机3速度
      status.maxMotorSpeed = map(value, 0, 198, 50, 255);
      Serial.print("同步模式：电机3速度设置为 ");
      Serial.println(status.maxMotorSpeed);
      break;
  }
} 