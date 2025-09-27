/*
 * ESP32 调试版本
 * 功能：详细调试数据接收过程
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
  // 先初始化调试串口
  Serial.begin(115200);
  delay(100);
  
  Serial.println("==========================================");
  Serial.println("ESP32 调试版本启动");
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
  
  delay(10);
}

// 处理Mega2560通信
void handleMegaCommunication() {
  // 检查是否有数据可读
  if (megaSerial.available()) {
    Serial.println("检测到数据可读，可用字节数: " + String(megaSerial.available()));
    
    // 读取所有可用数据
    String data = "";
    while (megaSerial.available()) {
      char c = megaSerial.read();
      data += c;
    }
    
    Serial.println("原始数据: [" + data + "]");
    Serial.println("数据长度: " + String(data.length()));
    
    // 显示每个字符的ASCII值
    Serial.print("ASCII值: ");
    for (int i = 0; i < data.length(); i++) {
      Serial.print(String(data.charAt(i), DEC) + " ");
    }
    Serial.println();
    
    // 处理数据
    data.trim();
    if (data.length() > 0) {
      dataCount++;
      Serial.println("处理数据 #" + String(dataCount) + ": " + data);
      
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
    } else {
      Serial.println("数据为空，忽略");
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
  }
}

// 处理从设备数据
void handleSlaveData(String data) {
  Serial.println("处理从设备数据: " + data);
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
