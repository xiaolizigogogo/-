/*
 * 数据修复测试程序
 * 尝试修复433MHz模块的数据接收问题
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
 * 3. 观察数据修复效果
 */

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);
  
  Serial.println("=== 数据修复测试程序 ===");
  Serial.println("尝试修复433MHz模块的数据接收问题");
  Serial.println("=================================");
  
  delay(2000);
  
  // 尝试修复模块
  Serial.println(">>> 开始修复模块...");
  
  // 修复方法1：发送同步信号
  Serial.println(">>> 方法1：发送同步信号...");
  for (int i = 0; i < 10; i++) {
    byte sync[] = {0xAA, 0x55, 0xAA, 0x55};
    Serial2.write(sync, 4);
    Serial2.flush();
    delay(100);
  }
  
  // 修复方法2：发送配置命令
  Serial.println(">>> 方法2：发送配置命令...");
  byte config[] = {0xFD, 0x01, 0x00, 0x00, 0x00, 0x00, 0xDF};
  Serial2.write(config, 7);
  Serial2.flush();
  delay(1000);
  
  // 修复方法3：发送配对命令
  Serial.println(">>> 方法3：发送配对命令...");
  byte pair[] = {0xFD, 0x0E, 0x00, 0x00, 0x00, 0x00, 0xDF};
  Serial2.write(pair, 7);
  Serial2.flush();
  delay(1000);
  
  // 修复方法4：发送测试数据
  Serial.println(">>> 方法4：发送测试数据...");
  byte test[] = {0xFD, 0x03, 0xD5, 0xAA, 0x55, 0x96, 0xDF};
  Serial2.write(test, 7);
  Serial2.flush();
  delay(1000);
  
  Serial.println(">>> 模块修复完成，开始接收测试...");
}

void loop() {
  static unsigned long lastReceiveTime = 0;
  static int receiveCount = 0;
  static unsigned long lastStatusTime = 0;
  static int totalBytes = 0;
  static int validFrames = 0;
  static int corruptedData = 0;
  static byte lastByte = 0;
  
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
    delay(300);
    
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
    
    // 检查数据连续性
    if (dataCount > 0) {
      if (lastByte != 0 && receivedData[0] != lastByte + 1) {
        Serial.print(" [数据不连续]");
      }
      lastByte = receivedData[dataCount - 1];
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
  
  // 每30秒重新修复模块
  static unsigned long lastRepairTime = 0;
  if (millis() - lastRepairTime >= 30000) {
    Serial.println(">>> 重新修复模块...");
    
    // 发送同步信号
    byte sync[] = {0xAA, 0x55, 0xAA, 0x55};
    Serial2.write(sync, 4);
    Serial2.flush();
    delay(100);
    
    // 发送配置命令
    byte config[] = {0xFD, 0x01, 0x00, 0x00, 0x00, 0x00, 0xDF};
    Serial2.write(config, 7);
    Serial2.flush();
    delay(1000);
    
    Serial.println(">>> 模块重新修复完成");
    lastRepairTime = millis();
  }
}
