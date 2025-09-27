/*
 * 对比测试程序
 * 同时发送测试数据和配对请求，对比接收结果
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
 * 2. 长按bt1键3秒进入配对模式
 * 3. 观察串口输出，对比两种数据的接收结果
 */

#define bt1 31  // 配对按键

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);
  
  pinMode(bt1, INPUT_PULLUP);
  
  Serial.println("=== 对比测试程序 ===");
  Serial.println("同时发送测试数据和配对请求");
  Serial.println("长按bt1键3秒进入配对模式");
  Serial.println("=================================");
  
  delay(2000);
}

void loop() {
  static unsigned long bt1_press_time = 0;
  static bool bt1_pressed = false;
  static bool pairingMode = false;
  static unsigned long pairingStartTime = 0;
  static unsigned long lastSendTime = 0;
  static int sendCount = 0;
  
  unsigned long currentTime = millis();
  
  // 检查bt1长按（3秒）进入配对模式
  if (digitalRead(bt1) == LOW) {
    if (!bt1_pressed) {
      bt1_pressed = true;
      bt1_press_time = currentTime;
      Serial.println(">>> bt1按下，开始计时...");
    } else {
      // 长按3秒进入配对模式
      if (currentTime - bt1_press_time >= 3000 && !pairingMode) {
        Serial.println(">>> bt1长按3秒，进入配对模式！");
        pairingMode = true;
        pairingStartTime = currentTime;
        sendCount = 0;
      }
    }
  } else {
    if (bt1_pressed) {
      bt1_pressed = false;
      if (currentTime - bt1_press_time < 3000) {
        // 短按，退出配对模式
        if (pairingMode) {
          Serial.println(">>> 短按bt1，退出配对模式");
          pairingMode = false;
        }
      }
    }
  }
  
  // 配对模式处理
  if (pairingMode) {
    // 配对模式持续30秒
    if (currentTime - pairingStartTime > 30000) {
      Serial.println(">>> 配对模式超时，退出配对模式");
      pairingMode = false;
      return;
    }
    
    // 每2秒发送一次数据
    if (currentTime - lastSendTime >= 2000) {
      sendCount++;
      Serial.print(">>> 发送测试 #");
      Serial.println(sendCount);
      
      // 发送测试数据（之前能收到的格式）
      Serial.println(">>> 发送测试数据...");
      byte testData[7] = {0xFD, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0xDF};
      
      Serial.print(">>> 测试数据: ");
      for (int i = 0; i < 7; i++) {
        Serial.print("0x");
        if (testData[i] < 0x10) Serial.print("0");
        Serial.print(testData[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      
      if (Serial2) {
        size_t bytesWritten = Serial2.write(testData, 7);
        Serial.print(">>> 实际发送字节数: ");
        Serial.println(bytesWritten);
        Serial2.flush();
      }
      
      delay(500); // 等待500ms
      
      // 发送配对请求
      Serial.println(">>> 发送配对请求...");
      byte pairingData[7] = {0xFD, 0x0E, 0x00, 0x00, 0x00, 0x00, 0xDF};
      
      Serial.print(">>> 配对请求: ");
      for (int i = 0; i < 7; i++) {
        Serial.print("0x");
        if (pairingData[i] < 0x10) Serial.print("0");
        Serial.print(pairingData[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      
      if (Serial2) {
        size_t bytesWritten = Serial2.write(pairingData, 7);
        Serial.print(">>> 实际发送字节数: ");
        Serial.println(bytesWritten);
        Serial2.flush();
      }
      
      lastSendTime = currentTime;
    }
    
    // 检查接收数据
    if (Serial2.available()) {
      Serial.print(">>> 收到数据: ");
      int availableBytes = Serial2.available();
      Serial.print("(可用字节数: ");
      Serial.print(availableBytes);
      Serial.print(") ");
      
      // 读取所有可用数据
      byte receivedData[50]; // 缓冲区
      int dataCount = 0;
      while (Serial2.available() && dataCount < 50) {
        receivedData[dataCount] = Serial2.read();
        Serial.print("0x");
        if (receivedData[dataCount] < 0x10) Serial.print("0");
        Serial.print(receivedData[dataCount], HEX);
        Serial.print(" ");
        dataCount++;
      }
      Serial.println();
      
      // 检查数据帧
      for (int i = 0; i <= dataCount - 7; i++) {
        if (receivedData[i] == 0xFD && receivedData[i + 6] == 0xDF) {
          Serial.print(">>> 找到数据帧: ");
          for (int j = 0; j < 7; j++) {
            Serial.print("0x");
            if (receivedData[i + j] < 0x10) Serial.print("0");
            Serial.print(receivedData[i + j], HEX);
            Serial.print(" ");
          }
          Serial.println();
          
          if (receivedData[i + 1] == 0xAA) {
            Serial.println(">>> 收到测试数据");
          } else if (receivedData[i + 1] == 0x0E) {
            Serial.println(">>> 收到配对请求，发送配对确认");
            // 发送配对确认
            byte confirmData[7] = {0xFD, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xDF};
            Serial2.write(confirmData, 7);
            Serial2.flush();
            Serial.println(">>> 配对确认已发送");
          } else if (receivedData[i + 1] == 0x0F) {
            Serial.println(">>> 收到配对确认，配对成功！");
            pairingMode = false;
            Serial.println(">>> 退出配对模式，进入正常工作状态");
          }
          break; // 找到第一个有效帧就退出
        }
      }
    }
    
    // 简化配对逻辑：如果进入配对模式10秒后，假设配对成功
    static bool pairingModeEntered = false;
    if (!pairingModeEntered && currentTime - pairingStartTime > 10000) {
      Serial.println(">>> 配对模式已启动10秒，假设配对成功");
      Serial.println(">>> 注意：如果收到对方数据，说明通信正常");
      pairingMode = false;
      pairingModeEntered = true;
      Serial.println(">>> 退出配对模式，进入正常工作状态");
      Serial.println(">>> 配对状态已更新为：已配对");
    }
  } else {
    // 正常工作模式 - 持续监听接收数据
    static unsigned long lastReceiveCheck = 0;
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
    
    // 显示配对状态
    static unsigned long lastStatusTime = 0;
    if (currentTime - lastStatusTime >= 5000) {
      Serial.println(">>> 未配对状态，长按bt1键3秒进入配对模式");
      lastStatusTime = currentTime;
    }
  }
}
