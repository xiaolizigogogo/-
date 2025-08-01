/*
 * Arduino 离线语音控制示例
 * 使用WTN6040语音识别模块
 * 无需联网，本地关键词识别
 */

#include <SoftwareSerial.h>

// 硬件定义
#define LED_PIN 13
#define SERVO_PIN 9
#define MOTOR_PIN1 5
#define MOTOR_PIN2 6
#define BUZZER_PIN 8

// WTN6040语音模块串口
#define WTN_RX 2
#define WTN_TX 3
SoftwareSerial wtnSerial(WTN_RX, WTN_TX);

// 语音指令定义
enum VoiceCommands {
  CMD_LED_ON = 1,      // "开灯"
  CMD_LED_OFF = 2,     // "关灯"
  CMD_SERVO_LEFT = 3,  // "左转"
  CMD_SERVO_RIGHT = 4, // "右转"
  CMD_MOTOR_FORWARD = 5, // "前进"
  CMD_MOTOR_BACKWARD = 6, // "后退"
  CMD_MOTOR_STOP = 7,  // "停止"
  CMD_SYSTEM_STATUS = 8 // "状态"
};

// 系统状态
struct SystemStatus {
  bool ledState = false;
  int servoAngle = 90;
  int motorSpeed = 0;
  bool motorDirection = true; // true=前进, false=后退
} status;

void setup() {
  Serial.begin(9600);
  wtnSerial.begin(9600);
  
  // 初始化引脚
  pinMode(LED_PIN, OUTPUT);
  pinMode(SERVO_PIN, OUTPUT);
  pinMode(MOTOR_PIN1, OUTPUT);
  pinMode(MOTOR_PIN2, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // 初始化舵机
  analogWrite(SERVO_PIN, map(status.servoAngle, 0, 180, 0, 255));
  
  // 初始化WTN6040模块
  initWTN6040();
  
  Serial.println("离线语音控制系统已启动");
  playStartupSound();
}

void loop() {
  // 检查语音模块是否有数据
  if (wtnSerial.available()) {
    int command = wtnSerial.read();
    Serial.print("收到语音指令: ");
    Serial.println(command);
    
    processVoiceCommand(command);
  }
  
  delay(100);
}

void initWTN6040() {
  // WTN6040初始化命令
  // 设置语音识别模式
  wtnSerial.write(0xAA);
  wtnSerial.write(0x55);
  wtnSerial.write(0x01); // 进入识别模式
  
  delay(1000);
  
  // 设置关键词数量
  wtnSerial.write(0xAA);
  wtnSerial.write(0x55);
  wtnSerial.write(0x08); // 8个关键词
  
  Serial.println("WTN6040语音模块初始化完成");
}

void processVoiceCommand(int command) {
  switch (command) {
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
      
    case CMD_MOTOR_FORWARD:
      motorForward();
      break;
      
    case CMD_MOTOR_BACKWARD:
      motorBackward();
      break;
      
    case CMD_MOTOR_STOP:
      motorStop();
      break;
      
    case CMD_SYSTEM_STATUS:
      reportStatus();
      break;
      
    default:
      Serial.println("未知指令");
      playErrorSound();
      break;
  }
}

void turnOnLED() {
  digitalWrite(LED_PIN, HIGH);
  status.ledState = true;
  Serial.println("LED已开启");
  playSuccessSound();
}

void turnOffLED() {
  digitalWrite(LED_PIN, LOW);
  status.ledState = false;
  Serial.println("LED已关闭");
  playSuccessSound();
}

void servoRotateLeft() {
  status.servoAngle = max(0, status.servoAngle - 30);
  int pwmValue = map(status.servoAngle, 0, 180, 0, 255);
  analogWrite(SERVO_PIN, pwmValue);
  Serial.print("舵机左转至: ");
  Serial.println(status.servoAngle);
  playSuccessSound();
}

void servoRotateRight() {
  status.servoAngle = min(180, status.servoAngle + 30);
  int pwmValue = map(status.servoAngle, 0, 180, 0, 255);
  analogWrite(SERVO_PIN, pwmValue);
  Serial.print("舵机右转至: ");
  Serial.println(status.servoAngle);
  playSuccessSound();
}

void motorForward() {
  digitalWrite(MOTOR_PIN1, HIGH);
  digitalWrite(MOTOR_PIN2, LOW);
  status.motorSpeed = 255;
  status.motorDirection = true;
  Serial.println("电机前进");
  playSuccessSound();
}

void motorBackward() {
  digitalWrite(MOTOR_PIN1, LOW);
  digitalWrite(MOTOR_PIN2, HIGH);
  status.motorSpeed = 255;
  status.motorDirection = false;
  Serial.println("电机后退");
  playSuccessSound();
}

void motorStop() {
  digitalWrite(MOTOR_PIN1, LOW);
  digitalWrite(MOTOR_PIN2, LOW);
  status.motorSpeed = 0;
  Serial.println("电机停止");
  playSuccessSound();
}

void reportStatus() {
  Serial.println("=== 系统状态 ===");
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
  Serial.println("================");
  playStatusSound();
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

void playErrorSound() {
  tone(BUZZER_PIN, 500, 300);
  delay(100);
  tone(BUZZER_PIN, 500, 300);
}

void playStatusSound() {
  tone(BUZZER_PIN, 1500, 100);
  delay(100);
  tone(BUZZER_PIN, 1500, 100);
  delay(100);
  tone(BUZZER_PIN, 1500, 100);
} 