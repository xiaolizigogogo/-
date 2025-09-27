/*
 * nRF24L01 主从控制器系统 - 设备管理实现
 * 支持动态添加备用控制器和无线控制盒
 */

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// 设备类型定义
enum DeviceType {
  DEVICE_MASTER = 0x01,      // 主控制器
  DEVICE_BACKUP = 0x02,      // 备用控制器
  DEVICE_CONTROL_BOX = 0x03, // 无线控制盒
  DEVICE_SENSOR = 0x04,      // 传感器
  DEVICE_ACTUATOR = 0x05     // 执行器
};

// 设备状态
enum DeviceStatus {
  STATUS_OFFLINE = 0x00,     // 离线
  STATUS_ONLINE = 0x01,      // 在线
  STATUS_ACTIVE = 0x02,      // 活跃
  STATUS_STANDBY = 0x03,     // 待机
  STATUS_ERROR = 0x04        // 错误
};

// 包类型定义
enum PacketType {
  PACKET_HEARTBEAT = 0x01,   // 心跳包
  PACKET_COMMAND = 0x02,     // 命令包
  PACKET_RESPONSE = 0x03,    // 响应包
  PACKET_DISCOVERY = 0x04,   // 发现包
  PACKET_STATUS = 0x05,      // 状态包
  PACKET_EMERGENCY = 0x06    // 紧急包
};

// 设备信息结构
struct DeviceInfo {
  uint8_t deviceId;          // 设备ID
  uint8_t deviceType;        // 设备类型
  uint8_t status;            // 设备状态
  uint32_t lastSeen;         // 最后通信时间
  uint8_t signalStrength;    // 信号强度
  bool isActive;             // 是否活跃
};

// 通信数据包结构
struct CommunicationPacket {
  uint8_t header[4];         // 包头
  uint8_t packetType;        // 包类型
  uint8_t sourceId;          // 源设备ID
  uint8_t targetId;          // 目标设备ID (0xFF=广播)
  uint8_t command;           // 命令
  uint8_t data[20];          // 数据
  uint8_t checksum;          // 校验和
  uint8_t footer[2];         // 包尾
};

// 设备管理类
class DeviceManager {
private:
  DeviceInfo devices[10];     // 最大支持10个设备
  uint8_t deviceCount;
  uint8_t activeMasterId;
  
public:
  DeviceManager() : deviceCount(0), activeMasterId(0) {
    // 初始化设备列表
    for (int i = 0; i < 10; i++) {
      devices[i].deviceId = 0;
      devices[i].deviceType = 0;
      devices[i].status = STATUS_OFFLINE;
      devices[i].lastSeen = 0;
      devices[i].signalStrength = 0;
      devices[i].isActive = false;
    }
  }
  
  // 设备注册
  bool registerDevice(uint8_t deviceId, uint8_t deviceType) {
    if (deviceCount >= 10) {
      Serial.println("设备数量已达上限");
      return false;
    }
    
    // 检查设备是否已存在
    for (int i = 0; i < deviceCount; i++) {
      if (devices[i].deviceId == deviceId) {
        Serial.print("设备已存在: ");
        Serial.println(deviceId);
        return false;
      }
    }
    
    // 添加新设备
    devices[deviceCount].deviceId = deviceId;
    devices[deviceCount].deviceType = deviceType;
    devices[deviceCount].status = STATUS_ONLINE;
    devices[deviceCount].lastSeen = millis();
    devices[deviceCount].signalStrength = 100;
    devices[deviceCount].isActive = false;
    
    deviceCount++;
    
    Serial.print("设备注册成功: ID=");
    Serial.print(deviceId);
    Serial.print(", 类型=");
    Serial.println(deviceType);
    
    return true;
  }
  
  // 设备注销
  bool unregisterDevice(uint8_t deviceId) {
    for (int i = 0; i < deviceCount; i++) {
      if (devices[i].deviceId == deviceId) {
        // 移动后续设备
        for (int j = i; j < deviceCount - 1; j++) {
          devices[j] = devices[j + 1];
        }
        deviceCount--;
        
        Serial.print("设备注销成功: ");
        Serial.println(deviceId);
        return true;
      }
    }
    return false;
  }
  
  // 更新设备状态
  void updateDeviceStatus(uint8_t deviceId, uint8_t status) {
    for (int i = 0; i < deviceCount; i++) {
      if (devices[i].deviceId == deviceId) {
        devices[i].status = status;
        devices[i].lastSeen = millis();
        break;
      }
    }
  }
  
  // 获取活跃主控制器
  uint8_t getActiveMaster() {
    return activeMasterId;
  }
  
  // 设置活跃主控制器
  void setActiveMaster(uint8_t deviceId) {
    activeMasterId = deviceId;
    updateDeviceStatus(deviceId, STATUS_ACTIVE);
  }
  
  // 获取设备列表
  void getDeviceList(DeviceInfo* list, uint8_t* count) {
    *count = deviceCount;
    for (int i = 0; i < deviceCount; i++) {
      list[i] = devices[i];
    }
  }
  
  // 查找设备
  DeviceInfo* findDevice(uint8_t deviceId) {
    for (int i = 0; i < deviceCount; i++) {
      if (devices[i].deviceId == deviceId) {
        return &devices[i];
      }
    }
    return nullptr;
  }
  
  // 主控制器选举
  uint8_t electMaster() {
    uint8_t candidateId = 0;
    uint8_t highestPriority = 0;
    
    for (int i = 0; i < deviceCount; i++) {
      if (devices[i].deviceType == DEVICE_MASTER || 
          devices[i].deviceType == DEVICE_BACKUP) {
        
        uint8_t priority = calculatePriority(devices[i]);
        if (priority > highestPriority) {
          highestPriority = priority;
          candidateId = devices[i].deviceId;
        }
      }
    }
    
    return candidateId;
  }
  
  // 优先级计算
  uint8_t calculatePriority(DeviceInfo device) {
    uint8_t priority = 0;
    
    // 设备类型优先级
    if (device.deviceType == DEVICE_MASTER) priority += 100;
    else if (device.deviceType == DEVICE_BACKUP) priority += 50;
    
    // 信号强度
    priority += device.signalStrength;
    
    // 在线时间
    priority += (millis() - device.lastSeen) / 1000;
    
    return priority;
  }
  
  // 打印设备列表
  void printDeviceList() {
    Serial.println("=== 设备列表 ===");
    for (int i = 0; i < deviceCount; i++) {
      Serial.print("设备ID: ");
      Serial.print(devices[i].deviceId);
      Serial.print(", 类型: ");
      Serial.print(devices[i].deviceType);
      Serial.print(", 状态: ");
      Serial.print(devices[i].status);
      Serial.print(", 最后通信: ");
      Serial.print((millis() - devices[i].lastSeen) / 1000);
      Serial.println("秒前");
    }
    Serial.println("================");
  }
};

// 全局变量
RF24 radio(7, 8); // CE, CSN
DeviceManager deviceManager;
uint8_t MY_DEVICE_ID = 1;  // 当前设备ID
uint8_t MY_DEVICE_TYPE = DEVICE_MASTER;  // 当前设备类型

// 地址配置
const byte BROADCAST_ADDRESS[6] = "BCAST";  // 广播地址
const byte MASTER_ADDRESS[6] = "MASTR";     // 主控制器地址
const byte BACKUP_ADDRESS[6] = "BACKP";     // 备用控制器地址
const byte CONTROL_BOX_ADDRESS[6] = "CTRLB"; // 控制盒地址

// 时间变量
unsigned long lastHeartbeatTime = 0;
unsigned long lastDiscoveryTime = 0;
unsigned long lastStatusCheckTime = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("=== nRF24L01 主从控制器系统 ===");
  
  // 初始化nRF24L01
  radio.begin();
  radio.setChannel(76);
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setRetries(15, 15);
  
  // 设置接收管道
  setupReceivePipes();
  
  // 开始监听
  radio.startListening();
  
  // 注册自己
  deviceManager.registerDevice(MY_DEVICE_ID, MY_DEVICE_TYPE);
  if (MY_DEVICE_TYPE == DEVICE_MASTER) {
    deviceManager.setActiveMaster(MY_DEVICE_ID);
  }
  
  Serial.print("设备初始化完成 - ID: ");
  Serial.print(MY_DEVICE_ID);
  Serial.print(", 类型: ");
  Serial.println(MY_DEVICE_TYPE);
  
  // 开始设备发现
  startDeviceDiscovery();
}

void loop() {
  // 检查接收数据
  if (radio.available()) {
    CommunicationPacket packet;
    radio.read(&packet, sizeof(packet));
    processPacket(&packet);
  }
  
  // 发送心跳
  if (millis() - lastHeartbeatTime > 1000) {
    sendHeartbeat();
    lastHeartbeatTime = millis();
  }
  
  // 设备发现
  if (millis() - lastDiscoveryTime > 5000) {
    startDeviceDiscovery();
    lastDiscoveryTime = millis();
  }
  
  // 状态检查
  if (millis() - lastStatusCheckTime > 10000) {
    checkDeviceStatus();
    lastStatusCheckTime = millis();
  }
  
  // 主控制器健康检查
  if (MY_DEVICE_TYPE == DEVICE_MASTER) {
    checkMasterHealth();
  }
  
  // 打印设备列表（每30秒）
  static unsigned long lastPrintTime = 0;
  if (millis() - lastPrintTime > 30000) {
    deviceManager.printDeviceList();
    lastPrintTime = millis();
  }
}

// 设置接收管道
void setupReceivePipes() {
  // 管道0: 广播地址
  radio.openReadingPipe(0, BROADCAST_ADDRESS);
  
  // 管道1: 设备专用地址
  byte deviceAddress[6];
  generateDeviceAddress(MY_DEVICE_ID, deviceAddress);
  radio.openReadingPipe(1, deviceAddress);
  
  // 管道2-5: 其他设备地址
  for (int i = 2; i < 6; i++) {
    radio.openReadingPipe(i, BROADCAST_ADDRESS);
  }
}

// 动态地址生成
void generateDeviceAddress(uint8_t deviceId, byte* address) {
  address[0] = 'D';
  address[1] = 'E';
  address[2] = 'V';
  address[3] = (deviceId >> 4) & 0x0F;
  address[4] = deviceId & 0x0F;
  address[5] = '\0';
}

// 开始设备发现
void startDeviceDiscovery() {
  Serial.println("开始设备发现...");
  
  CommunicationPacket discovery;
  discovery.header[0] = 0xAA;
  discovery.header[1] = 0x55;
  discovery.header[2] = 0xAA;
  discovery.header[3] = 0x55;
  discovery.packetType = PACKET_DISCOVERY;
  discovery.sourceId = MY_DEVICE_ID;
  discovery.targetId = 0xFF;  // 广播
  discovery.command = 0x01;   // 发现请求
  discovery.data[0] = MY_DEVICE_TYPE;
  discovery.data[1] = 0xFF;   // 所有能力
  discovery.checksum = calculateChecksum(&discovery);
  discovery.footer[0] = 0x55;
  discovery.footer[1] = 0xAA;
  
  // 广播发现请求
  radio.stopListening();
  radio.write(&discovery, sizeof(discovery));
  radio.startListening();
  
  Serial.println("设备发现请求已发送");
}

// 发送心跳
void sendHeartbeat() {
  CommunicationPacket heartbeat;
  heartbeat.header[0] = 0xAA;
  heartbeat.header[1] = 0x55;
  heartbeat.header[2] = 0xAA;
  heartbeat.header[3] = 0x55;
  heartbeat.packetType = PACKET_HEARTBEAT;
  heartbeat.sourceId = MY_DEVICE_ID;
  heartbeat.targetId = 0xFF;  // 广播
  heartbeat.command = 0x00;   // 心跳
  heartbeat.data[0] = MY_DEVICE_TYPE;
  heartbeat.data[1] = deviceManager.getActiveMaster();
  heartbeat.checksum = calculateChecksum(&heartbeat);
  heartbeat.footer[0] = 0x55;
  heartbeat.footer[1] = 0xAA;
  
  radio.stopListening();
  radio.write(&heartbeat, sizeof(heartbeat));
  radio.startListening();
}

// 处理接收到的数据包
void processPacket(CommunicationPacket* packet) {
  // 验证数据包
  if (!validatePacket(packet)) {
    Serial.println("无效数据包");
    return;
  }
  
  // 更新设备状态
  deviceManager.updateDeviceStatus(packet->sourceId, STATUS_ONLINE);
  
  // 根据包类型处理
  switch (packet->packetType) {
    case PACKET_HEARTBEAT:
      handleHeartbeat(packet);
      break;
    case PACKET_DISCOVERY:
      handleDiscovery(packet);
      break;
    case PACKET_COMMAND:
      handleCommand(packet);
      break;
    case PACKET_RESPONSE:
      handleResponse(packet);
      break;
    case PACKET_STATUS:
      handleStatus(packet);
      break;
    case PACKET_EMERGENCY:
      handleEmergency(packet);
      break;
    default:
      Serial.println("未知包类型");
      break;
  }
}

// 处理心跳包
void handleHeartbeat(CommunicationPacket* packet) {
  Serial.print("收到心跳 - 设备ID: ");
  Serial.print(packet->sourceId);
  Serial.print(", 类型: ");
  Serial.println(packet->data[0]);
  
  // 如果是新设备，自动注册
  if (deviceManager.findDevice(packet->sourceId) == nullptr) {
    deviceManager.registerDevice(packet->sourceId, packet->data[0]);
  }
}

// 处理发现包
void handleDiscovery(CommunicationPacket* packet) {
  Serial.print("收到发现请求 - 设备ID: ");
  Serial.print(packet->sourceId);
  Serial.print(", 类型: ");
  Serial.println(packet->data[0]);
  
  // 注册新设备
  deviceManager.registerDevice(packet->sourceId, packet->data[0]);
  
  // 发送发现响应
  sendDiscoveryResponse(packet->sourceId);
}

// 发送发现响应
void sendDiscoveryResponse(uint8_t targetId) {
  CommunicationPacket response;
  response.header[0] = 0xAA;
  response.header[1] = 0x55;
  response.header[2] = 0xAA;
  response.header[3] = 0x55;
  response.packetType = PACKET_DISCOVERY;
  response.sourceId = MY_DEVICE_ID;
  response.targetId = targetId;
  response.command = 0x02;   // 发现响应
  response.data[0] = MY_DEVICE_TYPE;
  response.data[1] = deviceManager.getActiveMaster();
  response.checksum = calculateChecksum(&response);
  response.footer[0] = 0x55;
  response.footer[1] = 0xAA;
  
  radio.stopListening();
  radio.write(&response, sizeof(response));
  radio.startListening();
  
  Serial.print("发现响应已发送到设备: ");
  Serial.println(targetId);
}

// 处理命令包
void handleCommand(CommunicationPacket* packet) {
  Serial.print("收到命令 - 设备ID: ");
  Serial.print(packet->sourceId);
  Serial.print(", 命令: ");
  Serial.println(packet->command);
  
  // 根据命令类型处理
  switch (packet->command) {
    case 0x01:  // 请求设备列表
      sendDeviceList(packet->sourceId);
      break;
    case 0x02:  // 请求状态
      sendStatus(packet->sourceId);
      break;
    case 0x03:  // 设置主控制器
      setMaster(packet->sourceId);
      break;
    default:
      Serial.println("未知命令");
      break;
  }
}

// 发送设备列表
void sendDeviceList(uint8_t targetId) {
  DeviceInfo deviceList[10];
  uint8_t count;
  deviceManager.getDeviceList(deviceList, &count);
  
  CommunicationPacket response;
  response.header[0] = 0xAA;
  response.header[1] = 0x55;
  response.header[2] = 0xAA;
  response.header[3] = 0x55;
  response.packetType = PACKET_RESPONSE;
  response.sourceId = MY_DEVICE_ID;
  response.targetId = targetId;
  response.command = 0x01;   // 设备列表响应
  response.data[0] = count;  // 设备数量
  
  // 添加设备信息
  for (int i = 0; i < count && i < 5; i++) {  // 最多5个设备
    response.data[1 + i * 3] = deviceList[i].deviceId;
    response.data[2 + i * 3] = deviceList[i].deviceType;
    response.data[3 + i * 3] = deviceList[i].status;
  }
  
  response.checksum = calculateChecksum(&response);
  response.footer[0] = 0x55;
  response.footer[1] = 0xAA;
  
  radio.stopListening();
  radio.write(&response, sizeof(response));
  radio.startListening();
  
  Serial.print("设备列表已发送到设备: ");
  Serial.println(targetId);
}

// 发送状态
void sendStatus(uint8_t targetId) {
  CommunicationPacket response;
  response.header[0] = 0xAA;
  response.header[1] = 0x55;
  response.header[2] = 0xAA;
  response.header[3] = 0x55;
  response.packetType = PACKET_STATUS;
  response.sourceId = MY_DEVICE_ID;
  response.targetId = targetId;
  response.command = 0x00;   // 状态响应
  response.data[0] = MY_DEVICE_TYPE;
  response.data[1] = deviceManager.getActiveMaster();
  response.data[2] = deviceManager.findDevice(MY_DEVICE_ID)->status;
  response.checksum = calculateChecksum(&response);
  response.footer[0] = 0x55;
  response.footer[1] = 0xAA;
  
  radio.stopListening();
  radio.write(&response, sizeof(response));
  radio.startListening();
  
  Serial.print("状态已发送到设备: ");
  Serial.println(targetId);
}

// 设置主控制器
void setMaster(uint8_t deviceId) {
  if (MY_DEVICE_TYPE == DEVICE_MASTER || MY_DEVICE_TYPE == DEVICE_BACKUP) {
    deviceManager.setActiveMaster(deviceId);
    Serial.print("设置主控制器: ");
    Serial.println(deviceId);
  }
}

// 处理响应包
void handleResponse(CommunicationPacket* packet) {
  Serial.print("收到响应 - 设备ID: ");
  Serial.print(packet->sourceId);
  Serial.print(", 命令: ");
  Serial.println(packet->command);
}

// 处理状态包
void handleStatus(CommunicationPacket* packet) {
  Serial.print("收到状态 - 设备ID: ");
  Serial.print(packet->sourceId);
  Serial.print(", 状态: ");
  Serial.println(packet->data[2]);
}

// 处理紧急包
void handleEmergency(CommunicationPacket* packet) {
  Serial.print("收到紧急包 - 设备ID: ");
  Serial.print(packet->sourceId);
  Serial.print(", 命令: ");
  Serial.println(packet->command);
  
  // 紧急处理逻辑
  if (packet->command == 0x01) {  // 主控制器故障
    Serial.println("主控制器故障，开始切换...");
    switchToBackup();
  }
}

// 检查设备状态
void checkDeviceStatus() {
  DeviceInfo deviceList[10];
  uint8_t count;
  deviceManager.getDeviceList(deviceList, &count);
  
  for (int i = 0; i < count; i++) {
    if (millis() - deviceList[i].lastSeen > 15000) {  // 15秒超时
      Serial.print("设备离线: ");
      Serial.println(deviceList[i].deviceId);
      deviceManager.updateDeviceStatus(deviceList[i].deviceId, STATUS_OFFLINE);
    }
  }
}

// 检查主控制器健康状态
void checkMasterHealth() {
  uint8_t activeMaster = deviceManager.getActiveMaster();
  if (activeMaster != 0) {
    DeviceInfo* master = deviceManager.findDevice(activeMaster);
    if (master && (millis() - master->lastSeen > 10000)) {
      Serial.println("主控制器离线，开始切换...");
      switchToBackup();
    }
  }
}

// 切换到备用控制器
void switchToBackup() {
  uint8_t newMaster = deviceManager.electMaster();
  
  if (newMaster != 0 && newMaster != deviceManager.getActiveMaster()) {
    Serial.print("切换到备用控制器: ");
    Serial.println(newMaster);
    
    // 发送切换通知
    sendMasterSwitchNotification(newMaster);
    
    // 更新活跃主控制器
    deviceManager.setActiveMaster(newMaster);
  }
}

// 发送主控制器切换通知
void sendMasterSwitchNotification(uint8_t newMasterId) {
  CommunicationPacket notification;
  notification.header[0] = 0xAA;
  notification.header[1] = 0x55;
  notification.header[2] = 0xAA;
  notification.header[3] = 0x55;
  notification.packetType = PACKET_EMERGENCY;
  notification.sourceId = MY_DEVICE_ID;
  notification.targetId = 0xFF;  // 广播
  notification.command = 0x02;   // 主控制器切换
  notification.data[0] = newMasterId;
  notification.checksum = calculateChecksum(&notification);
  notification.footer[0] = 0x55;
  notification.footer[1] = 0xAA;
  
  radio.stopListening();
  radio.write(&notification, sizeof(notification));
  radio.startListening();
  
  Serial.print("主控制器切换通知已发送: ");
  Serial.println(newMasterId);
}

// 验证数据包
bool validatePacket(CommunicationPacket* packet) {
  // 检查包头
  if (packet->header[0] != 0xAA || packet->header[1] != 0x55 ||
      packet->header[2] != 0xAA || packet->header[3] != 0x55) {
    return false;
  }
  
  // 检查包尾
  if (packet->footer[0] != 0x55 || packet->footer[1] != 0xAA) {
    return false;
  }
  
  // 检查校验和
  if (packet->checksum != calculateChecksum(packet)) {
    return false;
  }
  
  return true;
}

// 计算校验和
uint8_t calculateChecksum(CommunicationPacket* packet) {
  uint8_t checksum = 0;
  
  // 计算除校验和字段外的所有字节
  for (int i = 0; i < sizeof(CommunicationPacket); i++) {
    if (i != offsetof(CommunicationPacket, checksum)) {
      checksum ^= ((uint8_t*)packet)[i];
    }
  }
  
  return checksum;
}

