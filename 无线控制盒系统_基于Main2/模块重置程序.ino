/*
 * 模块重置程序
 * 重置433MHz模块到默认状态
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
 * 2. 观察模块重置过程
 * 3. 测试重置后的接收功能
 */

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);
  
  Serial.println("=== 模块重置程序 ===");
  Serial.println("重置433MHz模块到默认状态");
  Serial.println("=================================");
  
  delay(2000);
  
  // 重置模块
  Serial.println(">>> 开始重置模块...");
  
  // 方法1：发送重置命令
  Serial.println(">>> 方法1：发送重置命令...");
  byte reset1[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  Serial2.write(reset1, 8);
  Serial2.flush();
  delay(1000);
  
  // 方法2：发送AT命令
  Serial.println(">>> 方法2：发送AT命令...");
  Serial2.println("AT+RST");
  Serial2.flush();
  delay(1000);
  
  // 方法3：发送配置命令
  Serial.println(">>> 方法3：发送配置命令...");
  byte config[] = {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55};
  Serial2.write(config, 8);
  Serial2.flush();
  delay(1000);
  
  // 方法4：发送配对命令
  Serial.println(">>> 方法4：发送配对命令...");
  byte pair[] = {0xFD, 0x0E, 0x00, 0x00, 0x00, 0x00, 0xDF};
  Serial2.write(pair, 7);
  Serial2.flush();
  delay(1000);
  
  Serial.println(">>> 模块重置完成，开始接收测试...");
}

void loop() {
  static unsigned long lastReceiveTime = 0;
  static int receiveCount = 0;
  static unsigned long lastStatusTime = 0;
  
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
    lastReceiveTime = currentTime;
  }
  
  // 每5秒显示一次状态
  if (millis() - lastStatusTime >= 5000) {
    Serial.print(">>> 状态: 已收到");
    Serial.print(receiveCount);
    Serial.print("次数据");
    if (receiveCount > 0) {
      Serial.print("，最后接收时间: ");
      Serial.print(millis() - lastReceiveTime);
      Serial.print("ms前");
    }
    Serial.println();
    Serial.println(">>> 请用USB-TTL发送数据到433MHz模块测试接收功能");
    lastStatusTime = millis();
  }
  
  // 每10秒发送一次测试数据
  static unsigned long lastTestTime = 0;
  if (millis() - lastTestTime >= 10000) {
    Serial.println(">>> 发送测试数据...");
    byte testData[] = {0xFD, 0x01, 0x02, 0x03, 0x04, 0x05, 0xDF};
    Serial2.write(testData, 7);
    Serial2.flush();
    Serial.println(">>> 测试数据已发送");
    lastTestTime = millis();
  }
}
