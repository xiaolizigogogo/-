/*
 * ESP32 调试串口通信版
 * 功能：显示更清晰的调试信息，避免乱码问题
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

// 数据接收缓冲区
String receivedData = "";
unsigned long lastReceiveTime = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("==========================================");
  Serial.println("ESP32 Debug Serial Communication");
  Serial.println("==========================================");
  
  // 初始化引脚
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  
  // 初始化与Mega2560的通信
  megaSerial.begin(115200, SERIAL_8N1, MEGA_RX_PIN, MEGA_TX_PIN);
  
  Serial.println("ESP32 initialization completed!");
  
  // 发送初始化响应
  megaSerial.println("ESP32_READY");
  Serial.println("Sent ESP32_READY to Mega2560");
  
  // 等待Mega2560响应
  Serial.println("Waiting for Mega2560 response...");
  unsigned long timeout = millis() + 15000;
  while (millis() < timeout) {
    if (megaSerial.available()) {
      String response = megaSerial.readStringUntil('\n');
      response.trim();
      
      Serial.println("Received from Mega2560: " + response);
      
      if (response == "INIT_MASTER" || response == "SLAVE_READY") {
        megaConnected = true;
        digitalWrite(STATUS_LED_PIN, HIGH);
        Serial.println("Mega2560 connected successfully!");
        break;
      }
    }
    delay(100);
  }
  
  if (!megaConnected) {
    Serial.println("Mega2560 connection timeout!");
    Serial.println("Will continue trying in loop()...");
  }
  
  Serial.println("==========================================");
  Serial.println("System ready, starting data communication");
  Serial.println("==========================================");
}

void loop() {
  // 处理与Mega2560的通信
  handleMegaCommunication();
  
  // 发送心跳
  sendHeartbeat();
  
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
  
  // 如果超过2秒没有收到数据，清空缓冲区
  if (millis() - lastReceiveTime > 2000 && receivedData.length() > 0) {
    Serial.println("Data receive timeout, clearing buffer: " + receivedData);
    receivedData = "";
  }
}

// 处理完整数据
void processCompleteData(String data) {
  data.trim(); // 去除首尾空白字符
  
  if (data.length() == 0) {
    Serial.println("Received empty data, ignoring");
    return;
  }
  
  dataCount++;
  Serial.println("Received complete data #" + String(dataCount) + ": " + data);
  Serial.println("Data length: " + String(data.length()));
  
  // 处理不同类型的数据
  if (data.startsWith("MASTER_DATA:")) {
    handleMasterData(data);
  } else if (data.startsWith("SLAVE_STATUS:") || data.startsWith("SLAVE_DATA:")) {
    handleSlaveData(data);
  } else if (data == "INIT_MASTER" || data == "SLAVE_READY") {
    if (!megaConnected) {
      megaConnected = true;
      digitalWrite(STATUS_LED_PIN, HIGH);
      Serial.println("Mega2560 connected successfully!");
    }
  } else {
    Serial.println("Other data: " + data);
  }
  
  // 状态指示
  digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
}

// 处理主设备数据
void handleMasterData(String data) {
  Serial.println("Processing master data: " + data);
  
  // 解析数据
  if (data.length() > 12) { // "MASTER_DATA:" 长度为12
    String dataPart = data.substring(12);
    Serial.println("Data part: " + dataPart);
    
    // 分割数据
    int commaCount = 0;
    for (int i = 0; i < dataPart.length(); i++) {
      if (dataPart.charAt(i) == ',') {
        commaCount++;
      }
    }
    Serial.println("Data fields count: " + String(commaCount + 1));
    
    // 解析具体数据
    parseMasterData(dataPart);
  }
}

// 解析主设备数据
void parseMasterData(String dataPart) {
  // 分割数据
  String fields[20]; // 最多20个字段
  int fieldCount = 0;
  int lastIndex = 0;
  
  for (int i = 0; i < dataPart.length() && fieldCount < 20; i++) {
    if (dataPart.charAt(i) == ',') {
      fields[fieldCount] = dataPart.substring(lastIndex, i);
      fieldCount++;
      lastIndex = i + 1;
    }
  }
  
  // 添加最后一个字段
  if (lastIndex < dataPart.length()) {
    fields[fieldCount] = dataPart.substring(lastIndex);
    fieldCount++;
  }
  
  // 显示解析结果
  Serial.println("Parsed data fields:");
  for (int i = 0; i < fieldCount; i++) {
    Serial.println("  Field " + String(i) + ": " + fields[i]);
  }
}

// 处理从设备数据
void handleSlaveData(String data) {
  Serial.println("Processing slave data: " + data);
}

// 发送心跳
void sendHeartbeat() {
  if (millis() - lastHeartbeat > 5000) { // 每5秒发送一次
    if (megaConnected) {
      megaSerial.println("ESP32_HEARTBEAT");
      Serial.println("Sent heartbeat to Mega2560");
    } else {
      // 如果未连接，继续尝试发送ESP32_READY
      megaSerial.println("ESP32_READY");
      Serial.println("Continuing to try connecting to Mega2560...");
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
  String status = "Mega: " + String(megaConnected ? "Connected" : "Disconnected") + 
                  ", Received data: " + String(dataCount);
  return status;
}
