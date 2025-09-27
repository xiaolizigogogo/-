/*
 * ESP32 改进数据接收版
 * 功能：改进数据接收处理，解决数据接收不完整的问题
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
unsigned long lastDataTime = 0;

// 数据接收缓冲区
String receivedData = "";
unsigned long lastReceiveTime = 0;

void setup() {
  // 先初始化调试串口
  Serial.begin(115200);
  delay(100);
  
  Serial.println("==========================================");
  Serial.println("ESP32 改进数据接收版启动");
  Serial.println("==========================================");
  
  // 初始化引脚
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  
  // 初始化与Mega2560的通信
  megaSerial.begin(115200, SERIAL_8N1, MEGA_RX_PIN, MEGA_TX_PIN);
  
  // 等待启动完成
  delay(2000);
  
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
}

void loop() {
  // 处理与Mega2560的通信
  handleMegaCommunication();
  
  // 发送心跳
  sendHeartbeat();
  
  // 处理接收到的数据
  processReceivedData();
  
  delay(10);
}

// 处理Mega2560通信
void handleMegaCommunication() {
  // 检查是否有数据可读
  if (megaSerial.available()) {
    // 读取所有可用数据
    while (megaSerial.available()) {
      char c = megaSerial.read();
      
      if (c == '\n' || c == '\r') {
        // 遇到换行符，处理完整数据
        if (receivedData.length() > 0) {
          processCompleteData(receivedData);
          receivedData = ""; // 清空缓冲区
        }
      } else {
        // 添加到接收缓冲区
        receivedData += c;
      }
    }
    
    lastReceiveTime = millis();
  }
  
  // 如果超过1秒没有收到数据，清空缓冲区
  if (millis() - lastReceiveTime > 1000 && receivedData.length() > 0) {
    Serial.println("数据接收超时，清空缓冲区: " + receivedData);
    receivedData = "";
  }
}

// 处理完整数据
void processCompleteData(String data) {
  data.trim(); // 去除首尾空白字符
  
  if (data.length() == 0) {
    Serial.println("收到空数据，忽略");
    return;
  }
  
  Serial.println("收到完整数据: " + data);
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
    Serial.println("未知数据类型: " + data);
  }
  
  lastDataTime = millis();
}

// 处理主设备数据
void handleMasterData(String data) {
  Serial.println("处理主设备数据: " + data);
  
  // 解析数据
  if (data.length() > 12) { // "MASTER_DATA:" 长度为12
    String dataPart = data.substring(12);
    Serial.println("数据部分: " + dataPart);
    
    // 这里可以添加数据解析逻辑
    // 例如：分割逗号分隔的数据
    int commaCount = 0;
    for (int i = 0; i < dataPart.length(); i++) {
      if (dataPart.charAt(i) == ',') {
        commaCount++;
      }
    }
    Serial.println("数据字段数量: " + String(commaCount + 1));
  }
}

// 处理从设备数据
void handleSlaveData(String data) {
  Serial.println("处理从设备数据: " + data);
}

// 处理接收到的数据
void processReceivedData() {
  // 如果超过5秒没有收到数据，显示警告
  if (millis() - lastDataTime > 5000 && lastDataTime > 0) {
    Serial.println("警告：超过5秒没有收到数据");
    lastDataTime = millis(); // 重置计时器
  }
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

// 获取连接状态
bool isMegaConnected() {
  return megaConnected;
}

// 获取系统状态
String getSystemStatus() {
  String status = "Mega: " + String(megaConnected ? "已连接" : "未连接") + 
                  ", 最后数据时间: " + String((millis() - lastDataTime) / 1000) + "秒前";
  return status;
}
