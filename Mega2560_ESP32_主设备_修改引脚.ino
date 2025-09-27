/*
 * Mega2560 ESP32主设备 - 修改引脚版本
 * 功能：连接ESP32，实现无线数据同步
 * 使用Serial3避免与烧录引脚冲突
 */

// ESP32通信相关 - 修改引脚
#define ESP32_TX_PIN 14    // Mega2560 TX3 (原来是TX2 Pin 16)
#define ESP32_RX_PIN 15    // Mega2560 RX3 (原来是RX2 Pin 17)
#define ESP32_STATUS_PIN 2  // 状态指示

// 在Mega2560上使用Serial3与ESP32通信
#define esp32Serial Serial3

// 系统状态
bool esp32Connected = false;
unsigned long lastDataSync = 0;
const unsigned long SYNC_INTERVAL = 1000; // 1秒同步一次

// 原有的业务逻辑变量
struct Val {
  float servoAngle = 106.0;
  int servoSpeed = 4;
  int leftTime = 23;
  int rightTime = 3;
  int motorSpeed = 0;
  int centerValue = 1202;
  int displayAngle = 35;
  int buttonState = 1;
  int encoder0 = 0;
  int encoder1 = 0;
  int encoder2 = 0;
  int encoder3 = 0;
  int encoder4 = 0;
} Val;

int ecCnt[5] = {70, 17, 31, 102, 1};

void setup() {
  Serial.begin(115200);
  
  Serial.println("==========================================");
  Serial.println("Mega2560 ESP32主设备启动 - 修改引脚版本");
  Serial.println("==========================================");
  
  // 初始化ESP32通信
  initESP32Communication();
  
  // 初始化其他硬件
  initHardware();
  
  Serial.println("Mega2560主设备初始化完成！");
}

void loop() {
  // 处理ESP32通信
  handleESP32Communication();
  
  // 原有的业务逻辑
  handleBusinessLogic();
  
  // 同步数据到从设备
  syncDataToSlave();
  
  delay(10);
}

// 初始化ESP32通信
void initESP32Communication() {
  esp32Serial.begin(115200);
  pinMode(ESP32_STATUS_PIN, OUTPUT);
  digitalWrite(ESP32_STATUS_PIN, LOW);

  Serial.println("初始化ESP32通信...");
  Serial.println("等待ESP32启动...");

  // 等待ESP32发送ESP32_READY
  unsigned long timeout = millis() + 15000; // 增加超时时间到15秒
  while (millis() < timeout) {
    if (esp32Serial.available()) {
      String response = esp32Serial.readStringUntil('\n');
      response.trim();

      Serial.println("收到ESP32消息: " + response);

      if (response == "ESP32_READY") {
        esp32Connected = true;
        digitalWrite(ESP32_STATUS_PIN, HIGH);
        Serial.println("ESP32连接成功！");

        // 发送初始化命令
        sendToESP32("INIT_MASTER");
        return;
      }
    }
    delay(100);
  }

  esp32Connected = false;
  digitalWrite(ESP32_STATUS_PIN, LOW);
  Serial.println("ESP32连接超时！");
  Serial.println("将在loop()中继续尝试连接...");
}

// 处理ESP32通信
void handleESP32Communication() {
  if (esp32Serial.available()) {
    String data = esp32Serial.readStringUntil('\n');
    data.trim();

    Serial.println("收到ESP32数据: " + data);

    // 解析从设备状态数据
    if (data.startsWith("SLAVE_STATUS:")) {
      parseSlaveStatus(data);
    } else if (data.startsWith("SLAVE_DATA:")) {
      parseSlaveData(data);
    } else if (data == "ESP32_READY") {
      if (!esp32Connected) {
        esp32Connected = true;
        digitalWrite(ESP32_STATUS_PIN, HIGH);
        Serial.println("ESP32连接成功！");
        // 发送初始化命令
        sendToESP32("INIT_MASTER");
      }
    }
  }
}

// 发送数据到ESP32
void sendToESP32(String data) {
  if (esp32Connected) {
    esp32Serial.println(data);
    Serial.println("发送数据到ESP32: " + data);
  }
}

// 同步数据到从设备
void syncDataToSlave() {
  if (millis() - lastDataSync > SYNC_INTERVAL) {
    if (esp32Connected) {
      sendCurrentData();
    }
    lastDataSync = millis();
  }
}

// 发送当前数据
void sendCurrentData() {
  String data = "MASTER_DATA:" + String(Val.servoAngle, 2) + "," +
                String(Val.servoSpeed) + "," +
                String(Val.leftTime) + "," +
                String(Val.rightTime) + "," +
                String(Val.motorSpeed) + "," +
                String(Val.centerValue) + "," +
                String(Val.displayAngle) + "," +
                String(Val.buttonState) + "," +
                String(Val.encoder0) + "," +
                String(Val.encoder1) + "," +
                String(Val.encoder2) + "," +
                String(Val.encoder3) + "," +
                String(Val.encoder4) + "," +
                String(ecCnt[0]) + "," +
                String(ecCnt[1]) + "," +
                String(ecCnt[2]) + "," +
                String(ecCnt[3]) + "," +
                String(ecCnt[4]);
  
  sendToESP32(data);
}

// 解析从设备状态
void parseSlaveStatus(String data) {
  Serial.println("解析从设备状态: " + data);
  // 这里可以添加状态解析逻辑
}

// 解析从设备数据
void parseSlaveData(String data) {
  Serial.println("解析从设备数据: " + data);
  // 这里可以添加数据解析逻辑
}

// 初始化硬件
void initHardware() {
  Serial.println("初始化硬件...");
  
  // 模拟硬件初始化
  Serial.println("编码器初始化完成");
  Serial.println("按键初始化完成");
  Serial.println("舵机初始化完成");
  Serial.println("电机初始化完成");
}

// 处理业务逻辑
void handleBusinessLogic() {
  // 模拟业务逻辑处理
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 100) {
    // 模拟数据变化
    Val.servoAngle += 0.1;
    if (Val.servoAngle > 180) Val.servoAngle = 0;
    
    ecCnt[0] = (ecCnt[0] + 1) % 100;
    ecCnt[1] = (ecCnt[1] + 1) % 100;
    
    lastUpdate = millis();
  }
}
