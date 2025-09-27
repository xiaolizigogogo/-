/*
 * 模块发送配置程序
 * 配置433MHz模块的发送功能
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
 * 2. 观察模块发送配置过程
 * 3. 测试配置后的发送功能
 */

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);
  
  Serial.println("=== 模块发送配置程序 ===");
  Serial.println("配置433MHz模块的发送功能");
  Serial.println("=================================");
  
  delay(2000);
  
  // 配置模块发送功能
  Serial.println(">>> 开始配置模块发送功能...");
  
  // 配置方法1：发送同步信号
  Serial.println(">>> 方法1：发送同步信号...");
  for (int i = 0; i < 5; i++) {
    byte sync[] = {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55};
    Serial2.write(sync, 8);
    Serial2.flush();
    delay(200);
  }
  
  // 配置方法2：发送配置命令
  Serial.println(">>> 方法2：发送配置命令...");
  byte config[] = {0xFD, 0x01, 0x00, 0x00, 0x00, 0x00, 0xDF};
  Serial2.write(config, 7);
  Serial2.flush();
  delay(1000);
  
  // 配置方法3：发送配对命令
  Serial.println(">>> 方法3：发送配对命令...");
  byte pair[] = {0xFD, 0x0E, 0x00, 0x00, 0x00, 0x00, 0xDF};
  Serial2.write(pair, 7);
  Serial2.flush();
  delay(1000);
  
  // 配置方法4：发送测试数据
  Serial.println(">>> 方法4：发送测试数据...");
  byte test[] = {0xFD, 0x03, 0xD5, 0xAA, 0x55, 0x96, 0xDF};
  Serial2.write(test, 7);
  Serial2.flush();
  delay(1000);
  
  Serial.println(">>> 模块发送配置完成，开始测试...");
}

void loop() {
  static unsigned long lastSendTime = 0;
  static int sendCount = 0;
  static unsigned long lastStatusTime = 0;
  static int receiveCount = 0;
  
  // 每5秒发送一次测试数据
  if (millis() - lastSendTime >= 5000) {
    sendCount++;
    Serial.print(">>> 发送测试数据 #");
    Serial.print(sendCount);
    Serial.print(": ");
    
    // 发送测试数据
    byte testData[] = {0xFD, 0x03, 0xD5, 0xAA, 0x55, 0x96, 0xDF};
    Serial2.write(testData, 7);
    Serial2.flush();
    
    // 显示发送的数据
    for (int i = 0; i < 7; i++) {
      Serial.print("0x");
      if (testData[i] < 0x10) Serial.print("0");
      Serial.print(testData[i], HEX);
      Serial.print(" ");
    }
    Serial.println("(共7字节)");
    
    lastSendTime = millis();
  }
  
  // 检查是否有数据到达
  if (Serial2.available()) {
    receiveCount++;
    Serial.print(">>> 收到数据 #");
    Serial.print(receiveCount);
    Serial.print(": ");
    
    // 读取所有可用数据
    byte receivedData[50];
    int dataCount = 0;
    
    // 等待数据稳定
    delay(100);
    
    while (Serial2.available() && dataCount < 50) {
      receivedData[dataCount] = Serial2.read();
      dataCount++;
    }
    
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
    for (int i = 0; i <= dataCount - 7; i++) {
      if (receivedData[i] == 0xFD && receivedData[i + 6] == 0xDF) {
        Serial.print(" [找到数据帧]");
        break;
      }
    }
    
    Serial.println();
  }
  
  // 每10秒显示一次状态
  if (millis() - lastStatusTime >= 10000) {
    Serial.println("=================================");
    Serial.print(">>> 统计信息:");
    Serial.print(" 发送次数: ");
    Serial.print(sendCount);
    Serial.print(", 接收次数: ");
    Serial.print(receiveCount);
    Serial.println();
    Serial.println(">>> 请用USB-TTL监听433MHz模块的发送数据");
    Serial.println(">>> 如果USB-TTL收到数据，说明发送功能正常");
    Serial.println("=================================");
    lastStatusTime = millis();
  }
  
  // 每30秒重新配置模块
  static unsigned long lastConfigTime = 0;
  if (millis() - lastConfigTime >= 30000) {
    Serial.println(">>> 重新配置模块发送功能...");
    
    // 发送同步信号
    byte sync[] = {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55};
    Serial2.write(sync, 8);
    Serial2.flush();
    delay(200);
    
    // 发送配置命令
    byte config[] = {0xFD, 0x01, 0x00, 0x00, 0x00, 0x00, 0xDF};
    Serial2.write(config, 7);
    Serial2.flush();
    delay(1000);
    
    Serial.println(">>> 模块发送功能重新配置完成");
    lastConfigTime = millis();
  }
}
