/*
 * 遥控器配对配置工具
 * 用于设置设备ID、测试配对和配置遥控器
 */

#include <KeyValueEEPROM.h>
#include <EEPROM.h>
#include <RCSwitch.h>

// 硬件定义
RCSwitch mySwitch = RCSwitch();

// 配置状态
struct ConfigState {
  uint8_t deviceId = 0x01;
  uint32_t remoteId = 0;
  bool isPaired = false;
  bool configMode = true;
} config;

// 串口命令处理
String inputString = "";
bool stringComplete = false;

void setup() {
  Serial.begin(9600);
  
  // 初始化无线接收器
  mySwitch.enableReceive(0);
  
  // 从EEPROM读取配置
  loadConfig();
  
  Serial.println("=== 遥控器配对配置工具 ===");
  Serial.println("支持的命令：");
  Serial.println("help - 显示帮助信息");
  Serial.println("status - 显示当前状态");
  Serial.println("set id <1-255> - 设置设备ID");
  Serial.println("pair - 进入配对模式");
  Serial.println("unpair - 取消配对");
  Serial.println("test - 测试遥控器信号");
  Serial.println("save - 保存配置");
  Serial.println("load - 加载配置");
  Serial.println("reset - 重置所有配置");
  Serial.println("==========================");
  
  printStatus();
}

void loop() {
  // 处理串口命令
  handleSerialCommands();
  
  // 处理遥控器信号
  handleRemoteSignals();
  
  delay(10);
}

void handleSerialCommands() {
  if (stringComplete) {
    inputString.trim();
    processCommand(inputString);
    inputString = "";
    stringComplete = false;
  }
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      stringComplete = true;
    } else {
      inputString += inChar;
    }
  }
}

void processCommand(String command) {
  command.toLowerCase();
  
  if (command == "help") {
    printHelp();
  } else if (command == "status") {
    printStatus();
  } else if (command.startsWith("set id ")) {
    setDeviceId(command.substring(7));
  } else if (command == "pair") {
    enterPairingMode();
  } else if (command == "unpair") {
    unpairDevice();
  } else if (command == "test") {
    testRemoteSignals();
  } else if (command == "save") {
    saveConfig();
  } else if (command == "load") {
    loadConfig();
  } else if (command == "reset") {
    resetConfig();
  } else {
    Serial.println("未知命令，输入 'help' 查看帮助");
  }
}

void printHelp() {
  Serial.println("=== 命令帮助 ===");
  Serial.println("help - 显示此帮助信息");
  Serial.println("status - 显示当前设备状态");
  Serial.println("set id <1-255> - 设置设备ID (例如: set id 1)");
  Serial.println("pair - 进入配对模式，等待遥控器信号");
  Serial.println("unpair - 取消当前配对");
  Serial.println("test - 测试模式，显示所有遥控器信号");
  Serial.println("save - 保存当前配置到EEPROM");
  Serial.println("load - 从EEPROM加载配置");
  Serial.println("reset - 重置所有配置为默认值");
  Serial.println("================");
}

void printStatus() {
  Serial.println("=== 设备状态 ===");
  Serial.print("设备ID: ");
  Serial.println(config.deviceId);
  Serial.print("配对状态: ");
  Serial.println(config.isPaired ? "已配对" : "未配对");
  if (config.isPaired) {
    Serial.print("配对遥控器ID: 0x");
    Serial.println(config.remoteId, HEX);
  }
  Serial.print("配置模式: ");
  Serial.println(config.configMode ? "开启" : "关闭");
  Serial.println("================");
}

void setDeviceId(String idStr) {
  int id = idStr.toInt();
  if (id >= 1 && id <= 255) {
    config.deviceId = id;
    Serial.print("设备ID已设置为: ");
    Serial.println(id);
    saveConfig();
  } else {
    Serial.println("错误：设备ID必须在1-255范围内");
  }
}

void enterPairingMode() {
  Serial.println("进入配对模式...");
  Serial.println("请按下遥控器的配对按键");
  Serial.println("等待10秒...");
  
  unsigned long startTime = millis();
  const unsigned long timeout = 10000; // 10秒超时
  
  while (millis() - startTime < timeout) {
    if (mySwitch.available()) {
      long value = mySwitch.getReceivedValue();
      Serial.print("收到配对信号: 0x");
      Serial.println(value, HEX);
      
      // 解析遥控器ID
      uint32_t remoteId = (value >> 16) & 0xFFFF;
      uint8_t command = value & 0xFF;
      
      if (command == 0x01) { // 配对请求
        config.remoteId = remoteId;
        config.isPaired = true;
        config.configMode = false;
        
        Serial.println("配对成功！");
        Serial.print("遥控器ID: 0x");
        Serial.println(remoteId, HEX);
        
        saveConfig();
        return;
      }
      
      mySwitch.resetAvailable();
    }
    delay(100);
  }
  
  Serial.println("配对超时，未收到有效信号");
}

void unpairDevice() {
  Serial.println("取消配对...");
  config.isPaired = false;
  config.remoteId = 0;
  config.configMode = true;
  
  // 清除EEPROM中的配对信息
  KeyValueEEPROM::put("is_paired", false);
  KeyValueEEPROM::put("remote_id", 0);
  
  Serial.println("配对已取消");
  printStatus();
}

void testRemoteSignals() {
  Serial.println("进入测试模式...");
  Serial.println("按下遥控器任意按键查看信号");
  Serial.println("按任意键退出测试模式");
  
  while (!Serial.available()) {
    if (mySwitch.available()) {
      long value = mySwitch.getReceivedValue();
      Serial.print("信号: 0x");
      Serial.print(value, HEX);
      Serial.print(" (");
      Serial.print(value);
      Serial.print(") - 遥控器ID: 0x");
      Serial.print((value >> 16) & 0xFFFF, HEX);
      Serial.print(" 指令: 0x");
      Serial.print(value & 0xFF, HEX);
      Serial.println(")");
      
      mySwitch.resetAvailable();
    }
    delay(100);
  }
  
  // 清空串口缓冲区
  while (Serial.available()) {
    Serial.read();
  }
  
  Serial.println("退出测试模式");
}

void saveConfig() {
  KeyValueEEPROM::put("device_id", config.deviceId);
  KeyValueEEPROM::put("is_paired", config.isPaired);
  KeyValueEEPROM::put("remote_id", config.remoteId);
  KeyValueEEPROM::put("config_mode", config.configMode);
  
  Serial.println("配置已保存到EEPROM");
}

void loadConfig() {
  config.deviceId = KeyValueEEPROM::get("device_id", 0x01);
  config.isPaired = KeyValueEEPROM::get("is_paired", false);
  config.remoteId = KeyValueEEPROM::get("remote_id", 0);
  config.configMode = KeyValueEEPROM::get("config_mode", true);
  
  Serial.println("配置已从EEPROM加载");
  printStatus();
}

void resetConfig() {
  Serial.println("重置所有配置...");
  
  // 重置内存中的配置
  config.deviceId = 0x01;
  config.remoteId = 0;
  config.isPaired = false;
  config.configMode = true;
  
  // 清除EEPROM
  KeyValueEEPROM::put("device_id", 0x01);
  KeyValueEEPROM::put("is_paired", false);
  KeyValueEEPROM::put("remote_id", 0);
  KeyValueEEPROM::put("config_mode", true);
  
  Serial.println("配置已重置为默认值");
  printStatus();
}

void handleRemoteSignals() {
  if (mySwitch.available()) {
    long value = mySwitch.getReceivedValue();
    
    // 在配置模式下显示所有信号
    if (config.configMode) {
      Serial.print("收到信号: 0x");
      Serial.println(value, HEX);
    } else {
      // 在正常模式下只处理配对遥控器的信号
      uint32_t remoteId = (value >> 16) & 0xFFFF;
      if (config.isPaired && remoteId == config.remoteId) {
        uint8_t command = value & 0xFF;
        Serial.print("配对遥控器指令: 0x");
        Serial.println(command, HEX);
        
        // 处理指令
        handleRemoteCommand(command);
      }
    }
    
    mySwitch.resetAvailable();
  }
}

void handleRemoteCommand(uint8_t command) {
  switch (command) {
    case 0x04: // LED开
      Serial.println("指令: 开灯");
      break;
    case 0x05: // LED关
      Serial.println("指令: 关灯");
      break;
    case 0x06: // 前进
      Serial.println("指令: 前进");
      break;
    case 0x07: // 后退
      Serial.println("指令: 后退");
      break;
    case 0x08: // 停止
      Serial.println("指令: 停止");
      break;
    case 0x09: // 左转
      Serial.println("指令: 左转");
      break;
    case 0x0A: // 右转
      Serial.println("指令: 右转");
      break;
    case 0x0B: // 加速
      Serial.println("指令: 加速");
      break;
    case 0x0C: // 减速
      Serial.println("指令: 减速");
      break;
    case 0x0D: // 状态
      Serial.println("指令: 状态查询");
      break;
    case 0x0E: // 紧急停止
      Serial.println("指令: 紧急停止");
      break;
    case 0x03: // 取消配对
      Serial.println("指令: 取消配对");
      unpairDevice();
      break;
    default:
      Serial.print("未知指令: 0x");
      Serial.println(command, HEX);
      break;
  }
} 