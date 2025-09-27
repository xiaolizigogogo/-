/*
 * 433MHz发射接收一体模块双向通信系统
 * 支持设备间相互通信和数据传输
 */

#include <RCSwitch.h>

// 硬件定义
#define LED_PIN 13
#define SERVO_PIN 9
#define MOTOR_PIN1 5
#define MOTOR_PIN2 6
#define BUZZER_PIN 8

// 433MHz一体模块引脚
#define TX_EN_PIN 7    // 发射使能引脚
#define RX_EN_PIN 4    // 接收使能引脚
#define DATA_PIN 2     // 数据引脚（发射/接收共用）

// 无线通信
RCSwitch mySwitch = RCSwitch();

// 设备配置
#define DEVICE_ID 1    // 本设备ID
#define MASTER_ID 0    // 主控设备ID

// 通信协议
#define CMD_LED_ON     0x01
#define CMD_LED_OFF    0x02
#define CMD_MOTOR_FWD  0x03
#define CMD_MOTOR_BWD  0x04
#define CMD_MOTOR_STOP 0x05
#define CMD_SERVO_LEFT 0x06
#define CMD_SERVO_RIGHT 0x07
#define CMD_STATUS_REQ 0x08
#define CMD_STATUS_RESP 0x09
#define CMD_HEARTBEAT  0x0A
#define CMD_EMERGENCY  0x0B

// 系统状态
int servoAngle = 90;
int motorSpeed = 0;
bool ledState = false;
bool motorDirection = true;
unsigned long lastHeartbeat = 0;
unsigned long lastStatusCheck = 0;

// 通信状态
bool isMaster = false;
bool communicationEnabled = true;

void setup() {
  Serial.begin(9600);
  
  // 初始化433MHz一体模块
  pinMode(TX_EN_PIN, OUTPUT);
  pinMode(RX_EN_PIN, OUTPUT);
  
  // 初始化为接收模式
  setReceiveMode();
  
  // 初始化无线通信
  mySwitch.enableReceive(digitalPinToInterrupt(DATA_PIN));
  
  // 初始化其他引脚
  pinMode(LED_PIN, OUTPUT);
  pinMode(SERVO_PIN, OUTPUT);
  pinMode(MOTOR_PIN1, OUTPUT);
  pinMode(MOTOR_PIN2, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // 初始化舵机
  analogWrite(SERVO_PIN, map(servoAngle, 0, 180, 0, 255));
  
  Serial.println("=== 433MHz双向通信系统启动 ===");
  Serial.print("设备ID: ");
  Serial.println(DEVICE_ID);
  Serial.println("输入 'master' 设置为主控设备");
  Serial.println("输入 'slave' 设置为从设备");
  Serial.println("输入 'send <cmd> <data>' 发送命令");
  Serial.println("输入 'status' 查看状态");
  
  // 启动提示音
  playStartupSound();
}

void loop() {
  // 检查串口命令
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    processSerialCommand(command);
  }
  
  // 检查接收到的信号
  if (mySwitch.available()) {
    long value = mySwitch.getReceivedValue();
    int protocol = mySwitch.getReceivedProtocol();
    int bitLength = mySwitch.getReceivedBitlength();
    
    Serial.print("收到信号: ");
    Serial.print(value);
    Serial.print(" 协议: ");
    Serial.print(protocol);
    Serial.print(" 位长: ");
    Serial.println(bitLength);
    
    // 解析接收到的数据
    processReceivedData(value);
    
    // 重置接收器
    mySwitch.resetAvailable();
  }
  
  // 主控设备定期发送心跳
  if (isMaster && millis() - lastHeartbeat > 5000) {
    sendHeartbeat();
    lastHeartbeat = millis();
  }
  
  // 定期状态检查
  if (millis() - lastStatusCheck > 10000) {
    if (!isMaster) {
      sendStatusResponse();
    }
    lastStatusCheck = millis();
  }
  
  delay(10);
}

void processSerialCommand(String command) {
  if (command == "master") {
    setMasterMode();
  } else if (command == "slave") {
    setSlaveMode();
  } else if (command == "status") {
    showStatus();
  } else if (command.startsWith("send ")) {
    processSendCommand(command);
  } else if (command == "help") {
    showHelp();
  } else {
    Serial.println("未知命令，输入 'help' 查看帮助");
  }
}

void setMasterMode() {
  isMaster = true;
  Serial.println("设置为主控设备");
  Serial.println("将定期发送心跳信号");
  playSuccessSound();
}

void setSlaveMode() {
  isMaster = false;
  Serial.println("设置为从设备");
  Serial.println("将响应主控设备命令");
  playSuccessSound();
}

void processSendCommand(String command) {
  // 解析 send <cmd> <data> 格式
  command = command.substring(5); // 移除 "send "
  int spaceIndex = command.indexOf(' ');
  
  if (spaceIndex == -1) {
    Serial.println("格式错误: send <cmd> <data>");
    return;
  }
  
  String cmdStr = command.substring(0, spaceIndex);
  String dataStr = command.substring(spaceIndex + 1);
  
  int cmd = cmdStr.toInt();
  int data = dataStr.toInt();
  
  // 发送命令
  sendCommand(cmd, data);
}

void sendCommand(int command, int data) {
  if (!communicationEnabled) {
    Serial.println("通信未启用");
    return;
  }
  
  // 切换到发射模式
  setTransmitMode();
  delay(10);
  
  // 构建数据包
  long packet = buildPacket(DEVICE_ID, command, data);
  
  // 发送数据
  mySwitch.send(packet, 24);
  
  Serial.print("发送命令: ");
  Serial.print(command);
  Serial.print(" 数据: ");
  Serial.println(data);
  
  // 切换回接收模式
  setReceiveMode();
  delay(10);
  
  playSuccessSound();
}

void processReceivedData(long value) {
  // 解析数据包
  int senderId = (value >> 16) & 0xFF;
  int command = (value >> 8) & 0xFF;
  int data = value & 0xFF;
  
  Serial.print("来自设备 ");
  Serial.print(senderId);
  Serial.print(" 命令: ");
  Serial.print(command);
  Serial.print(" 数据: ");
  Serial.println(data);
  
  // 处理命令
  switch (command) {
    case CMD_LED_ON:
      ledOn();
      break;
      
    case CMD_LED_OFF:
      ledOff();
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
      servoLeft();
      break;
      
    case CMD_SERVO_RIGHT:
      servoRight();
      break;
      
    case CMD_STATUS_REQ:
      sendStatusResponse();
      break;
      
    case CMD_STATUS_RESP:
      processStatusResponse(data);
      break;
      
    case CMD_HEARTBEAT:
      processHeartbeat(senderId);
      break;
      
    case CMD_EMERGENCY:
      emergencyStop();
      break;
      
    default:
      Serial.println("未知命令");
      break;
  }
}

long buildPacket(int deviceId, int command, int data) {
  return ((long)deviceId << 16) | ((long)command << 8) | data;
}

void sendHeartbeat() {
  sendCommand(CMD_HEARTBEAT, DEVICE_ID);
}

void sendStatusResponse() {
  // 构建状态数据
  int status = 0;
  if (ledState) status |= 0x01;
  if (motorSpeed > 0) status |= 0x02;
  if (motorDirection) status |= 0x04;
  
  sendCommand(CMD_STATUS_RESP, status);
}

void processStatusResponse(int status) {
  Serial.print("设备状态: ");
  Serial.print("LED=");
  Serial.print((status & 0x01) ? "ON" : "OFF");
  Serial.print(" 电机=");
  Serial.print((status & 0x02) ? "运行" : "停止");
  Serial.print(" 方向=");
  Serial.println((status & 0x04) ? "前进" : "后退");
}

void processHeartbeat(int senderId) {
  Serial.print("收到心跳信号，来自设备: ");
  Serial.println(senderId);
}

void setTransmitMode() {
  digitalWrite(TX_EN_PIN, HIGH);
  digitalWrite(RX_EN_PIN, LOW);
  mySwitch.disableReceive();
}

void setReceiveMode() {
  digitalWrite(TX_EN_PIN, LOW);
  digitalWrite(RX_EN_PIN, HIGH);
  mySwitch.enableReceive(digitalPinToInterrupt(DATA_PIN));
}

// 控制函数
void ledOn() {
  ledState = true;
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED已开启");
  playSuccessSound();
}

void ledOff() {
  ledState = false;
  digitalWrite(LED_PIN, LOW);
  Serial.println("LED已关闭");
  playSuccessSound();
}

void motorForward() {
  digitalWrite(MOTOR_PIN1, HIGH);
  digitalWrite(MOTOR_PIN2, LOW);
  motorSpeed = 255;
  motorDirection = true;
  Serial.println("电机前进");
  playSuccessSound();
}

void motorBackward() {
  digitalWrite(MOTOR_PIN1, LOW);
  digitalWrite(MOTOR_PIN2, HIGH);
  motorSpeed = 255;
  motorDirection = false;
  Serial.println("电机后退");
  playSuccessSound();
}

void motorStop() {
  digitalWrite(MOTOR_PIN1, LOW);
  digitalWrite(MOTOR_PIN2, LOW);
  motorSpeed = 0;
  Serial.println("电机停止");
  playSuccessSound();
}

void servoLeft() {
  servoAngle = max(0, servoAngle - 30);
  analogWrite(SERVO_PIN, map(servoAngle, 0, 180, 0, 255));
  Serial.print("舵机左转至: ");
  Serial.println(servoAngle);
  playSuccessSound();
}

void servoRight() {
  servoAngle = min(180, servoAngle + 30);
  analogWrite(SERVO_PIN, map(servoAngle, 0, 180, 0, 255));
  Serial.print("舵机右转至: ");
  Serial.println(servoAngle);
  playSuccessSound();
}

void emergencyStop() {
  motorStop();
  ledOff();
  servoAngle = 90;
  analogWrite(SERVO_PIN, map(servoAngle, 0, 180, 0, 255));
  Serial.println("紧急停止！所有设备已停止");
  playEmergencySound();
}

void showStatus() {
  Serial.println("=== 系统状态 ===");
  Serial.print("设备ID: ");
  Serial.println(DEVICE_ID);
  Serial.print("模式: ");
  Serial.println(isMaster ? "主控" : "从设备");
  Serial.print("通信状态: ");
  Serial.println(communicationEnabled ? "启用" : "禁用");
  Serial.print("LED: ");
  Serial.println(ledState ? "开启" : "关闭");
  Serial.print("舵机角度: ");
  Serial.println(servoAngle);
  Serial.print("电机: ");
  if (motorSpeed == 0) {
    Serial.println("停止");
  } else {
    Serial.print(motorDirection ? "前进" : "后退");
    Serial.print(" 速度: ");
    Serial.println(motorSpeed);
  }
  Serial.println("================");
}

void showHelp() {
  Serial.println("=== 命令帮助 ===");
  Serial.println("master - 设置为主控设备");
  Serial.println("slave  - 设置为从设备");
  Serial.println("status - 查看系统状态");
  Serial.println("send <cmd> <data> - 发送命令");
  Serial.println("help   - 显示此帮助");
  Serial.println("");
  Serial.println("命令代码:");
  Serial.println("0x01 - LED开启");
  Serial.println("0x02 - LED关闭");
  Serial.println("0x03 - 电机前进");
  Serial.println("0x04 - 电机后退");
  Serial.println("0x05 - 电机停止");
  Serial.println("0x06 - 舵机左转");
  Serial.println("0x07 - 舵机右转");
  Serial.println("0x08 - 状态请求");
  Serial.println("0x09 - 状态响应");
  Serial.println("0x0A - 心跳信号");
  Serial.println("0x0B - 紧急停止");
  Serial.println("================");
}

// 音效函数
void playStartupSound() {
  tone(BUZZER_PIN, 1000, 200);
  delay(200);
  tone(BUZZER_PIN, 1500, 200);
  delay(200);
  tone(BUZZER_PIN, 2000, 200);
}

void playSuccessSound() {
  tone(BUZZER_PIN, 2000, 100);
  delay(100);
  tone(BUZZER_PIN, 2500, 100);
}

void playEmergencySound() {
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 500, 200);
    delay(200);
    tone(BUZZER_PIN, 1000, 200);
    delay(200);
  }
} 