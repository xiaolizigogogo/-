/*
 * ESP32串口通信版
 * 功能：使用串口进行ESP32间通信
 * 特点：最简单可靠，不需要WiFi或蓝牙
 */

// 串口配置
#define SERIAL_BAUD 115200
#define SERIAL_TIMEOUT 1000

// 全局变量
unsigned long lastHeartbeat = 0;
int messageCount = 0;
String inputBuffer = "";

void setup() {
  // 初始化串口
  Serial.begin(SERIAL_BAUD);
  Serial.setTimeout(SERIAL_TIMEOUT);
  delay(2000);
  
  Serial.println();
  Serial.println("==========================================");
  Serial.println("ESP32串口通信版");
  Serial.println("==========================================");
  
  // 显示系统信息
  displaySystemInfo();
  
  // 初始化GPIO
  pinMode(2, OUTPUT); // 内置LED
  
  Serial.println("==========================================");
  Serial.println("串口通信系统初始化完成！");
  Serial.println("波特率: " + String(SERIAL_BAUD));
  Serial.println("==========================================");
  Serial.println();
  Serial.println("可用命令:");
  Serial.println("  test - 发送测试消息");
  Serial.println("  ping - 发送Ping");
  Serial.println("  status - 获取状态信息");
  Serial.println("  heartbeat - 发送心跳");
  Serial.println("==========================================");
}

void loop() {
  // 处理串口输入
  handleSerialInput();
  
  // 发送心跳
  sendHeartbeat();
  
  // 发送测试消息
  sendTestMessages();
  
  // 闪烁LED
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 1000) {
    lastBlink = millis();
    digitalWrite(2, !digitalRead(2));
  }
  
  // 显示状态信息
  showStatus();
  
  delay(100);
}

// 显示系统信息
void displaySystemInfo() {
  Serial.println("系统信息:");
  Serial.print("  芯片型号: ");
  Serial.println(ESP.getChipModel());
  Serial.print("  可用内存: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
  Serial.print("  CPU温度: ");
  Serial.print(temperatureRead());
  Serial.println("°C");
  Serial.println();
}

// 处理串口输入
void handleSerialInput() {
  while (Serial.available()) {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      if (inputBuffer.length() > 0) {
        processCommand(inputBuffer);
        inputBuffer = "";
      }
    } else {
      inputBuffer += c;
    }
  }
}

// 处理命令
void processCommand(String command) {
  command.trim();
  Serial.println("收到命令: " + command);
  messageCount++;
  
  if (command == "test") {
    Serial.println("test_response: 测试成功 from ESP32");
  } else if (command == "ping") {
    Serial.println("pong: " + String(millis()) + " from ESP32");
  } else if (command == "status") {
    String status = "status_response: memory=" + String(ESP.getFreeHeap()) + 
                   ", temp=" + String(temperatureRead()) + 
                   ", uptime=" + String(millis() / 1000) +
                   ", messages=" + String(messageCount);
    Serial.println(status);
  } else if (command == "heartbeat") {
    Serial.println("heartbeat_response: " + String(millis()) + " from ESP32");
  } else if (command == "help") {
    Serial.println("可用命令: test, ping, status, heartbeat, help");
  } else {
    Serial.println("echo: " + command);
  }
}

// 发送心跳
void sendHeartbeat() {
  if (millis() - lastHeartbeat > 10000) { // 每10秒发送一次
    lastHeartbeat = millis();
    
    String heartbeat = "heartbeat: " + String(millis()) + " from ESP32";
    Serial.println(heartbeat);
  }
}

// 发送测试消息
void sendTestMessages() {
  static unsigned long lastTest = 0;
  if (millis() - lastTest > 30000) { // 每30秒发送一次
    lastTest = millis();
    
    static int testIndex = 0;
    String testMessages[] = {"test: message " + String(testIndex), 
                            "ping: " + String(millis()),
                            "status: request"};
    
    String testMessage = testMessages[testIndex % 3];
    Serial.println(testMessage);
    testIndex++;
  }
}

// 显示状态信息
void showStatus() {
  static unsigned long lastStatusDisplay = 0;
  
  if (millis() - lastStatusDisplay > 30000) { // 每30秒显示一次
    lastStatusDisplay = millis();
    
    Serial.println();
    Serial.println("==========================================");
    Serial.println("串口通信状态");
    Serial.println("==========================================");
    
    Serial.println("通信状态:");
    Serial.println("  消息计数: " + String(messageCount));
    Serial.println("  波特率: " + String(SERIAL_BAUD));
    
    Serial.println("系统状态:");
    Serial.println("  可用内存: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("  CPU温度: " + String(temperatureRead()) + "°C");
    Serial.println("  运行时间: " + String(millis() / 1000) + " 秒");
    
    Serial.println("==========================================");
    Serial.println();
  }
}