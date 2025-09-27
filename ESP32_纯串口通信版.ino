/*
 * ESP32 纯串口通信版
 * 功能：只进行串口通信，不包含WiFi功能
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
int dataCount = 0;

// 数据统计
unsigned long totalDataReceived = 0;
unsigned long totalDataSent = 0;
unsigned long lastStatsTime = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("==========================================");
  Serial.println("ESP32 纯串口通信版启动");
  Serial.println("==========================================");
  
  // 初始化引脚
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  
  // 初始化与Mega2560的通信
  megaSerial.begin(115200, SERIAL_8N1, MEGA_RX_PIN, MEGA_TX_PIN);
  
  Serial.println("ESP32初始化完成！");
  
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
  
  Serial.println("==========================================");
  Serial.println("系统就绪，开始数据通信");
  Serial.println("==========================================");
}

void loop() {
  // 处理与Mega2560的通信
  handleMegaCommunication();
  
  // 发送心跳
  sendHeartbeat();
  
  // 显示统计信息
  showStats();
  
  delay(10);
}

// 处理Mega2560通信
void handleMegaCommunication() {
  if (megaSerial.available()) {
    String data = megaSerial.readStringUntil('\n');
    data.trim();
    
    if (data.length() > 0) {
      dataCount++;
      totalDataReceived++;
      
      Serial.println("收到数据 #" + String(dataCount) + ": " + data);
      Serial.println("数据长度: " + String(data.length()));
      
      // 处理不同类型的数据
      if (data.startsWith("MASTER_DATA:")) {
        handleMasterData(data);
      } else if (data.startsWith("SLAVE_STATUS:") || data.startsWith("SLAVE_DATA:")) {
        handleSlaveData(data);
      } else if (data == "INIT_MASTER" || data == "SLAVE_READY") {
        megaConnected = true;
        digitalWrite(STATUS_LED_PIN, HIGH);
        Serial.println("Mega2560重新连接成功！");
      } else {
        Serial.println("其他数据: " + data);
      }
      
      // 状态指示
      digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
    }
  }
}

// 处理主设备数据
void handleMasterData(String data) {
  Serial.println("处理主设备数据: " + data);
  
  // 解析数据
  if (data.length() > 12) { // "MASTER_DATA:" 长度为12
    String dataPart = data.substring(12);
    Serial.println("数据部分: " + dataPart);
    
    // 分割数据
    int commaCount = 0;
    for (int i = 0; i < dataPart.length(); i++) {
      if (dataPart.charAt(i) == ',') {
        commaCount++;
      }
    }
    Serial.println("数据字段数量: " + String(commaCount + 1));
    
    // 这里可以添加数据解析和存储逻辑
    // 例如：解析舵机角度、编码器值等
  }
}

// 处理从设备数据
void handleSlaveData(String data) {
  Serial.println("处理从设备数据: " + data);
  
  // 这里可以添加从设备数据处理逻辑
}

// 发送心跳
void sendHeartbeat() {
  if (millis() - lastHeartbeat > 5000) { // 每5秒发送一次
    if (megaConnected) {
      megaSerial.println("ESP32_HEARTBEAT");
      Serial.println("发送心跳到Mega2560");
      totalDataSent++;
    }
    lastHeartbeat = millis();
  }
}

// 显示统计信息
void showStats() {
  if (millis() - lastStatsTime > 30000) { // 每30秒显示一次
    Serial.println("==========================================");
    Serial.println("系统统计信息:");
    Serial.println("Mega2560连接状态: " + String(megaConnected ? "已连接" : "未连接"));
    Serial.println("接收数据总数: " + String(totalDataReceived));
    Serial.println("发送数据总数: " + String(totalDataSent));
    Serial.println("运行时间: " + String(millis() / 1000) + "秒");
    Serial.println("==========================================");
    lastStatsTime = millis();
  }
}

// 获取连接状态
bool isMegaConnected() {
  return megaConnected;
}

// 获取系统状态
String getSystemStatus() {
  String status = "Mega: " + String(megaConnected ? "已连接" : "未连接") + 
                  ", 接收: " + String(totalDataReceived) +
                  ", 发送: " + String(totalDataSent);
  return status;
}
