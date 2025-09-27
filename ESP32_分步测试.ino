/*
 * ESP32 分步测试程序
 * 功能：逐步测试各个功能，找出问题所在
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
int testStep = 0;

void setup() {
  // 先初始化调试串口
  Serial.begin(115200);
  delay(100);
  
  Serial.println("==========================================");
  Serial.println("ESP32 分步测试程序启动");
  Serial.println("==========================================");
  
  // 初始化引脚
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  
  Serial.println("步骤1: 引脚初始化完成");
  
  // 初始化与Mega2560的通信
  megaSerial.begin(115200, SERIAL_8N1, MEGA_RX_PIN, MEGA_TX_PIN);
  
  Serial.println("步骤2: 串口初始化完成");
  
  // 等待启动完成
  delay(2000);
  
  Serial.println("步骤3: 启动延迟完成");
  
  // 发送初始化响应
  megaSerial.println("ESP32_READY");
  Serial.println("步骤4: 发送ESP32_READY到Mega2560");
  
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
        Serial.println("步骤5: Mega2560连接成功！");
        break;
      }
    }
    delay(100);
  }
  
  if (!megaConnected) {
    Serial.println("步骤5: Mega2560连接超时！");
  }
  
  Serial.println("ESP32分步测试初始化完成！");
}

void loop() {
  // 处理与Mega2560的通信
  handleMegaCommunication();
  
  // 发送心跳
  sendHeartbeat();
  
  // 状态指示
  if (millis() % 2000 < 100) {
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
  }
  
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
  Serial.println("处理主设备数据: " + data);
  // 这里可以添加数据处理逻辑
}

// 处理从设备数据
void handleSlaveData(String data) {
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
