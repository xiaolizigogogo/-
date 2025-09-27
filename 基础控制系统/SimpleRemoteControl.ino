/*
 * 焊接环境无线遥控器控制
 * 使用433MHz遥控器，适合噪音环境
 * 支持自动编码检测和适配
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

// 遥控器编码存储（动态检测）
long remoteCodes[12] = {0}; // 存储12个按键的编码
bool codesDetected = false;
int currentKeyIndex = 0;

// 按键功能映射
const char* keyFunctions[] = {
  "LED开关", "左转", "右转", "前进", "后退", "停止",
  "状态查询", "重置", "加速", "减速", "紧急停止", "取消配对"
};

// 系统状态
int servoAngle = 90;
int motorSpeed = 0;
bool ledState = false;
bool motorDirection = true;
bool learningMode = false;

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
  
  Serial.println("=== 433MHz遥控器自动适配系统 ===");
  Serial.println("等待遥控器信号...");
  Serial.println("输入 'learn' 进入学习模式");
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
  
  // 检查遥控器信号
  if (mySwitch.available()) {
    long value = mySwitch.getReceivedValue();
    Serial.print("收到遥控器信号: ");
    Serial.println(value);
    
    if (learningMode) {
      // 学习模式：记录编码
      learnRemoteCode(value);
    } else if (codesDetected) {
      // 正常模式：处理指令
      processRemoteCommand(value);
    } else {
      Serial.println("请先进入学习模式配置遥控器编码");
    }
    
    // 重置接收器
    mySwitch.resetAvailable();
    
    // 防抖延时
    delay(200);
  }
  
  delay(10);
}

void processSerialCommand(String command) {
  if (command == "learn") {
    startLearningMode();
  } else if (command == "status") {
    showStatus();
  } else if (command == "reset") {
    resetLearning();
  } else if (command == "help") {
    showHelp();
  } else {
    Serial.println("未知命令，输入 'help' 查看帮助");
  }
}

void startLearningMode() {
  learningMode = true;
  currentKeyIndex = 0;
  Serial.println("=== 进入学习模式 ===");
  Serial.println("请按顺序按下遥控器按键：");
  Serial.println("1. LED开关  2. 左转  3. 右转  4. 前进");
  Serial.println("5. 后退     6. 停止  7. 状态  8. 重置");
  Serial.println("9. 加速    10. 减速 11. 紧急停止 12. 取消配对");
  Serial.println("当前等待按键: " + String(currentKeyIndex + 1) + " - " + keyFunctions[currentKeyIndex]);
  playLearningSound();
}

void learnRemoteCode(long code) {
  if (currentKeyIndex < 12) {
    remoteCodes[currentKeyIndex] = code;
    Serial.print("按键 ");
    Serial.print(currentKeyIndex + 1);
    Serial.print(" (");
    Serial.print(keyFunctions[currentKeyIndex]);
    Serial.print(") 编码: ");
    Serial.println(code);
    
    currentKeyIndex++;
    
    if (currentKeyIndex < 12) {
      Serial.println("请按下个按键: " + String(currentKeyIndex + 1) + " - " + keyFunctions[currentKeyIndex]);
      playSuccessSound();
    } else {
      // 学习完成
      codesDetected = true;
      learningMode = false;
      Serial.println("=== 学习完成！===");
      Serial.println("遥控器已配置，可以正常使用");
      showLearnedCodes();
      playCompleteSound();
    }
  }
}

void processRemoteCommand(long command) {
  // 查找匹配的编码
  for (int i = 0; i < 12; i++) {
    if (remoteCodes[i] == command) {
      Serial.print("执行功能: ");
      Serial.println(keyFunctions[i]);
      
      switch (i) {
        case 0: toggleLED(); break;
        case 1: servoLeft(); break;
        case 2: servoRight(); break;
        case 3: motorForward(); break;
        case 4: motorBackward(); break;
        case 5: motorStop(); break;
        case 6: reportStatus(); break;
        case 7: resetSystem(); break;
        case 8: motorAccelerate(); break;
        case 9: motorDecelerate(); break;
        case 10: emergencyStop(); break;
        case 11: cancelPairing(); break;
      }
      return;
    }
  }
  
  Serial.println("未知指令，未找到匹配的编码");
  playErrorSound();
}

void showLearnedCodes() {
  Serial.println("=== 已学习的编码 ===");
  for (int i = 0; i < 12; i++) {
    Serial.print("按键 ");
    Serial.print(i + 1);
    Serial.print(" (");
    Serial.print(keyFunctions[i]);
    Serial.print("): ");
    Serial.println(remoteCodes[i]);
  }
  Serial.println("==================");
}

void showStatus() {
  Serial.println("=== 系统状态 ===");
  Serial.print("学习模式: ");
  Serial.println(learningMode ? "是" : "否");
  Serial.print("编码已检测: ");
  Serial.println(codesDetected ? "是" : "否");
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

void resetLearning() {
  codesDetected = false;
  learningMode = false;
  currentKeyIndex = 0;
  for (int i = 0; i < 12; i++) {
    remoteCodes[i] = 0;
  }
  Serial.println("学习数据已重置");
}

void showHelp() {
  Serial.println("=== 命令帮助 ===");
  Serial.println("learn  - 进入学习模式");
  Serial.println("status - 查看系统状态");
  Serial.println("reset  - 重置学习数据");
  Serial.println("help   - 显示此帮助");
  Serial.println("================");
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

void motorAccelerate() {
  motorSpeed = min(255, motorSpeed + 50);
  Serial.print("电机加速，当前速度: ");
  Serial.println(motorSpeed);
  playSuccessSound();
}

void motorDecelerate() {
  motorSpeed = max(0, motorSpeed - 50);
  Serial.print("电机减速，当前速度: ");
  Serial.println(motorSpeed);
  playSuccessSound();
}

void emergencyStop() {
  motorStop();
  ledState = false;
  digitalWrite(LED_PIN, LOW);
  servoAngle = 90;
  analogWrite(SERVO_PIN, map(servoAngle, 0, 180, 0, 255));
  Serial.println("紧急停止！所有设备已停止");
  playEmergencySound();
}

void cancelPairing() {
  resetLearning();
  Serial.println("配对已取消");
  playResetSound();
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

void playLearningSound() {
  tone(BUZZER_PIN, 800, 100);
  delay(100);
  tone(BUZZER_PIN, 1200, 100);
  delay(100);
  tone(BUZZER_PIN, 1600, 100);
}

void playCompleteSound() {
  tone(BUZZER_PIN, 2000, 200);
  delay(200);
  tone(BUZZER_PIN, 1500, 200);
  delay(200);
  tone(BUZZER_PIN, 1000, 200);
}

void playEmergencySound() {
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 500, 200);
    delay(200);
    tone(BUZZER_PIN, 1000, 200);
    delay(200);
  }
}

void playResetSound() {
  tone(BUZZER_PIN, 800, 200);
  delay(200);
  tone(BUZZER_PIN, 600, 200);
  delay(200);
  tone(BUZZER_PIN, 400, 200);
} 