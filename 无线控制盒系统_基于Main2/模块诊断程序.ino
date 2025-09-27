/*
 * 模块诊断程序
 * 用于诊断433MHz模块的发送和接收功能
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
 * 2. 观察串口输出，检查模块状态
 * 3. 用USB-TTL测试模块验证发送功能
 */

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);
  
  Serial.println("=== 433MHz模块诊断程序 ===");
  Serial.println("用于诊断模块的发送和接收功能");
  Serial.println("=================================");
  
  delay(2000);
  
  // 测试1：发送简单数据
  Serial.println(">>> 测试1：发送简单数据");
  byte testData1[7] = {0xFD, 0x01, 0x02, 0x03, 0x04, 0x05, 0xDF};
  
  Serial.print("发送数据: ");
  for (int i = 0; i < 7; i++) {
    Serial.print("0x");
    if (testData1[i] < 0x10) Serial.print("0");
    Serial.print(testData1[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  size_t bytesWritten = Serial2.write(testData1, 7);
  Serial.print("实际发送字节数: ");
  Serial.println(bytesWritten);
  Serial2.flush();
  
  delay(1000);
  
  // 测试2：发送配对请求格式
  Serial.println(">>> 测试2：发送配对请求格式");
  byte testData2[7] = {0xFD, 0x0E, 0x00, 0x00, 0x00, 0x00, 0xDF};
  
  Serial.print("发送数据: ");
  for (int i = 0; i < 7; i++) {
    Serial.print("0x");
    if (testData2[i] < 0x10) Serial.print("0");
    Serial.print(testData2[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  bytesWritten = Serial2.write(testData2, 7);
  Serial.print("实际发送字节数: ");
  Serial.println(bytesWritten);
  Serial2.flush();
  
  delay(1000);
  
  // 测试3：发送测试数据格式
  Serial.println(">>> 测试3：发送测试数据格式");
  byte testData3[7] = {0xFD, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0xDF};
  
  Serial.print("发送数据: ");
  for (int i = 0; i < 7; i++) {
    Serial.print("0x");
    if (testData3[i] < 0x10) Serial.print("0");
    Serial.print(testData3[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  bytesWritten = Serial2.write(testData3, 7);
  Serial.print("实际发送字节数: ");
  Serial.println(bytesWritten);
  Serial2.flush();
  
  delay(1000);
  
  // 测试4：发送单个字节
  Serial.println(">>> 测试4：发送单个字节");
  Serial.println("发送字节: 0xAA");
  
  bytesWritten = Serial2.write(0xAA);
  Serial.print("实际发送字节数: ");
  Serial.println(bytesWritten);
  Serial2.flush();
  
  delay(1000);
  
  // 测试5：发送多个字节
  Serial.println(">>> 测试5：发送多个字节");
  byte testData5[5] = {0xAA, 0x55, 0xAA, 0x55, 0xAA};
  
  Serial.print("发送数据: ");
  for (int i = 0; i < 5; i++) {
    Serial.print("0x");
    if (testData5[i] < 0x10) Serial.print("0");
    Serial.print(testData5[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  bytesWritten = Serial2.write(testData5, 5);
  Serial.print("实际发送字节数: ");
  Serial.println(bytesWritten);
  Serial2.flush();
  
  delay(1000);
  
  Serial.println(">>> 诊断完成");
  Serial.println("请检查USB-TTL测试模块是否收到数据");
  Serial.println("如果USB-TTL没有收到数据，说明模块发送功能有问题");
  Serial.println("=================================");
}

void loop() {
  static unsigned long lastSendTime = 0;
  static int testCount = 0;
  
  unsigned long currentTime = millis();
  
  // 每5秒发送一次测试数据
  if (currentTime - lastSendTime >= 5000) {
    testCount++;
    Serial.print(">>> 循环测试 #");
    Serial.println(testCount);
    
    // 发送测试数据
    byte testData[7] = {0xFD, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0xDF};
    
    Serial.print("发送数据: ");
    for (int i = 0; i < 7; i++) {
      Serial.print("0x");
      if (testData[i] < 0x10) Serial.print("0");
      Serial.print(testData[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    
    size_t bytesWritten = Serial2.write(testData, 7);
    Serial.print("实际发送字节数: ");
    Serial.println(bytesWritten);
    Serial2.flush();
    
    lastSendTime = currentTime;
  }
  
  // 检查接收数据
  if (Serial2.available()) {
    Serial.print(">>> 收到数据: ");
    while (Serial2.available()) {
      byte data = Serial2.read();
      Serial.print("0x");
      if (data < 0x10) Serial.print("0");
      Serial.print(data, HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
}
