#include <EEPROM.h>

// 心跳配置
#define HEARTBEAT_ENABLED true        // 是否启用心跳
#define HEARTBEAT_INTERVAL 5000       // 心跳间隔（毫秒）
#define HEARTBEAT_TIMEOUT 15000       // 心跳超时时间（毫秒）
#define MAX_MISSED_HEARTBEATS 3       // 最大丢失心跳次数

// 业务模式配置
#define BUSINESS_MODE_SHORT_TERM 0    // 短时间操作模式
#define BUSINESS_MODE_LONG_TERM 1     // 长时间待机模式
#define BUSINESS_MODE_SAFETY_CRITICAL 2  // 安全关键模式

// 当前业务模式（可根据实际需求修改）
int currentBusinessMode = BUSINESS_MODE_LONG_TERM;

// 心跳状态变量
unsigned long lastHeartbeatSent = 0;
unsigned long lastHeartbeatReceived = 0;
int missedHeartbeats = 0;
bool connectionLost = false;

// 其他变量
bool pairingMode = false;
bool isPaired = false;
unsigned long lastPairingSendTime = 0;
int pairingAttempts = 0;
const int MAX_PAIRING_ATTEMPTS = 3;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);
  
  // 从EEPROM读取配对状态
  isPaired = EEPROM.read(0) == 1;
  
  Serial.println("=== 智能心跳控制程序 ===");
  Serial.print("当前业务模式: ");
  switch(currentBusinessMode) {
    case BUSINESS_MODE_SHORT_TERM:
      Serial.println("短时间操作模式");
      break;
    case BUSINESS_MODE_LONG_TERM:
      Serial.println("长时间待机模式");
      break;
    case BUSINESS_MODE_SAFETY_CRITICAL:
      Serial.println("安全关键模式");
      break;
  }
  
  Serial.print("心跳机制: ");
  Serial.println(HEARTBEAT_ENABLED ? "启用" : "禁用");
  
  if (isPaired) {
    Serial.println(">>> 设备已配对，进入正常工作模式");
  } else {
    Serial.println(">>> 设备未配对，长按bt1键3秒进入配对模式");
  }
}

void loop() {
  unsigned long currentTime = millis();
  
  // 检查按钮状态
  if (digitalRead(2) == LOW) {
    if (currentTime - lastPairingSendTime >= 3000 && !pairingMode) {
      enterPairingMode();
    }
  } else {
    if (pairingMode) {
      exitPairingMode();
    }
  }
  
  // 根据业务模式处理心跳
  handleHeartbeatByBusinessMode(currentTime);
  
  // 处理接收数据
  handleReceivedData();
  
  // 配对模式处理
  if (pairingMode) {
    handlePairingMode(currentTime);
  }
  
  // 正常工作模式处理
  if (isPaired && !pairingMode) {
    handleNormalOperation(currentTime);
  }
  
  delay(100);
}

void handleHeartbeatByBusinessMode(unsigned long currentTime) {
  if (!HEARTBEAT_ENABLED || !isPaired) return;
  
  switch(currentBusinessMode) {
    case BUSINESS_MODE_SHORT_TERM:
      // 短时间操作模式：不发送心跳，节省电量
      break;
      
    case BUSINESS_MODE_LONG_TERM:
      // 长时间待机模式：定期发送心跳
      if (currentTime - lastHeartbeatSent >= HEARTBEAT_INTERVAL) {
        sendHeartbeat();
        lastHeartbeatSent = currentTime;
      }
      break;
      
    case BUSINESS_MODE_SAFETY_CRITICAL:
      // 安全关键模式：频繁发送心跳，快速检测连接状态
      if (currentTime - lastHeartbeatSent >= 2000) {  // 2秒间隔
        sendHeartbeat();
        lastHeartbeatSent = currentTime;
      }
      break;
  }
  
  // 检查心跳超时
  if (currentTime - lastHeartbeatReceived > HEARTBEAT_TIMEOUT) {
    missedHeartbeats++;
    if (missedHeartbeats >= MAX_MISSED_HEARTBEATS) {
      handleConnectionLost();
    }
  }
}

void sendHeartbeat() {
  byte heartbeat[] = {0xFD, 0x04, 0xD5, 0xAA, 0x55, 0x98, 0xDF};
  Serial2.write(heartbeat, sizeof(heartbeat));
  Serial2.flush();
  Serial.println(">>> 发送心跳信号");
}

void handleConnectionLost() {
  if (!connectionLost) {
    connectionLost = true;
    Serial.println(">>> 警告：连接丢失，设备可能离线");
    
    // 根据业务模式处理连接丢失
    switch(currentBusinessMode) {
      case BUSINESS_MODE_SHORT_TERM:
        // 短时间操作：静默处理
        break;
      case BUSINESS_MODE_LONG_TERM:
        // 长时间待机：尝试重新连接
        Serial.println(">>> 尝试重新连接...");
        break;
      case BUSINESS_MODE_SAFETY_CRITICAL:
        // 安全关键：立即进入安全模式
        Serial.println(">>> 进入安全模式，停止所有操作");
        break;
    }
  }
}

void handleReceivedData() {
  if (Serial2.available()) {
    byte data = Serial2.read();
    
    if (data == 0xF0) {
      Serial.println(">>> 收到错误码: 0xF0");
      return;
    }
    
    // 处理心跳响应
    if (data == 0x98) {
      lastHeartbeatReceived = millis();
      missedHeartbeats = 0;
      connectionLost = false;
      Serial.println(">>> 收到心跳响应");
    }
    
    // 处理其他数据...
  }
}

void enterPairingMode() {
  pairingMode = true;
  pairingAttempts = 0;
  Serial.println(">>> 进入配对模式");
}

void exitPairingMode() {
  pairingMode = false;
  Serial.println(">>> 退出配对模式");
}

void handlePairingMode(unsigned long currentTime) {
  if (currentTime - lastPairingSendTime >= 10000) {
    if (pairingAttempts < MAX_PAIRING_ATTEMPTS) {
      sendPairingRequest();
      pairingAttempts++;
      lastPairingSendTime = currentTime;
    } else {
      Serial.println(">>> 配对尝试次数超限，退出配对模式");
      pairingMode = false;
    }
  }
}

void sendPairingRequest() {
  byte pairingRequest[] = {0xFD, 0x03, 0xD5, 0xAA, 0x55, 0x96, 0xDF};
  Serial2.write(pairingRequest, sizeof(pairingRequest));
  Serial2.flush();
  Serial.println(">>> 发送配对请求");
}

void handleNormalOperation(unsigned long currentTime) {
  // 正常工作模式的业务逻辑
  // 这里可以添加具体的控制命令处理
}

// 业务模式切换函数
void setBusinessMode(int mode) {
  currentBusinessMode = mode;
  Serial.print(">>> 业务模式切换为: ");
  switch(mode) {
    case BUSINESS_MODE_SHORT_TERM:
      Serial.println("短时间操作模式");
      break;
    case BUSINESS_MODE_LONG_TERM:
      Serial.println("长时间待机模式");
      break;
    case BUSINESS_MODE_SAFETY_CRITICAL:
      Serial.println("安全关键模式");
      break;
  }
}

// 心跳配置函数
void configureHeartbeat(bool enabled, int interval, int timeout) {
  // 这里可以动态配置心跳参数
  Serial.print(">>> 心跳配置: ");
  Serial.print(enabled ? "启用" : "禁用");
  Serial.print(", 间隔: ");
  Serial.print(interval);
  Serial.print("ms, 超时: ");
  Serial.print(timeout);
  Serial.println("ms");
}
