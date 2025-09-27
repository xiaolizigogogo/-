/*
 * 简单通信测试程序
 * 用于验证433MHz发射接收一体模块的基本通信
 * 
 * 接线：
 * Arduino Mega 2560    →    433MHz发射接收一体模块
 * 引脚16 (TX)         →    RXD
 * 引脚17 (RX)         →    TXD
 * GND                 →    GND
 * 5V/3.3V             →    VCC
 * 
 * 使用方法：
 * 1. 上传此程序到两个Arduino设备
 * 2. 观察串口输出，看是否能收到对方的数据
 */

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);
  
  Serial.println("=== 简单通信测试程序 ===");
  Serial.println("用于验证433MHz发射接收一体模块");
  Serial.println("=================================");
  
  delay(2000);
  
  // 发送测试数据
  Serial.println(">>> 发送测试数据...");
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
  
  // 等待接收
  delay(1000);
  if (Serial2.available()) {
    Serial.print("收到响应: ");
    while (Serial2.available()) {
      byte response = Serial2.read();
      Serial.print("0x");
      if (response < 0x10) Serial.print("0");
      Serial.print(response, HEX);
      Serial.print(" ");
    }
    Serial.println();
  } else {
    Serial.println("未收到响应");
  }
}

void loop() {
  static unsigned long lastSendTime = 0;
  static unsigned long lastReceiveCheck = 0;
  static int sendCount = 0;
  
  unsigned long currentTime = millis();
  
  // 每5秒发送一次测试数据
  if (currentTime - lastSendTime >= 5000) {
    sendCount++;
    Serial.print(">>> 发送测试数据 #");
    Serial.println(sendCount);
    
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
  
  // 持续监听接收数据
  if (currentTime - lastReceiveCheck >= 1000) {
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
    lastReceiveCheck = currentTime;
  }
}
