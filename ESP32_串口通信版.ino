/*
 * ESP32 串口通信版
 * 功能：与Mega2560进行串口通信，不包含WiFi功能
 * 避免WiFi导致的重启问题
 */

// 串口配置
#define MEGA_TX_PIN 16     // ESP32 TX
#define MEGA_RX_PIN 17     // ESP32 RX
#define STATUS_LED_PIN 2   // 状态指示

// 在ESP32上使用Serial2与Mega2560通信
#define megaSerial Serial2

// 系统状态
bool megaConnected = false;
unsigned long lastHeartbeat = 0;
unsigned long lastDataSync = 0;

// 数据缓存
String lastMasterData = "";
String lastSlaveData = "";
unsigned long lastMasterUpdate = 0;
unsigned long lastSlaveUpdate = 0;

void setup() {
  // 先初始化调试串口
  Serial.begin(115200);
  delay(100);
  
  Serial.println("==========================================");
  Serial.println("ESP32 串口通信版启动");
  Serial.println("==========================================");
  
  // 初始化引脚
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  
  // 初始化与Mega2560的通信
  megaSerial.begin(115200, SERIAL_8N1, MEGA_RX_PIN, MEGA_TX_PIN);
  
  // 等待启动完成
  delay(2000);
  
  Serial.println("ESP32串口通信版初始化完成！");
  
  // 发送初始化响应
  megaSerial.println("ESP32_READY");
  Serial.println("发送ESP32_READY到Mega2560");
  
  // 等待Mega2560响应
  unsigned long timeout = millis() + 10000;
  while (millis() < timeout) {
    if (megaSerial.available()) {
      String response = megaSerial.readStringUntil('\n');
      response.trim();
      
      Serial.println("收到Mega2560消息: " + response);
      
      if (response == "INIT_MASTER" || response == "SLAVE_READY") {
        megaConnected = true;
        digitalWrite(STATUS_LED_PIN, HIGH);
        Serial.println("Mega2560连接成功！");
        break;
      }
    }
    delay(100);
  }
  
  if (!megaConnected) {
    Serial.println("Mega2560连接超时！");
  }
}

void loop() {
  // 处理与Mega2560的通信
  handleMegaCommunication();
  
  // 发送心跳
  sendHeartbeat();
  
  // 同步数据
  syncData();
  
  delay(10);
}

// 处理Mega2560通信
void handleMegaCommunication() {
  if (megaSerial.available()) {
    String data = megaSerial.readStringUntil('\n');
    data.trim();
    
    Serial.println("收到Mega2560数据: " + data);
    
    // 处理不同类型的数据
    if (data.startsWith("MASTER_DATA:")) {
      handleMasterData(data);
    } else if (data.startsWith("SLAVE_STATUS:") || data.startsWith("SLAVE_DATA:")) {
      handleSlaveData(data);
    } else if (data == "INIT_MASTER" || data == "SLAVE_READY") {
      megaConnected = true;
      digitalWrite(STATUS_LED_PIN, HIGH);
      Serial.println("Mega2560重新连接成功！");
    }
  }
}

// 处理主设备数据
void handleMasterData(String data) {
  lastMasterData = data;
  lastMasterUpdate = millis();
  
  Serial.println("处理主设备数据: " + data);
  
  // 这里可以添加数据处理逻辑
  // 例如：解析数据、存储到EEPROM等
}

// 处理从设备数据
void handleSlaveData(String data) {
  lastSlaveData = data;
  lastSlaveUpdate = millis();
  
  Serial.println("处理从设备数据: " + data);
  
  // 这里可以添加数据处理逻辑
}

// 发送心跳
void sendHeartbeat() {
  if (millis() - lastHeartbeat > 5000) { // 每5秒发送一次
    if (megaConnected) {
      megaSerial.println("ESP32_HEARTBEAT");
      Serial.println("发送心跳到Mega2560");
    }
    lastHeartbeat = millis();
  }
}

// 同步数据
void syncData() {
  if (millis() - lastDataSync > 1000) { // 每1秒同步一次
    // 检查主设备数据是否需要同步
    if (lastMasterData.length() > 0 && millis() - lastMasterUpdate < 5000) {
      Serial.println("主设备数据同步: " + lastMasterData);
    }
    
    // 检查从设备数据是否需要同步
    if (lastSlaveData.length() > 0 && millis() - lastSlaveUpdate < 5000) {
      Serial.println("从设备数据同步: " + lastSlaveData);
    }
    
    lastDataSync = millis();
  }
}

// 获取连接状态
bool isMegaConnected() {
  return megaConnected;
}

// 获取系统状态
String getSystemStatus() {
  String status = "Mega: " + String(megaConnected ? "已连接" : "未连接") + 
                  ", 主设备数据: " + String(lastMasterData.length() > 0 ? "有" : "无") +
                  ", 从设备数据: " + String(lastSlaveData.length() > 0 ? "有" : "无");
  return status;
}
