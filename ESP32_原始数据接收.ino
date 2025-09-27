/*
 * ESP32 原始数据接收版
 * 功能：接收所有原始数据，不进行任何处理
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
  Serial.println("ESP32 原始数据接收版启动");
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
    // 读取所有可用数据
    String data = "";
    while (megaSerial.available()) {
      char c = megaSerial.read();
      data += c;
    }
    
    if (data.length() > 0) {
      dataCount++;
      Serial.println("收到原始数据 #" + String(dataCount) + ": [" + data + "]");
      Serial.println("数据长度: " + String(data.length()));
      
      // 显示每个字符
      Serial.print("字符: ");
      for (int i = 0; i < data.length(); i++) {
        char c = data.charAt(i);
        if (c >= 32 && c <= 126) {
          Serial.print(c);
        } else {
          Serial.print("[" + String(c, DEC) + "]");
        }
      }
      Serial.println();
    }
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
