/*
 * ESP32 分步测试版
 * 功能：分步测试各个功能，找出问题所在
 */

// 串口配置
#define MEGA_TX_PIN 16     // ESP32 TX
#define MEGA_RX_PIN 17     // ESP32 RX
#define STATUS_LED_PIN 2   // 状态指示

// 在ESP32上使用Serial2与Mega2560通信
#define megaSerial Serial2

// 系统状态
bool megaConnected = false;
int testStep = 0;
unsigned long lastTestTime = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("==========================================");
  Serial.println("ESP32 分步测试版启动");
  Serial.println("==========================================");
  
  // 步骤1: 初始化引脚
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  Serial.println("步骤1: 引脚初始化完成");
  
  // 步骤2: 初始化串口
  megaSerial.begin(115200, SERIAL_8N1, MEGA_RX_PIN, MEGA_TX_PIN);
  Serial.println("步骤2: 串口初始化完成");
  
  // 步骤3: 等待启动完成
  delay(2000);
  Serial.println("步骤3: 启动延迟完成");
  
  // 步骤4: 发送初始化消息
  megaSerial.println("ESP32_READY");
  Serial.println("步骤4: 发送ESP32_READY到Mega2560");
  
  // 步骤5: 等待Mega2560响应
  Serial.println("步骤5: 等待Mega2560响应...");
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
  
  Serial.println("==========================================");
  Serial.println("初始化完成，开始测试循环");
  Serial.println("==========================================");
}

void loop() {
  // 处理与Mega2560的通信
  handleMegaCommunication();
  
  // 分步测试
  performStepTest();
  
  delay(10);
}

// 处理Mega2560通信
void handleMegaCommunication() {
  if (megaSerial.available()) {
    String data = megaSerial.readStringUntil('\n');
    data.trim();
    
    if (data.length() > 0) {
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
}

// 处理主设备数据
void handleMasterData(String data) {
  Serial.println("处理主设备数据: " + data);
}

// 处理从设备数据
void handleSlaveData(String data) {
  Serial.println("处理从设备数据: " + data);
}

// 分步测试
void performStepTest() {
  if (millis() - lastTestTime > 10000) { // 每10秒执行一个测试步骤
    testStep++;
    
    switch (testStep) {
      case 1:
        Serial.println("测试步骤1: 发送心跳");
        if (megaConnected) {
          megaSerial.println("ESP32_HEARTBEAT");
          Serial.println("发送心跳到Mega2560");
        }
        break;
        
      case 2:
        Serial.println("测试步骤2: 状态指示");
        digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
        break;
        
      case 3:
        Serial.println("测试步骤3: 内存检查");
        Serial.println("可用内存: " + String(ESP.getFreeHeap()) + " bytes");
        break;
        
      case 4:
        Serial.println("测试步骤4: 系统状态");
        Serial.println("Mega2560连接: " + String(megaConnected ? "已连接" : "未连接"));
        Serial.println("运行时间: " + String(millis() / 1000) + "秒");
        testStep = 0; // 重置测试步骤
        break;
    }
    
    lastTestTime = millis();
  }
}
