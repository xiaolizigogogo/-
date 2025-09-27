/*
 * ESP32 原始数据接收版
 * 功能：接收所有原始数据，显示ASCII值，不进行任何处理
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
  Serial.println("ESP32 Raw Data Receiver");
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
  Serial.println("System ready, starting raw data reception");
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
    String data = "";
    while (megaSerial.available()) {
      char c = megaSerial.read();
      data += c;
    }
    
    if (data.length() > 0) {
      dataCount++;
      Serial.println("Received raw data #" + String(dataCount) + ": [" + data + "]");
      Serial.println("Data length: " + String(data.length()));
      
      // 显示每个字符的ASCII值
      Serial.print("ASCII values: ");
      for (int i = 0; i < data.length(); i++) {
        Serial.print(String(data.charAt(i), DEC) + " ");
      }
      Serial.println();
      
      // 显示每个字符
      Serial.print("Characters: ");
      for (int i = 0; i < data.length(); i++) {
        char c = data.charAt(i);
        if (c >= 32 && c <= 126) {
          Serial.print(c);
        } else {
          Serial.print("[" + String(c, DEC) + "]");
        }
      }
      Serial.println();
      
      // 处理数据
      processRawData(data);
      
      // 状态指示
      digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
    }
  }
}

// 处理原始数据
void processRawData(String data) {
  data.trim();
  
  if (data.length() == 0) {
    Serial.println("Empty data, ignoring");
    return;
  }
  
  // 检查是否包含特定关键字
  if (data.indexOf("MASTER_DATA:") >= 0) {
    Serial.println("Found MASTER_DATA in raw data");
    
    // 提取MASTER_DATA部分
    int startIndex = data.indexOf("MASTER_DATA:");
    if (startIndex >= 0) {
      String masterData = data.substring(startIndex);
      Serial.println("Extracted MASTER_DATA: " + masterData);
      
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
    Serial.println("Found SLAVE_DATA in raw data");
  } else if (data == "INIT_MASTER" || data == "SLAVE_READY") {
    if (!megaConnected) {
      megaConnected = true;
      digitalWrite(STATUS_LED_PIN, HIGH);
      Serial.println("Mega2560 connected successfully!");
    }
  } else {
    Serial.println("Other raw data: " + data);
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
