/*
 * ESP32 简单串口测试程序
 * 用于测试与Mega2560的串口通信
 */

// 串口配置
#define MEGA_TX_PIN 1      // ESP32 TX
#define MEGA_RX_PIN 3      // ESP32 RX
#define STATUS_LED_PIN 2   // 状态指示

// 在ESP32上使用Serial1与Mega2560通信
#define megaSerial Serial1

void setup() {
  // 先初始化调试串口
  Serial.begin(115200);
  delay(100);
  
  Serial.println("==========================================");
  Serial.println("ESP32 Simple Serial Test Program");
  Serial.println("==========================================");
  
  // 初始化与Mega2560的通信
  megaSerial.begin(115200, SERIAL_8N1, MEGA_RX_PIN, MEGA_TX_PIN);
  
  // 初始化引脚
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, HIGH);
  
  // 等待启动完成
  delay(3000);
  
  Serial.println("ESP32 started, waiting for Mega2560...");
  
  // 发送测试消息
  megaSerial.println("ESP32_READY");
  Serial.println("Sent ESP32_READY to Mega2560");
}

void loop() {
  // 处理来自Mega2560的数据
  if (megaSerial.available()) {
    String data = megaSerial.readStringUntil('\n');
    data.trim();
    
    Serial.println("Received from Mega2560: " + data);
    
    // Echo data
    if (data == "SLAVE_READY") {
      Serial.println("Mega2560 Slave connected!");
      digitalWrite(STATUS_LED_PIN, LOW);
      delay(100);
      digitalWrite(STATUS_LED_PIN, HIGH);
    }
  }
  
  // Send heartbeat periodically
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 5000) {
    megaSerial.println("ESP32_HEARTBEAT");
    Serial.println("Sent heartbeat to Mega2560");
    lastHeartbeat = millis();
  }
  
  delay(100);
}
