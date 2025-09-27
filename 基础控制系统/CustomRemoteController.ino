/*
 * 自定义433MHz遥控器端程序
 * 带液晶显示的双向通信遥控器
 */

#include <RCSwitch.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// 硬件定义
#define BUZZER_PIN 8

// 433MHz一体模块引脚
#define TX_EN_PIN 7    // 发射使能引脚
#define RX_EN_PIN 4    // 接收使能引脚
#define DATA_PIN 2     // 数据引脚

// 按键引脚
#define KEY_UP 9       // 上按键
#define KEY_DOWN 10    // 下按键
#define KEY_LEFT 11    // 左按键
#define KEY_RIGHT 12   // 右按键
#define KEY_OK 13      // 确认按键
#define KEY_BACK A0    // 返回按键

// 无线通信
RCSwitch mySwitch = RCSwitch();

// LCD显示屏 (I2C地址0x27, 16x2字符)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// 设备配置
#define REMOTE_ID 1    // 遥控器ID
#define DEVICE_ID 2    // 设备ID

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
#define CMD_SPEED_SET  0x0C
#define CMD_ANGLE_SET  0x0D
#define CMD_MODE_SET   0x0E
#define CMD_DATA_REQ   0x0F
#define CMD_DATA_RESP  0x10

// 显示状态
int currentScreen = 0;  // 0=主界面, 1=状态, 2=设置, 3=数据, 4=系统
int selectedItem = 0;
int menuDepth = 0;

// 设备状态
bool deviceConnected = false;
bool ledState = false;
bool motorRunning = false;
bool motorDirection = true;
int motorSpeed = 0;
int servoAngle = 90;
int workMode = 0;
int batteryLevel = 100;
int signalStrength = 100;
float temperature = 25.0;
unsigned long workTime = 0;
unsigned long lastStatusUpdate = 0;
unsigned long lastHeartbeat = 0;

// 设置参数
int targetSpeed = 150;
int targetAngle = 90;
int targetMode = 0;

// 菜单项
const char* mainMenu[] = {
  "状态监控", "参数设置", "数据查看", "系统信息"
};

const char* statusMenu[] = {
  "设备状态", "运行参数", "系统信息", "返回"
};

const char* settingsMenu[] = {
  "速度设置", "角度设置", "模式设置", "返回"
};

const char* dataMenu[] = {
  "温度数据", "电量数据", "工作时间", "返回"
};

void setup() {
  Serial.begin(9600);
  
  // 初始化433MHz一体模块
  pinMode(TX_EN_PIN, OUTPUT);
  pinMode(RX_EN_PIN, OUTPUT);
  setReceiveMode();
  
  // 初始化无线通信
  mySwitch.enableReceive(digitalPinToInterrupt(DATA_PIN));
  
  // 初始化按键
  pinMode(KEY_UP, INPUT_PULLUP);
  pinMode(KEY_DOWN, INPUT_PULLUP);
  pinMode(KEY_LEFT, INPUT_PULLUP);
  pinMode(KEY_RIGHT, INPUT_PULLUP);
  pinMode(KEY_OK, INPUT_PULLUP);
  pinMode(KEY_BACK, INPUT_PULLUP);
  
  // 初始化蜂鸣器
  pinMode(BUZZER_PIN, OUTPUT);
  
  // 初始化LCD
  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  // 显示启动信息
  lcd.setCursor(0, 0);
  lcd.print("Custom 433MHz");
  lcd.setCursor(0, 1);
  lcd.print("Remote Control");
  
  Serial.println("=== 自定义433MHz遥控器启动 ===");
  Serial.println("等待设备连接...");
  
  delay(2000);
  
  // 显示主界面
  showMainScreen();
  
  // 启动提示音
  playStartupSound();
}

void loop() {
  // 检查按键输入
  checkButtons();
  
  // 检查接收到的信号
  if (mySwitch.available()) {
    long value = mySwitch.getReceivedValue();
    processReceivedData(value);
    mySwitch.resetAvailable();
  }
  
  // 定期请求状态
  if (millis() - lastStatusUpdate > 3000) {
    requestStatus();
    lastStatusUpdate = millis();
  }
  
  // 定期发送心跳
  if (millis() - lastHeartbeat > 10000) {
    sendHeartbeat();
    lastHeartbeat = millis();
  }
  
  // 更新显示
  updateDisplay();
  
  delay(100);
}

void checkButtons() {
  // 上按键
  if (digitalRead(KEY_UP) == LOW) {
    handleUpButton();
    delay(200);
  }
  
  // 下按键
  if (digitalRead(KEY_DOWN) == LOW) {
    handleDownButton();
    delay(200);
  }
  
  // 左按键
  if (digitalRead(KEY_LEFT) == LOW) {
    handleLeftButton();
    delay(200);
  }
  
  // 右按键
  if (digitalRead(KEY_RIGHT) == LOW) {
    handleRightButton();
    delay(200);
  }
  
  // 确认按键
  if (digitalRead(KEY_OK) == LOW) {
    handleOkButton();
    delay(200);
  }
  
  // 返回按键
  if (digitalRead(KEY_BACK) == LOW) {
    handleBackButton();
    delay(200);
  }
}

void handleUpButton() {
  if (currentScreen == 0) {
    // 主界面：发送前进命令
    sendCommand(CMD_MOTOR_FWD, 0);
  } else if (currentScreen == 2) {
    // 设置界面：增加选中项的值
    adjustSetting(1);
  } else {
    // 菜单界面：上一项
    selectedItem = max(0, selectedItem - 1);
  }
  playSuccessSound();
}

void handleDownButton() {
  if (currentScreen == 0) {
    // 主界面：发送后退命令
    sendCommand(CMD_MOTOR_BWD, 0);
  } else if (currentScreen == 2) {
    // 设置界面：减少选中项的值
    adjustSetting(-1);
  } else {
    // 菜单界面：下一项
    int maxItems = getMaxMenuItems();
    selectedItem = min(maxItems - 1, selectedItem + 1);
  }
  playSuccessSound();
}

void handleLeftButton() {
  if (currentScreen == 0) {
    // 主界面：发送左转命令
    sendCommand(CMD_SERVO_LEFT, 0);
  } else if (currentScreen == 2) {
    // 设置界面：上一项
    selectedItem = max(0, selectedItem - 1);
  }
  playSuccessSound();
}

void handleRightButton() {
  if (currentScreen == 0) {
    // 主界面：发送右转命令
    sendCommand(CMD_SERVO_RIGHT, 0);
  } else if (currentScreen == 2) {
    // 设置界面：下一项
    int maxItems = getMaxMenuItems();
    selectedItem = min(maxItems - 1, selectedItem + 1);
  }
  playSuccessSound();
}

void handleOkButton() {
  if (currentScreen == 0) {
    // 主界面：进入菜单
    currentScreen = 1;
    selectedItem = 0;
  } else if (currentScreen == 1) {
    // 状态菜单：进入子菜单
    switch (selectedItem) {
      case 0: currentScreen = 1; break; // 设备状态
      case 1: currentScreen = 2; break; // 运行参数
      case 2: currentScreen = 4; break; // 系统信息
      case 3: currentScreen = 0; break; // 返回
    }
    selectedItem = 0;
  } else if (currentScreen == 2) {
    // 设置界面：确认设置
    applySettings();
  }
  playSuccessSound();
}

void handleBackButton() {
  if (currentScreen > 0) {
    currentScreen--;
    selectedItem = 0;
  }
  playSuccessSound();
}

int getMaxMenuItems() {
  switch (currentScreen) {
    case 1: return 4; // 状态菜单
    case 2: return 4; // 设置菜单
    case 3: return 4; // 数据菜单
    default: return 4;
  }
}

void adjustSetting(int delta) {
  switch (selectedItem) {
    case 0: // 速度设置
      targetSpeed = constrain(targetSpeed + delta * 10, 0, 255);
      break;
    case 1: // 角度设置
      targetAngle = constrain(targetAngle + delta * 5, 0, 180);
      break;
    case 2: // 模式设置
      targetMode = (targetMode + delta + 3) % 3;
      break;
  }
}

void applySettings() {
  // 发送设置命令
  sendCommand(CMD_SPEED_SET, targetSpeed);
  sendCommand(CMD_ANGLE_SET, targetAngle);
  sendCommand(CMD_MODE_SET, targetMode);
}

void sendCommand(int command, int data) {
  setTransmitMode();
  delay(10);
  
  long packet = buildPacket(REMOTE_ID, command, data);
  mySwitch.send(packet, 24);
  
  Serial.print("发送命令: ");
  Serial.print(command);
  Serial.print(" 数据: ");
  Serial.println(data);
  
  setReceiveMode();
  delay(10);
}

void requestStatus() {
  sendCommand(CMD_STATUS_REQ, 0);
}

void sendHeartbeat() {
  sendCommand(CMD_HEARTBEAT, REMOTE_ID);
}

void processReceivedData(long value) {
  int senderId = (value >> 16) & 0xFF;
  int command = (value >> 8) & 0xFF;
  int data = value & 0xFF;
  
  Serial.print("来自设备 ");
  Serial.print(senderId);
  Serial.print(" 命令: ");
  Serial.print(command);
  Serial.print(" 数据: ");
  Serial.println(data);
  
  switch (command) {
    case CMD_STATUS_RESP:
      updateDeviceStatus(data);
      deviceConnected = true;
      break;
    case CMD_DATA_RESP:
      updateDeviceData(data);
      break;
    case CMD_HEARTBEAT:
      deviceConnected = true;
      break;
  }
}

void updateDeviceStatus(int status) {
  ledState = (status & 0x01) != 0;
  motorRunning = (status & 0x02) != 0;
  motorDirection = (status & 0x04) != 0;
  // 更新其他状态信息
}

void updateDeviceData(int data) {
  // 更新设备数据
  temperature = data * 0.5; // 示例：数据转换为温度
}

long buildPacket(int deviceId, int command, int data) {
  return ((long)deviceId << 16) | ((long)command << 8) | data;
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

// 显示函数
void showMainScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("433MHz Remote");
  lcd.setCursor(0, 1);
  if (deviceConnected) {
    lcd.print("Connected");
  } else {
    lcd.print("Disconnected");
  }
}

void updateDisplay() {
  switch (currentScreen) {
    case 0:
      showMainScreen();
      break;
    case 1:
      showStatusScreen();
      break;
    case 2:
      showSettingsScreen();
      break;
    case 3:
      showDataScreen();
      break;
    case 4:
      showSystemScreen();
      break;
  }
}

void showStatusScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Status Monitor");
  lcd.setCursor(0, 1);
  
  switch (selectedItem) {
    case 0:
      lcd.print("LED: ");
      lcd.print(ledState ? "ON " : "OFF");
      break;
    case 1:
      lcd.print("Motor: ");
      lcd.print(motorSpeed);
      break;
    case 2:
      lcd.print("Servo: ");
      lcd.print(servoAngle);
      break;
    case 3:
      lcd.print("Mode: ");
      lcd.print(workMode);
      break;
  }
}

void showSettingsScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Settings");
  lcd.setCursor(0, 1);
  
  switch (selectedItem) {
    case 0:
      lcd.print("Speed: ");
      lcd.print(targetSpeed);
      break;
    case 1:
      lcd.print("Angle: ");
      lcd.print(targetAngle);
      break;
    case 2:
      lcd.print("Mode: ");
      lcd.print(targetMode);
      break;
    case 3:
      lcd.print("Back");
      break;
  }
}

void showDataScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Data Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print(temperature, 1);
  lcd.print("C");
}

void showSystemScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Info");
  lcd.setCursor(0, 1);
  lcd.print("Batt: ");
  lcd.print(batteryLevel);
  lcd.print("%");
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