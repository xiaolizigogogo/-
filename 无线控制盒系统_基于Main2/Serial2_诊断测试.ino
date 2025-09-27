/*
 * Serial2诊断测试程序
 * 用于测试Arduino Mega 2560的Serial2与433MHz模块的通信
 * 
 * 接线：
 * Arduino Mega 2560    →    433MHz模块
 * 引脚16 (TX)         →    RXD
 * 引脚17 (RX)         →    TXD
 * GND                 →    GND
 * 5V/3.3V             →    VCC
 */

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);
  
  Serial.println("=== Serial2诊断测试程序 ===");
  Serial.println("用于测试433MHz模块通信");
  Serial.println("波特率: 9600bps");
  Serial.println("数据格式: FD XXXXXXXXXX DF (7个字节)");
  Serial.println("================================");
  
  delay(2000);
  
  // 测试1: 发送测试数据
  Serial.println(">>> 测试1: 发送测试数据");
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
  
  // 等待响应
  delay(500);
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
  
  delay(2000);
  
  // 测试2: 发送配对请求
  Serial.println(">>> 测试2: 发送配对请求");
  byte pairingData[7] = {0xFD, 0x0E, 0x00, 0x00, 0x00, 0x00, 0xDF};
  
  Serial.print("发送数据: ");
  for (int i = 0; i < 7; i++) {
    Serial.print("0x");
    if (pairingData[i] < 0x10) Serial.print("0");
    Serial.print(pairingData[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  bytesWritten = Serial2.write(pairingData, 7);
  Serial.print("实际发送字节数: ");
  Serial.println(bytesWritten);
  Serial2.flush();
  
  // 等待响应
  delay(500);
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
  
  Serial.println("================================");
  Serial.println("测试完成！");
  Serial.println("如果看到'0xF0'响应，说明模块工作正常");
  Serial.println("如果没有响应，请检查：");
  Serial.println("1. 接线是否正确");
  Serial.println("2. 模块电源是否正常");
  Serial.println("3. 模块是否工作");
}

void loop() {
  // 每5秒发送一次测试数据
  static unsigned long lastTest = 0;
  if (millis() - lastTest >= 5000) {
    Serial.println(">>> 循环测试: 发送测试数据");
    
    byte testData[7] = {0xFD, 0x03, 0x06, 0x05, 0x01, 0x60, 0xDF};
    
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
    
    // 等待响应
    delay(200);
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
    
    lastTest = millis();
  }
}
