/*
 * Serial2硬件诊断程序
 * 用于诊断Arduino Mega 2560的Serial2硬件连接问题
 */

void setup() {
  Serial.begin(115200);
  
  Serial.println("=== Serial2硬件诊断程序 ===");
  Serial.println("用于诊断433MHz模块连接问题");
  Serial.println("================================");
  
  // 检查板子类型
  #if defined(__AVR_ATmega2560__)
    Serial.println("✓ 检测到Arduino Mega 2560");
  #elif defined(__AVR_ATmega1280__)
    Serial.println("✓ 检测到Arduino Mega 1280");
  #else
    Serial.println("✗ 未检测到支持的Arduino板子");
    return;
  #endif
  
  Serial.println("=== 接线检查 ===");
  Serial.println("请确认以下连接：");
  Serial.println("Arduino Mega 2560    →    433MHz模块");
  Serial.println("引脚16 (TX)         →    RXD");
  Serial.println("引脚17 (RX)         →    TXD");
  Serial.println("GND                 →    GND");
  Serial.println("5V/3.3V             →    VCC");
  Serial.println("================================");
  
  // 初始化Serial2
  Serial2.begin(9600);
  Serial.println("Serial2已初始化，波特率: 9600");
  
  delay(1000);
  
  // 测试1: 检查Serial2是否可用
  Serial.println(">>> 测试1: 检查Serial2状态");
  if (Serial2) {
    Serial.println("✓ Serial2对象可用");
  } else {
    Serial.println("✗ Serial2对象不可用");
  }
  
  // 测试2: 发送单个字节测试
  Serial.println(">>> 测试2: 发送单个字节");
  Serial2.write(0xFD);
  Serial2.flush();
  Serial.println("已发送: 0xFD");
  
  delay(100);
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
  
  delay(1000);
  
  // 测试3: 发送完整数据帧
  Serial.println(">>> 测试3: 发送完整数据帧");
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
  
  // 测试4: 检查引脚状态
  Serial.println(">>> 测试4: 检查引脚状态");
  pinMode(16, OUTPUT);
  pinMode(17, INPUT);
  
  // 测试TX引脚(16)
  digitalWrite(16, HIGH);
  Serial.println("TX引脚(16)设置为HIGH");
  delay(100);
  digitalWrite(16, LOW);
  Serial.println("TX引脚(16)设置为LOW");
  
  // 测试RX引脚(17)
  int rxState = digitalRead(17);
  Serial.print("RX引脚(17)状态: ");
  Serial.println(rxState ? "HIGH" : "LOW");
  
  Serial.println("================================");
  Serial.println("诊断完成！");
  Serial.println("如果所有测试都显示'未收到响应'，请检查：");
  Serial.println("1. 433MHz模块是否正确连接到Arduino");
  Serial.println("2. 模块电源是否正常（LED是否亮）");
  Serial.println("3. 接线是否正确（TX→RXD, RX→TXD）");
  Serial.println("4. 模块是否工作正常");
}

void loop() {
  // 每10秒发送一次测试
  static unsigned long lastTest = 0;
  if (millis() - lastTest >= 10000) {
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
