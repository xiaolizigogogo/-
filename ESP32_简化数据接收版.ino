/*
 * ESP32 简化数据接收版
 * 功能：简化数据接收处理，避免编码问题
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
  Serial.println("ESP32 Simple Data Receiver");
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
  Serial.println("System ready, starting simple data reception");
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
      Serial.println("Received data #" + String(dataCount) + ": " + data);
      Serial.println("Data length: " + String(data.length()));
      
      // 处理数据
      processData(data);
      
      // 状态指示
      digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
    }
  }
}

// 处理数据
void processData(String data) {
  // 检查是否包含特定关键字
  if (data.indexOf("MASTER_DATA:") >= 0) {
    Serial.println("Found MASTER_DATA");
    
    // 提取MASTER_DATA部分
    int startIndex = data.indexOf("MASTER_DATA:");
    if (startIndex >= 0) {
      String masterData = data.substring(startIndex);
      Serial.println("MASTER_DATA: " + masterData);
      
      // 解析数据
      if (masterData.length() > 12) {
        String dataPart = masterData.substring(12);
        Serial.println("Data part: " + dataPart);
        
        // 分割数据
        int commaCount = 0;
        for (int i = 0; i < dataPart.length(); i++) {
          if (dataPart.charAt(i) == ',') {
            commaCount++;
          }
        }
        Serial.println("Data fields count: " + String(commaCount + 1));
      }
    }
  } else if (data.indexOf("SLAVE_DATA:") >= 0) {
    Serial.println("Found SLAVE_DATA");
  } else if (data == "INIT_MASTER" || data == "SLAVE_READY") {
    if (!megaConnected) {
      megaConnected = true;
      digitalWrite(STATUS_LED_PIN, HIGH);
      Serial.println("Mega2560 connected successfully!");
    }
  } else {
    Serial.println("Other data: " + data);
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
                  ", Received data: " + String(dataCount);
  return status;
}
