/*
 * 模块配置测试程序
 * 测试433MHz模块的配置和接收功能
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
 * 2. 观察模块的配置和接收状态
 * 3. 用USB-TTL发送数据测试接收
 */

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);
  
  Serial.println("=== 模块配置测试程序 ===");
  Serial.println("测试433MHz模块的配置和接收功能");
  Serial.println("=================================");
  
  delay(2000);
  
  // 测试模块配置
  Serial.println(">>> 开始模块配置测试...");
  
  // 发送配置命令
  Serial.println(">>> 发送配置命令...");
  
  // 配置命令1：设置工作模式
  byte config1[] = {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55};
  Serial2.write(config1, 8);
  Serial2.flush();
  Serial.println(">>> 配置命令1已发送");
  delay(1000);
  
  // 配置命令2：设置接收模式
  byte config2[] = {0xFD, 0x01, 0x00, 0x00, 0x00, 0x00, 0xDF};
  Serial2.write(config2, 7);
  Serial2.flush();
  Serial.println(">>> 配置命令2已发送");
  delay(1000);
  
  // 配置命令3：设置配对模式
  byte config3[] = {0xFD, 0x0E, 0x00, 0x00, 0x00, 0x00, 0xDF};
  Serial2.write(config3, 7);
  Serial2.flush();
  Serial.println(">>> 配置命令3已发送");
  delay(1000);
  
  Serial.println(">>> 模块配置完成，开始接收测试...");
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
  
  // 每30秒重新配置模块
  static unsigned long lastConfigTime = 0;
  if (millis() - lastConfigTime >= 30000) {
    Serial.println(">>> 重新配置模块...");
    
    // 重新发送配置命令
    byte config[] = {0xFD, 0x01, 0x00, 0x00, 0x00, 0x00, 0xDF};
    Serial2.write(config, 7);
    Serial2.flush();
    Serial.println(">>> 模块重新配置完成");
    
    lastConfigTime = millis();
  }
}
