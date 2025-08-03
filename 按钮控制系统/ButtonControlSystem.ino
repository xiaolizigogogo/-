/*
 * 按钮控制系统 - 替代旋钮方案
 * 集成遥控器配对功能
 * 
 * 功能：
 * 1. 7个参数的加减按钮控制
 * 2. 数码管实时显示参数值
 * 3. 遥控器配对和参数控制
 * 4. 参数保存到EEPROM
 * 5. 长按快速调节功能
 */

#include <RCSwitch.h>
#include <EEPROM.h>
#include <Servo.h>

// ==================== 硬件引脚定义 ====================

// 按钮引脚（每个参数2个按钮，共14个）
const int buttonPins[14] = {
  2, 3,   // 参数1: 转向角度 +/-
  4, 5,   // 参数2: 转向速度 +/-
  6, 7,   // 参数3: 电机速度 +/-
  8, 9,   // 参数4: 伺服角度 +/-
  10, 11, // 参数5: LED亮度 +/-
  12, 13, // 参数6: 延时时间 +/-
  A0, A1  // 参数7: 其他参数 +/-
};

// 数码管引脚（使用74HC595驱动）
#define DISPLAY_DATA_PIN   A2  // 数据引脚
#define DISPLAY_CLOCK_PIN  A3  // 时钟引脚
#define DISPLAY_LATCH_PIN  A4  // 锁存引脚
#define DISPLAY_ENABLE_PIN A5  // 使能引脚

// 状态LED引脚
#define LED_RED_PIN    A6
#define LED_YELLOW_PIN A7
#define LED_GREEN_PIN  A8

// 功能按钮引脚
#define BTN_PAIR_PIN    A9    // 配对按钮
#define BTN_RESET_PIN   A10   // 重置按钮
#define BTN_EMERGENCY_PIN A11 // 紧急停止按钮

// 输出控制引脚
#define SERVO_PIN       A12
#define MOTOR_PIN1      A13
#define MOTOR_PIN2      A14
#define LED_PIN         A15

// ==================== 遥控器相关定义 ====================

#define RCSWITCH_PIN    A16   // 433MHz接收引脚

RCSwitch mySwitch = RCSwitch();

// 配对状态
enum PairingState {
  UNPAIRED,
  WAITING,
  PAIRED
};

struct SystemStatus {
  PairingState pairingState = UNPAIRED;
  unsigned long pairedRemoteId = 0;
  unsigned long deviceId = 0;
  bool emergencyStop = false;
  
  // 按钮控制相关
  int currentDisplayParam = 0;  // 当前显示的参数（用于紧凑配置）
  bool displayMode = false;     // 显示模式（false=独立显示，true=分时显示）
};

SystemStatus status;

// ==================== 参数控制结构 ====================

struct ParameterControl {
  int currentValue;        // 当前值
  int minValue;           // 最小值
  int maxValue;           // 最大值
  int stepSize;           // 步长
  char name[16];          // 参数名称
  bool changed;           // 是否被修改
  unsigned long lastChange; // 最后修改时间
};

struct ButtonState {
  bool plusPressed;       // 增加按钮状态
  bool minusPressed;      // 减少按钮状态
  unsigned long lastPress; // 最后按下时间
  bool longPress;         // 长按状态
};

// 参数定义
ParameterControl parameters[7] = {
  {0, 0, 180, 5, "转向角度", false, 0},    // 0-180度，步长5
  {0, 0, 255, 10, "转向速度", false, 0},   // 0-255，步长10
  {0, 0, 255, 10, "电机速度", false, 0},   // 0-255，步长10
  {90, 0, 180, 5, "伺服角度", false, 0},   // 0-180度，步长5
  {128, 0, 255, 20, "LED亮度", false, 0},  // 0-255，步长20
  {1000, 100, 10000, 500, "延时", false, 0}, // 100-10000ms，步长500
  {0, 0, 100, 5, "其他参数", false, 0}     // 0-100，步长5
};

ButtonState buttonStates[7];

// 时间常量
const unsigned long LONG_PRESS_TIME = 500;    // 长按时间（毫秒）
const unsigned long FAST_REPEAT_TIME = 100;   // 快速重复时间（毫秒）
const unsigned long DEBOUNCE_TIME = 50;       // 防抖时间（毫秒）

// 数码管段码定义（共阴极）
const byte digitPatterns[10] = {
  0x3F, // 0
  0x06, // 1
  0x5B, // 2
  0x4F, // 3
  0x66, // 4
  0x6D, // 5
  0x7D, // 6
  0x07, // 7
  0x7F, // 8
  0x6F  // 9
};

// 硬件对象
Servo myServo;

// ==================== 遥控器命令定义 ====================

enum RemoteCommands {
  CMD_PAIR_REQUEST = 0x01,
  CMD_PAIR_CONFIRM = 0x02,
  CMD_EMERGENCY_STOP = 0x03,
  
  // 参数控制命令
  CMD_PARAM_INCREASE = 0x10,  // 参数增加
  CMD_PARAM_DECREASE = 0x11,  // 参数减少
  CMD_PARAM_SET = 0x12,       // 参数直接设置
  CMD_PARAM_RESET = 0x13,     // 参数重置
  
  // 功能控制命令
  CMD_MOTOR_FORWARD = 0x20,
  CMD_MOTOR_BACKWARD = 0x21,
  CMD_MOTOR_STOP = 0x22,
  CMD_SERVO_SET = 0x23,
  CMD_LED_SET = 0x24
};

// ==================== 初始化函数 ====================

void setup() {
  Serial.begin(9600);
  Serial.println("按钮控制系统启动");
  
  // 初始化引脚
  initializePins();
  
  // 初始化遥控器
  initializeRemoteControl();
  
  // 初始化显示
  initializeDisplay();
  
  // 初始化硬件
  initializeHardware();
  
  // 加载保存的参数
  loadParameters();
  
  // 更新显示
  updateAllDisplays();
  
  Serial.println("系统初始化完成");
}

void initializePins() {
  // 初始化按钮引脚
  for (int i = 0; i < 14; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  
  // 初始化功能按钮
  pinMode(BTN_PAIR_PIN, INPUT_PULLUP);
  pinMode(BTN_RESET_PIN, INPUT_PULLUP);
  pinMode(BTN_EMERGENCY_PIN, INPUT_PULLUP);
  
  // 初始化LED引脚
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_YELLOW_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  
  // 初始化数码管引脚
  pinMode(DISPLAY_DATA_PIN, OUTPUT);
  pinMode(DISPLAY_CLOCK_PIN, OUTPUT);
  pinMode(DISPLAY_LATCH_PIN, OUTPUT);
  pinMode(DISPLAY_ENABLE_PIN, OUTPUT);
  
  // 初始化输出引脚
  pinMode(MOTOR_PIN1, OUTPUT);
  pinMode(MOTOR_PIN2, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
}

void initializeRemoteControl() {
  mySwitch.enableReceive(RCSWITCH_PIN);
  
  // 从EEPROM加载配对信息
  loadPairingInfo();
  
  Serial.print("设备ID: ");
  Serial.println(status.deviceId);
  Serial.print("配对状态: ");
  Serial.println(status.pairingState == PAIRED ? "已配对" : "未配对");
}

void initializeDisplay() {
  digitalWrite(DISPLAY_ENABLE_PIN, LOW);  // 启用显示
  clearAllDisplays();
}

void initializeHardware() {
  myServo.attach(SERVO_PIN);
  myServo.write(parameters[3].currentValue);  // 设置伺服初始位置
}

// ==================== 主循环 ====================

void loop() {
  // 处理遥控器信号
  handleRemoteControl();
  
  // 处理本地按钮
  handleLocalButtons();
  
  // 处理参数调节按钮
  handleParameterButtons();
  
  // 更新LED状态
  updateLEDStatus();
  
  // 应用参数到硬件
  applyParameters();
  
  // 处理紧急停止
  handleEmergencyStop();
  
  delay(10);  // 短暂延时
}

// ==================== 按钮处理函数 ====================

void handleLocalButtons() {
  // 配对按钮
  if (digitalRead(BTN_PAIR_PIN) == LOW) {
    if (status.pairingState == UNPAIRED) {
      startPairing();
    } else if (status.pairingState == PAIRED) {
      toggleDisplayMode();
    }
    delay(200);  // 防抖
  }
  
  // 重置按钮
  if (digitalRead(BTN_RESET_PIN) == LOW) {
    if (status.pairingState == PAIRED) {
      resetAllParameters();
    } else {
      cancelPairing();
    }
    delay(200);  // 防抖
  }
  
  // 紧急停止按钮
  if (digitalRead(BTN_EMERGENCY_PIN) == LOW) {
    emergencyStop();
    delay(200);  // 防抖
  }
}

void handleParameterButtons() {
  for (int i = 0; i < 7; i++) {
    int plusPin = buttonPins[i * 2];
    int minusPin = buttonPins[i * 2 + 1];
    
    // 检测增加按钮
    if (digitalRead(plusPin) == LOW) {
      if (!buttonStates[i].plusPressed) {
        buttonStates[i].plusPressed = true;
        buttonStates[i].lastPress = millis();
        increaseParameter(i);
      } else if (millis() - buttonStates[i].lastPress > LONG_PRESS_TIME) {
        // 长按快速增加
        if (millis() - buttonStates[i].lastPress > FAST_REPEAT_TIME) {
          increaseParameter(i);
          buttonStates[i].lastPress = millis();
        }
      }
    } else {
      buttonStates[i].plusPressed = false;
    }
    
    // 检测减少按钮
    if (digitalRead(minusPin) == LOW) {
      if (!buttonStates[i].minusPressed) {
        buttonStates[i].minusPressed = true;
        buttonStates[i].lastPress = millis();
        decreaseParameter(i);
      } else if (millis() - buttonStates[i].lastPress > LONG_PRESS_TIME) {
        // 长按快速减少
        if (millis() - buttonStates[i].lastPress > FAST_REPEAT_TIME) {
          decreaseParameter(i);
          buttonStates[i].lastPress = millis();
        }
      }
    } else {
      buttonStates[i].minusPressed = false;
    }
  }
}

// ==================== 参数控制函数 ====================

void increaseParameter(int paramIndex) {
  if (parameters[paramIndex].currentValue < parameters[paramIndex].maxValue) {
    parameters[paramIndex].currentValue += parameters[paramIndex].stepSize;
    if (parameters[paramIndex].currentValue > parameters[paramIndex].maxValue) {
      parameters[paramIndex].currentValue = parameters[paramIndex].maxValue;
    }
    parameters[paramIndex].changed = true;
    parameters[paramIndex].lastChange = millis();
    
    updateDisplay(paramIndex);
    saveParameter(paramIndex);
    
    Serial.print("参数");
    Serial.print(paramIndex + 1);
    Serial.print("(");
    Serial.print(parameters[paramIndex].name);
    Serial.print(") 增加至: ");
    Serial.println(parameters[paramIndex].currentValue);
  }
}

void decreaseParameter(int paramIndex) {
  if (parameters[paramIndex].currentValue > parameters[paramIndex].minValue) {
    parameters[paramIndex].currentValue -= parameters[paramIndex].stepSize;
    if (parameters[paramIndex].currentValue < parameters[paramIndex].minValue) {
      parameters[paramIndex].currentValue = parameters[paramIndex].minValue;
    }
    parameters[paramIndex].changed = true;
    parameters[paramIndex].lastChange = millis();
    
    updateDisplay(paramIndex);
    saveParameter(paramIndex);
    
    Serial.print("参数");
    Serial.print(paramIndex + 1);
    Serial.print("(");
    Serial.print(parameters[paramIndex].name);
    Serial.print(") 减少至: ");
    Serial.println(parameters[paramIndex].currentValue);
  }
}

void resetAllParameters() {
  for (int i = 0; i < 7; i++) {
    parameters[i].currentValue = getDefaultValue(i);
    parameters[i].changed = false;
    updateDisplay(i);
    saveParameter(i);
  }
  Serial.println("所有参数已重置为默认值");
}

int getDefaultValue(int paramIndex) {
  switch (paramIndex) {
    case 0: return 0;      // 转向角度
    case 1: return 0;      // 转向速度
    case 2: return 0;      // 电机速度
    case 3: return 90;     // 伺服角度
    case 4: return 128;    // LED亮度
    case 5: return 1000;   // 延时时间
    case 6: return 0;      // 其他参数
    default: return 0;
  }
}

// ==================== 显示控制函数 ====================

void updateDisplay(int paramIndex) {
  int value = parameters[paramIndex].currentValue;
  
  if (!status.displayMode) {
    // 独立显示模式
    if (paramIndex < 4) {
      displayNumber(paramIndex, value);
    }
  } else {
    // 分时显示模式
    if (status.currentDisplayParam == paramIndex) {
      displayNumber(3, value);
    }
  }
}

void updateAllDisplays() {
  for (int i = 0; i < 7; i++) {
    updateDisplay(i);
  }
}

void displayNumber(int displayIndex, int number) {
  // 限制显示范围
  if (number > 99) number = 99;
  if (number < 0) number = 0;
  
  int tens = number / 10;
  int ones = number % 10;
  
  // 发送数据到74HC595
  digitalWrite(DISPLAY_LATCH_PIN, LOW);
  shiftOut(DISPLAY_DATA_PIN, DISPLAY_CLOCK_PIN, MSBFIRST, digitPatterns[ones]);
  shiftOut(DISPLAY_DATA_PIN, DISPLAY_CLOCK_PIN, MSBFIRST, digitPatterns[tens]);
  digitalWrite(DISPLAY_LATCH_PIN, HIGH);
}

void clearAllDisplays() {
  digitalWrite(DISPLAY_LATCH_PIN, LOW);
  shiftOut(DISPLAY_DATA_PIN, DISPLAY_CLOCK_PIN, MSBFIRST, 0x00);
  shiftOut(DISPLAY_DATA_PIN, DISPLAY_CLOCK_PIN, MSBFIRST, 0x00);
  digitalWrite(DISPLAY_LATCH_PIN, HIGH);
}

void toggleDisplayMode() {
  status.displayMode = !status.displayMode;
  if (status.displayMode) {
    status.currentDisplayParam = 4;  // 从参数4开始显示
  }
  updateAllDisplays();
  Serial.println(status.displayMode ? "切换到分时显示模式" : "切换到独立显示模式");
}

// ==================== 遥控器处理函数 ====================

void handleRemoteControl() {
  if (mySwitch.available()) {
    unsigned long value = mySwitch.getReceivedValue();
    mySwitch.resetAvailable();
    
    if (value != 0) {
      processRemoteCommand(value);
    }
  }
}

void processRemoteCommand(unsigned long value) {
  unsigned long remoteId = (value >> 16) & 0xFFFF;
  uint8_t command = value & 0xFF;
  
  Serial.print("收到遥控器命令: ID=");
  Serial.print(remoteId);
  Serial.print(", CMD=0x");
  Serial.println(command, HEX);
  
  // 处理配对请求
  if (command == CMD_PAIR_REQUEST && status.pairingState == UNPAIRED) {
    status.pairingState = WAITING;
    status.pairedRemoteId = remoteId;
    savePairingInfo();
    Serial.println("等待配对确认...");
    return;
  }
  
  // 处理配对确认
  if (command == CMD_PAIR_CONFIRM && status.pairingState == WAITING) {
    if (remoteId == status.pairedRemoteId) {
      status.pairingState = PAIRED;
      savePairingInfo();
      Serial.println("配对成功！");
    }
    return;
  }
  
  // 检查是否已配对
  if (status.pairingState != PAIRED) {
    Serial.println("设备未配对，忽略命令");
    return;
  }
  
  // 检查遥控器ID
  if (remoteId != status.pairedRemoteId) {
    Serial.println("遥控器ID不匹配，忽略命令");
    return;
  }
  
  // 处理控制命令
  switch (command) {
    case CMD_EMERGENCY_STOP:
      emergencyStop();
      break;
      
    case CMD_PARAM_INCREASE:
      {
        uint8_t paramIndex = (value >> 8) & 0x07;
        if (paramIndex < 7) {
          increaseParameter(paramIndex);
        }
      }
      break;
      
    case CMD_PARAM_DECREASE:
      {
        uint8_t paramIndex = (value >> 8) & 0x07;
        if (paramIndex < 7) {
          decreaseParameter(paramIndex);
        }
      }
      break;
      
    case CMD_PARAM_SET:
      {
        uint8_t paramIndex = (value >> 8) & 0x07;
        uint8_t paramValue = (value >> 16) & 0xFF;
        if (paramIndex < 7) {
          setParameter(paramIndex, paramValue);
        }
      }
      break;
      
    case CMD_MOTOR_FORWARD:
      setMotor(parameters[2].currentValue, true);
      break;
      
    case CMD_MOTOR_BACKWARD:
      setMotor(parameters[2].currentValue, false);
      break;
      
    case CMD_MOTOR_STOP:
      setMotor(0, true);
      break;
      
    case CMD_SERVO_SET:
      myServo.write(parameters[3].currentValue);
      break;
      
    case CMD_LED_SET:
      analogWrite(LED_PIN, parameters[4].currentValue);
      break;
  }
}

void setParameter(int paramIndex, int value) {
  if (value >= parameters[paramIndex].minValue && 
      value <= parameters[paramIndex].maxValue) {
    parameters[paramIndex].currentValue = value;
    parameters[paramIndex].changed = true;
    parameters[paramIndex].lastChange = millis();
    
    updateDisplay(paramIndex);
    saveParameter(paramIndex);
    
    Serial.print("遥控器设置参数");
    Serial.print(paramIndex + 1);
    Serial.print("为: ");
    Serial.println(value);
  }
}

// ==================== 配对管理函数 ====================

void startPairing() {
  status.pairingState = WAITING;
  status.pairedRemoteId = 0;
  Serial.println("开始配对模式...");
}

void cancelPairing() {
  status.pairingState = UNPAIRED;
  status.pairedRemoteId = 0;
  savePairingInfo();
  Serial.println("取消配对");
}

void loadPairingInfo() {
  // 从EEPROM加载配对信息
  status.deviceId = EEPROM.read(0) | (EEPROM.read(1) << 8);
  status.pairingState = (PairingState)EEPROM.read(2);
  status.pairedRemoteId = EEPROM.read(3) | (EEPROM.read(4) << 8) | 
                         (EEPROM.read(5) << 16) | (EEPROM.read(6) << 24);
  
  // 如果设备ID为0，生成新的设备ID
  if (status.deviceId == 0) {
    status.deviceId = random(1, 65536);
    EEPROM.write(0, status.deviceId & 0xFF);
    EEPROM.write(1, (status.deviceId >> 8) & 0xFF);
  }
}

void savePairingInfo() {
  EEPROM.write(2, (uint8_t)status.pairingState);
  EEPROM.write(3, status.pairedRemoteId & 0xFF);
  EEPROM.write(4, (status.pairedRemoteId >> 8) & 0xFF);
  EEPROM.write(5, (status.pairedRemoteId >> 16) & 0xFF);
  EEPROM.write(6, (status.pairedRemoteId >> 24) & 0xFF);
}

// ==================== 参数存储函数 ====================

void loadParameters() {
  for (int i = 0; i < 7; i++) {
    int addr = 10 + i * 2;  // 从EEPROM地址10开始存储
    int value = EEPROM.read(addr) | (EEPROM.read(addr + 1) << 8);
    
    // 检查值的有效性
    if (value >= parameters[i].minValue && value <= parameters[i].maxValue) {
      parameters[i].currentValue = value;
    }
  }
}

void saveParameter(int paramIndex) {
  int addr = 10 + paramIndex * 2;
  EEPROM.write(addr, parameters[paramIndex].currentValue & 0xFF);
  EEPROM.write(addr + 1, (parameters[paramIndex].currentValue >> 8) & 0xFF);
}

// ==================== 硬件控制函数 ====================

void applyParameters() {
  // 应用伺服角度
  myServo.write(parameters[3].currentValue);
  
  // 应用LED亮度
  analogWrite(LED_PIN, parameters[4].currentValue);
  
  // 电机控制需要根据具体逻辑实现
  // setMotor(parameters[2].currentValue, direction);
}

void setMotor(int speed, bool forward) {
  if (speed == 0) {
    digitalWrite(MOTOR_PIN1, LOW);
    digitalWrite(MOTOR_PIN2, LOW);
  } else if (forward) {
    analogWrite(MOTOR_PIN1, speed);
    digitalWrite(MOTOR_PIN2, LOW);
  } else {
    digitalWrite(MOTOR_PIN1, LOW);
    analogWrite(MOTOR_PIN2, speed);
  }
}

void emergencyStop() {
  status.emergencyStop = true;
  
  // 停止所有输出
  digitalWrite(MOTOR_PIN1, LOW);
  digitalWrite(MOTOR_PIN2, LOW);
  analogWrite(LED_PIN, 0);
  
  Serial.println("紧急停止！");
}

void handleEmergencyStop() {
  if (status.emergencyStop) {
    // 紧急停止状态下，只有重置按钮可以恢复
    if (digitalRead(BTN_RESET_PIN) == LOW) {
      status.emergencyStop = false;
      Serial.println("紧急停止已解除");
      delay(200);
    }
  }
}

// ==================== LED状态指示 ====================

void updateLEDStatus() {
  // 关闭所有LED
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_YELLOW_PIN, LOW);
  digitalWrite(LED_GREEN_PIN, LOW);
  
  if (status.emergencyStop) {
    // 紧急停止：红灯快闪
    digitalWrite(LED_RED_PIN, (millis() / 200) % 2);
  } else if (status.pairingState == WAITING) {
    // 等待配对：黄灯慢闪
    digitalWrite(LED_YELLOW_PIN, (millis() / 1000) % 2);
  } else if (status.pairingState == PAIRED) {
    // 已配对：绿灯常亮
    digitalWrite(LED_GREEN_PIN, HIGH);
  } else {
    // 未配对：红灯常亮
    digitalWrite(LED_RED_PIN, HIGH);
  }
}

// ==================== 调试和状态显示 ====================

void printStatus() {
  Serial.println("\n=== 系统状态 ===");
  Serial.print("设备ID: ");
  Serial.println(status.deviceId);
  Serial.print("配对状态: ");
  switch (status.pairingState) {
    case UNPAIRED: Serial.println("未配对"); break;
    case WAITING: Serial.println("等待配对"); break;
    case PAIRED: Serial.println("已配对"); break;
  }
  Serial.print("配对遥控器ID: ");
  Serial.println(status.pairedRemoteId);
  Serial.print("显示模式: ");
  Serial.println(status.displayMode ? "分时显示" : "独立显示");
  Serial.print("紧急停止: ");
  Serial.println(status.emergencyStop ? "是" : "否");
  
  Serial.println("\n=== 参数状态 ===");
  for (int i = 0; i < 7; i++) {
    Serial.print("参数");
    Serial.print(i + 1);
    Serial.print("(");
    Serial.print(parameters[i].name);
    Serial.print("): ");
    Serial.print(parameters[i].currentValue);
    Serial.print("/");
    Serial.print(parameters[i].maxValue);
    Serial.print(" [");
    Serial.print(parameters[i].changed ? "已修改" : "未修改");
    Serial.println("]");
  }
  Serial.println("================\n");
} 