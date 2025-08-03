/*
 * 焊接环境无线遥控器控制
 * 使用433MHz遥控器，适合噪音环境
 */

#include <RCSwitch.h>

// 硬件定义
#define LED_PIN 13
#define SERVO_PIN 9
#define MOTOR_PIN1 5
#define MOTOR_PIN2 6
#define BUZZER_PIN 8

// 无线遥控器
RCSwitch mySwitch = RCSwitch();

// 遥控器按键代码（需要根据实际遥控器调整）
#define KEY_LED 12345      // 开灯/关灯
#define KEY_LEFT 12346     // 左转
#define KEY_RIGHT 12347    // 右转
#define KEY_FORWARD 12348  // 前进
#define KEY_BACKWARD 12349 // 后退
#define KEY_STOP 12350     // 停止
#define KEY_STATUS 12351   // 状态
#define KEY_RESET 12352    // 重置

// 系统状态
int servoAngle = 90;
int motorSpeed = 0;
bool ledState = false;
bool motorDirection = true;

void setup() {
  Serial.begin(9600);
  
  // 初始化无线接收器
  mySwitch.enableReceive(0); // 使用中断引脚0
  
  // 初始化引脚
  pinMode(LED_PIN, OUTPUT);
  pinMode(SERVO_PIN, OUTPUT);
  pinMode(MOTOR_PIN1, OUTPUT);
  pinMode(MOTOR_PIN2, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // 初始化舵机
  analogWrite(SERVO_PIN, map(servoAngle, 0, 180, 0, 255));
  
  Serial.println("无线遥控器控制系统已启动");
  Serial.println("等待遥控器信号...");
  
  // 启动提示音
  playStartupSound();
}

void loop() {
  // 检查遥控器信号
  if (mySwitch.available()) {
    long value = mySwitch.getReceivedValue();
    Serial.print("收到遥控器指令: ");
    Serial.println(value);
    
    // 处理遥控器指令
    processRemoteCommand(value);
    
    // 重置接收器
    mySwitch.resetAvailable();
    
    // 防抖延时
    delay(200);
  }
  
  delay(10);
}

void processRemoteCommand(long command) {
  switch (command) {
    case KEY_LED:
      toggleLED();
      break;
      
    case KEY_LEFT:
      servoLeft();
      break;
      
    case KEY_RIGHT:
      servoRight();
      break;
      
    case KEY_FORWARD:
      motorForward();
      break;
      
    case KEY_BACKWARD:
      motorBackward();
      break;
      
    case KEY_STOP:
      motorStop();
      break;
      
    case KEY_STATUS:
      reportStatus();
      break;
      
    case KEY_RESET:
      resetSystem();
      break;
      
    default:
      Serial.println("未知指令");
      playErrorSound();
      break;
  }
}

void toggleLED() {
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  Serial.println(ledState ? "LED已开启" : "LED已关闭");
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

void reportStatus() {
  Serial.println("=== 系统状态 ===");
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
  playStatusSound();
}

void resetSystem() {
  // 重置状态
  servoAngle = 90;
  motorSpeed = 0;
  ledState = false;
  
  // 重置硬件
  analogWrite(SERVO_PIN, map(servoAngle, 0, 180, 0, 255));
  motorStop();
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("系统已重置");
  playResetSound();
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

void playResetSound() {
  tone(BUZZER_PIN, 800, 200);
  delay(200);
  tone(BUZZER_PIN, 600, 200);
  delay(200);
  tone(BUZZER_PIN, 400, 200);
} 