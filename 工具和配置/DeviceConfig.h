#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <Arduino.h>

// 设备配置结构
struct DeviceConfig {
  uint8_t deviceId;           // 设备唯一ID (1-255)
  char deviceName[16];        // 设备名称
  uint32_t frequency;         // 工作频率 (可选)
  bool enableRemote;          // 是否启用遥控器
  uint8_t maxSpeed;           // 最大速度
  uint8_t minSpeed;           // 最小速度
};

// 设备ID定义
#define DEVICE_ID_1 0x01
#define DEVICE_ID_2 0x02
#define DEVICE_ID_3 0x03
#define DEVICE_ID_4 0x04
#define DEVICE_ID_5 0x05

// 通用设备ID (所有设备响应)
#define DEVICE_ID_ALL 0xFF

// 遥控器指令结构
struct RemoteCommand {
  uint8_t deviceId;           // 目标设备ID
  uint8_t commandType;        // 指令类型
  uint8_t parameter;          // 参数
  uint32_t timestamp;         // 时间戳
};

// 指令类型定义
enum CommandType {
  CMD_NONE = 0x00,
  CMD_LED_ON = 0x01,
  CMD_LED_OFF = 0x02,
  CMD_MOTOR_FWD = 0x03,
  CMD_MOTOR_BWD = 0x04,
  CMD_MOTOR_STOP = 0x05,
  CMD_SERVO_LEFT = 0x06,
  CMD_SERVO_RIGHT = 0x07,
  CMD_SPEED_UP = 0x08,
  CMD_SPEED_DOWN = 0x09,
  CMD_STATUS = 0x0A,
  CMD_RECORD = 0x0B,
  CMD_RESET = 0x0C,
  CMD_EMERGENCY_STOP = 0x0D
};

// 遥控器按键映射 (设备ID + 指令类型)
#define REMOTE_KEY_1_LED_ON     ((DEVICE_ID_1 << 16) | CMD_LED_ON)
#define REMOTE_KEY_1_LED_OFF    ((DEVICE_ID_1 << 16) | CMD_LED_OFF)
#define REMOTE_KEY_1_MOTOR_FWD  ((DEVICE_ID_1 << 16) | CMD_MOTOR_FWD)
#define REMOTE_KEY_1_MOTOR_BWD  ((DEVICE_ID_1 << 16) | CMD_MOTOR_BWD)
#define REMOTE_KEY_1_MOTOR_STOP ((DEVICE_ID_1 << 16) | CMD_MOTOR_STOP)

#define REMOTE_KEY_2_LED_ON     ((DEVICE_ID_2 << 16) | CMD_LED_ON)
#define REMOTE_KEY_2_LED_OFF    ((DEVICE_ID_2 << 16) | CMD_LED_OFF)
#define REMOTE_KEY_2_MOTOR_FWD  ((DEVICE_ID_2 << 16) | CMD_MOTOR_FWD)
#define REMOTE_KEY_2_MOTOR_BWD  ((DEVICE_ID_2 << 16) | CMD_MOTOR_BWD)
#define REMOTE_KEY_2_MOTOR_STOP ((DEVICE_ID_2 << 16) | CMD_MOTOR_STOP)

// 通用指令 (所有设备响应)
#define REMOTE_KEY_ALL_STOP     ((DEVICE_ID_ALL << 16) | CMD_EMERGENCY_STOP)
#define REMOTE_KEY_ALL_RESET    ((DEVICE_ID_ALL << 16) | CMD_RESET)

// 设备配置管理类
class DeviceConfigManager {
private:
  DeviceConfig config;
  
public:
  DeviceConfigManager() {
    // 默认配置
    config.deviceId = DEVICE_ID_1;
    strcpy(config.deviceName, "Device1");
    config.frequency = 433000000;
    config.enableRemote = true;
    config.maxSpeed = 255;
    config.minSpeed = 50;
  }
  
  // 设置设备ID
  void setDeviceId(uint8_t id) {
    config.deviceId = id;
    updateDeviceName();
  }
  
  // 获取设备ID
  uint8_t getDeviceId() {
    return config.deviceId;
  }
  
  // 检查指令是否针对本设备
  bool isCommandForThisDevice(uint32_t command) {
    uint8_t targetDeviceId = (command >> 16) & 0xFF;
    return (targetDeviceId == config.deviceId || targetDeviceId == DEVICE_ID_ALL);
  }
  
  // 从指令中提取指令类型
  uint8_t getCommandType(uint32_t command) {
    return command & 0xFF;
  }
  
  // 更新设备名称
  void updateDeviceName() {
    sprintf(config.deviceName, "Device%d", config.deviceId);
  }
  
  // 获取设备名称
  const char* getDeviceName() {
    return config.deviceName;
  }
  
  // 保存配置到EEPROM
  void saveConfig() {
    // 这里可以添加EEPROM保存逻辑
  }
  
  // 从EEPROM加载配置
  void loadConfig() {
    // 这里可以添加EEPROM加载逻辑
  }
};

#endif // DEVICE_CONFIG_H 