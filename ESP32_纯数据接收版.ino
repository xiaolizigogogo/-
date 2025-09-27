/*
 * ESP32 纯数据接收版
 * 功能：只接收和处理纯数据，不显示调试信息
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

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("==========================================");
  Serial.println("ESP32 Pure Data Receiver");
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
  Serial.println("System ready, starting pure data reception");
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
    // 使用readStringUntil读取完整行
    String data = megaSerial.readStringUntil('\n');
    data.trim();
    
    if (data.length() > 0) {
      dataCount++;
      Serial.println("Data #" + String(dataCount) + ": " + data);
      
      // 处理数据
      processData(data);
      
      // 状态指示
      digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
    }
  }
}

// 处理数据
void processData(String data) {
  // 检查数据格式
  if (data.startsWith("MASTER_DATA:")) {
    Serial.println("Processing MASTER_DATA");
    
    // 提取数据部分
    String dataPart = data.substring(12);
    Serial.println("Data: " + dataPart);
    
    // 分割数据
    int commaCount = 0;
    for (int i = 0; i < dataPart.length(); i++) {
      if (dataPart.charAt(i) == ',') {
        commaCount++;
      }
    }
    Serial.println("Fields: " + String(commaCount + 1));
    
    // 解析具体数据
    parseMasterData(dataPart);
    
  } else if (data.startsWith("SLAVE_DATA:")) {
    Serial.println("Processing SLAVE_DATA");
    
  } else if (data == "INIT_MASTER" || data == "SLAVE_READY") {
    if (!megaConnected) {
      megaConnected = true;
      digitalWrite(STATUS_LED_PIN, HIGH);
      Serial.println("Mega2560 connected successfully!");
    }
    
  } else {
    Serial.println("Unknown data format: " + data);
  }
}

// 解析主设备数据
void parseMasterData(String dataPart) {
  // 分割数据
  String fields[20];
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
  Serial.println("Parsed fields:");
  for (int i = 0; i < fieldCount; i++) {
    Serial.println("  [" + String(i) + "] = " + fields[i]);
  }
  
  // 解析具体字段
  if (fieldCount >= 17) {
    float servoAngle = fields[0].toFloat();
    int servoSpeed = fields[1].toInt();
    int leftTime = fields[2].toInt();
    int rightTime = fields[3].toInt();
    int motorSpeed = fields[4].toInt();
    int centerValue = fields[5].toInt();
    int displayAngle = fields[6].toInt();
    int buttonState = fields[7].toInt();
    
    Serial.println("Parsed values:");
    Serial.println("  Servo Angle: " + String(servoAngle));
    Serial.println("  Servo Speed: " + String(servoSpeed));
    Serial.println("  Left Time: " + String(leftTime));
    Serial.println("  Right Time: " + String(rightTime));
    Serial.println("  Motor Speed: " + String(motorSpeed));
    Serial.println("  Center Value: " + String(centerValue));
    Serial.println("  Display Angle: " + String(displayAngle));
    Serial.println("  Button State: " + String(buttonState));
  }
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
                  ", Data count: " + String(dataCount);
  return status;
}
