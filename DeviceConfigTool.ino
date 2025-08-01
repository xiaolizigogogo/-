/*
 * 设备配置工具
 * 用于设置和测试多设备环境下的设备ID
 * 通过串口命令进行设备配置
 */

#include "DeviceConfig.h"
#include <KeyValueEEPROM.h>

// 设备配置管理器
DeviceConfigManager deviceConfig;

// 当前设备ID
uint8_t currentDeviceId = DEVICE_ID_1;

void setup() {
  Serial.begin(9600);
  
  // 从EEPROM读取设备ID
  currentDeviceId = KeyValueEEPROM::get("device_id", DEVICE_ID_1);
  deviceConfig.setDeviceId(currentDeviceId);
  
  Serial.println("=== 设备配置工具 ===");
  Serial.println("可用命令：");
  Serial.println("1. 'id' - 查看当前设备ID");
  Serial.println("2. 'set id X' - 设置设备ID为X (1-255)");
  Serial.println("3. 'test' - 测试设备ID配置");
  Serial.println("4. 'list' - 列出所有设备ID");
  Serial.println("5. 'help' - 显示帮助信息");
  Serial.println("==================");
  
  printCurrentDeviceInfo();
}

void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    processCommand(command);
  }
  
  delay(100);
}

void processCommand(String command) {
  command.toLowerCase();
  
  if (command == "id") {
    printCurrentDeviceInfo();
  }
  else if (command.startsWith("set id ")) {
    int newId = command.substring(7).toInt();
    setDeviceId(newId);
  }
  else if (command == "test") {
    testDeviceConfiguration();
  }
  else if (command == "list") {
    listAllDevices();
  }
  else if (command == "help") {
    printHelp();
  }
  else if (command == "reset") {
    resetDeviceConfig();
  }
  else {
    Serial.println("未知命令，输入 'help' 查看可用命令");
  }
}

void printCurrentDeviceInfo() {
  Serial.println("=== 当前设备信息 ===");
  Serial.print("设备ID: ");
  Serial.println(currentDeviceId);
  Serial.print("设备名称: ");
  Serial.println(deviceConfig.getDeviceName());
  Serial.print("工作频率: 433MHz");
  Serial.println();
  
  // 显示遥控器按键映射
  Serial.println("遥控器按键映射：");
  Serial.print("按键1: 设备");
  Serial.print(currentDeviceId);
  Serial.println("开灯");
  Serial.print("按键2: 设备");
  Serial.print(currentDeviceId);
  Serial.println("关灯");
  Serial.print("按键3: 设备");
  Serial.print(currentDeviceId);
  Serial.println("前进");
  Serial.print("按键4: 设备");
  Serial.print(currentDeviceId);
  Serial.println("后退");
  Serial.println("==================");
}

void setDeviceId(int newId) {
  if (newId >= 1 && newId <= 255) {
    currentDeviceId = newId;
    deviceConfig.setDeviceId(newId);
    KeyValueEEPROM::put("device_id", newId);
    
    Serial.print("设备ID已设置为: ");
    Serial.println(newId);
    Serial.print("设备名称: ");
    Serial.println(deviceConfig.getDeviceName());
    Serial.println("请重启设备以应用新配置");
  } else {
    Serial.println("错误：设备ID必须在1-255范围内");
  }
}

void testDeviceConfiguration() {
  Serial.println("=== 设备配置测试 ===");
  
  // 测试设备ID读取
  uint8_t savedId = KeyValueEEPROM::get("device_id", 0);
  Serial.print("EEPROM中保存的设备ID: ");
  Serial.println(savedId);
  
  // 测试设备ID匹配
  if (savedId == currentDeviceId) {
    Serial.println("✓ 设备ID配置正确");
  } else {
    Serial.println("✗ 设备ID配置错误");
  }
  
  // 测试遥控器指令解析
  uint32_t testCommand1 = ((currentDeviceId << 16) | CMD_LED_ON);
  uint32_t testCommand2 = ((currentDeviceId << 16) | CMD_MOTOR_FWD);
  uint32_t testCommand3 = ((DEVICE_ID_ALL << 16) | CMD_EMERGENCY_STOP);
  
  Serial.println("遥控器指令测试：");
  Serial.print("设备");
  Serial.print(currentDeviceId);
  Serial.print("开灯指令: 0x");
  Serial.println(testCommand1, HEX);
  
  Serial.print("设备");
  Serial.print(currentDeviceId);
  Serial.print("前进指令: 0x");
  Serial.println(testCommand2, HEX);
  
  Serial.print("紧急停止指令: 0x");
  Serial.println(testCommand3, HEX);
  
  // 测试指令解析
  if (deviceConfig.isCommandForThisDevice(testCommand1)) {
    Serial.println("✓ 设备指令解析正确");
  } else {
    Serial.println("✗ 设备指令解析错误");
  }
  
  if (deviceConfig.isCommandForThisDevice(testCommand3)) {
    Serial.println("✓ 通用指令解析正确");
  } else {
    Serial.println("✗ 通用指令解析错误");
  }
  
  Serial.println("==================");
}

void listAllDevices() {
  Serial.println("=== 设备列表 ===");
  Serial.println("设备ID | 设备名称 | 遥控器按键范围");
  Serial.println("-------|----------|----------------");
  
  for (int i = 1; i <= 10; i++) {
    Serial.print("  ");
    Serial.print(i);
    Serial.print("   | Device");
    Serial.print(i);
    Serial.print("  | 按键");
    Serial.print((i-1)*4 + 1);
    Serial.print("-");
    Serial.print(i*4);
    Serial.println();
  }
  
  Serial.println("  FF   | 所有设备 | 紧急停止/重置");
  Serial.println("==================");
}

void printHelp() {
  Serial.println("=== 帮助信息 ===");
  Serial.println("多设备遥控器隔离系统配置工具");
  Serial.println();
  Serial.println("命令说明：");
  Serial.println("1. 'id' - 查看当前设备ID和配置信息");
  Serial.println("2. 'set id X' - 设置设备ID为X (1-255)");
  Serial.println("3. 'test' - 测试设备配置是否正确");
  Serial.println("4. 'list' - 列出所有设备ID和按键映射");
  Serial.println("5. 'reset' - 重置设备配置为默认值");
  Serial.println("6. 'help' - 显示此帮助信息");
  Serial.println();
  Serial.println("使用说明：");
  Serial.println("- 每个设备必须设置唯一的设备ID");
  Serial.println("- 设备ID范围：1-255");
  Serial.println("- 设置完成后需要重启设备");
  Serial.println("- 遥控器按键会根据设备ID自动映射");
  Serial.println("==================");
}

void resetDeviceConfig() {
  currentDeviceId = DEVICE_ID_1;
  deviceConfig.setDeviceId(DEVICE_ID_1);
  KeyValueEEPROM::put("device_id", DEVICE_ID_1);
  
  Serial.println("设备配置已重置为默认值");
  Serial.print("设备ID: ");
  Serial.println(DEVICE_ID_1);
  Serial.println("请重启设备以应用新配置");
}

// 生成遥控器按键映射表
void generateRemoteMapping() {
  Serial.println("=== 遥控器按键映射表 ===");
  Serial.println("设备ID | 按键1 | 按键2 | 按键3 | 按键4");
  Serial.println("-------|-------|-------|-------|-------");
  
  for (int i = 1; i <= 5; i++) {
    uint32_t key1 = ((i << 16) | CMD_LED_ON);
    uint32_t key2 = ((i << 16) | CMD_LED_OFF);
    uint32_t key3 = ((i << 16) | CMD_MOTOR_FWD);
    uint32_t key4 = ((i << 16) | CMD_MOTOR_BWD);
    
    Serial.print("  ");
    Serial.print(i);
    Serial.print("   | 0x");
    Serial.print(key1, HEX);
    Serial.print(" | 0x");
    Serial.print(key2, HEX);
    Serial.print(" | 0x");
    Serial.print(key3, HEX);
    Serial.print(" | 0x");
    Serial.println(key4, HEX);
  }
  
  Serial.println("==================");
} 