/*
 * 数据完整性测试程序
 * 测试433MHz模块的数据接收完整性
 * 
 * 接线：
 * Arduino Mega 2560    →    433MHz发射接收一体模块
 * 引脚16 (TX)         →    RXD
 * 引脚17 (RX)         →    TXD
 * GND                 →    GND
 * 5V/3.3V             →    VCC
 * 
 * 使用方法：
 * 1. 上传此程序到Arduino设备
 * 2. 用USB-TTL发送数据到433MHz模块
 * 3. 观察数据接收的完整性
 */

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);
  
  Serial.println("=== 数据完整性测试程序 ===");
  Serial.println("测试433MHz模块的数据接收完整性");
  Serial.println("=================================");
  
  delay(2000);
  
  // 清空接收缓冲区
  while (Serial2.available()) {
    Serial2.read();
  }
  
  Serial.println(">>> 接收缓冲区已清空，开始测试...");
}

void loop() {
  static unsigned long lastReceiveTime = 0;
  static int receiveCount = 0;
  static unsigned long lastStatusTime = 0;
  static int totalBytes = 0;
  static int validFrames = 0;
  static int corruptedData = 0;
  
  // 检查是否有数据到达
  if (Serial2.available()) {
    receiveCount++;
    unsigned long currentTime = millis();
    
    Serial.print(">>> 收到数据 #");
    Serial.print(receiveCount);
    Serial.print(" (时间: ");
    Serial.print(currentTime);
    Serial.print("ms): ");
    
    // 读取所有可用数据
    byte receivedData[100];
    int dataCount = 0;
    
    // 等待数据稳定 - 增加等待时间
    delay(200);
    
    while (Serial2.available() && dataCount < 100) {
      receivedData[dataCount] = Serial2.read();
      dataCount++;
    }
    
    totalBytes += dataCount;
    
    // 显示接收到的数据
    for (int i = 0; i < dataCount; i++) {
      Serial.print("0x");
      if (receivedData[i] < 0x10) Serial.print("0");
      Serial.print(receivedData[i], HEX);
      Serial.print(" ");
    }
    Serial.print("(共");
    Serial.print(dataCount);
    Serial.print("字节)");
    
    // 检查数据帧
    bool foundFrame = false;
    for (int i = 0; i <= dataCount - 7; i++) {
      if (receivedData[i] == 0xFD && receivedData[i + 6] == 0xDF) {
        Serial.print(" [找到数据帧]");
        validFrames++;
        foundFrame = true;
        break;
      }
    }
    
    // 检查数据质量
    if (dataCount == 1 && receivedData[0] == 0x00) {
      Serial.print(" [数据损坏]");
      corruptedData++;
    } else if (dataCount > 0 && !foundFrame) {
      Serial.print(" [部分数据]");
    }
    
    Serial.println();
    lastReceiveTime = currentTime;
  }
  
  // 每10秒显示一次详细状态
  if (millis() - lastStatusTime >= 10000) {
    Serial.println("=================================");
    Serial.print(">>> 统计信息:");
    Serial.print(" 总接收次数: ");
    Serial.print(receiveCount);
    Serial.print(", 总字节数: ");
    Serial.print(totalBytes);
    Serial.print(", 有效帧数: ");
    Serial.print(validFrames);
    Serial.print(", 损坏数据: ");
    Serial.print(corruptedData);
    Serial.println();
    
    if (receiveCount > 0) {
      Serial.print(">>> 最后接收时间: ");
      Serial.print(millis() - lastReceiveTime);
      Serial.print("ms前");
      Serial.println();
    }
    
    Serial.println(">>> 请用USB-TTL发送数据到433MHz模块测试接收功能");
    Serial.println(">>> 建议发送: FD 03 D5 AA 55 96 DF");
    Serial.println("=================================");
    lastStatusTime = millis();
  }
  
  // 每30秒清空接收缓冲区
  static unsigned long lastClearTime = 0;
  if (millis() - lastClearTime >= 30000) {
    Serial.println(">>> 清空接收缓冲区...");
    while (Serial2.available()) {
      Serial2.read();
    }
    Serial.println(">>> 接收缓冲区已清空");
    lastClearTime = millis();
  }
}
