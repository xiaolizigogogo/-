/*
 * 发送修复配对程序
 * 修复配对模式下的发送问题
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
 * 3. 观察配对过程
 */

#define bt1 31  // 配对按键

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);
  
  pinMode(bt1, INPUT_PULLUP);
  
  Serial.println("=== 发送修复配对程序 ===");
  Serial.println("修复配对模式下的发送问题");
  Serial.println("长按bt1键3秒进入配对模式");
  Serial.println("===============================");
  
  delay(2000);
  
  // 完整初始化模块
  Serial.println(">>> 完整初始化模块...");
  
  // 方法1：发送重置命令
  Serial.println(">>> 方法1：发送重置命令...");
  for (int i = 0; i < 3; i++) {
    byte reset[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    Serial2.write(reset, 8);
    Serial2.flush();
    delay(500);
  }
  
  // 方法2：发送同步信号
  Serial.println(">>> 方法2：发送同步信号...");
  for (int i = 0; i < 5; i++) {
    byte sync[] = {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55};
    Serial2.write(sync, 8);
    Serial2.flush();
    delay(200);
  }
  
  // 方法3：发送配置命令
  Serial.println(">>> 方法3：发送配置命令...");
  byte config[] = {0xFD, 0x01, 0x00, 0x00, 0x00, 0x00, 0xDF};
  Serial2.write(config, 7);
  Serial2.flush();
  delay(1000);
  
  // 方法4：发送配对命令
  Serial.println(">>> 方法4：发送配对命令...");
  byte pair[] = {0xFD, 0x0E, 0x00, 0x00, 0x00, 0x00, 0xDF};
  Serial2.write(pair, 7);
  Serial2.flush();
  delay(1000);
  
  // 方法5：发送测试数据
  Serial.println(">>> 方法5：发送测试数据...");
  byte test[] = {0xFD, 0x03, 0xD5, 0xAA, 0x55, 0x96, 0xDF};
  Serial2.write(test, 7);
  Serial2.flush();
  delay(1000);
  
  Serial.println(">>> 模块完整初始化完成");
}

void loop() {
  static unsigned long bt1_press_time = 0;
  static bool bt1_pressed = false;
  static bool pairingMode = false;
  static unsigned long pairingStartTime = 0;
  static unsigned long lastPairingSendTime = 0;
  static bool isPaired = false;
  static int errorCount = 0;
  static int totalReceived = 0;
  static int validFrames = 0;
  
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
        isPaired = false;
        errorCount = 0;
        totalReceived = 0;
        validFrames = 0;
        
        // 进入配对模式时重新完整初始化模块
        Serial.println(">>> 重新完整初始化模块...");
        
        // 方法1：发送重置命令
        Serial.println(">>> 方法1：发送重置命令...");
        for (int i = 0; i < 3; i++) {
          byte reset[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
          Serial2.write(reset, 8);
          Serial2.flush();
          delay(500);
        }
        
        // 方法2：发送同步信号
        Serial.println(">>> 方法2：发送同步信号...");
        for (int i = 0; i < 5; i++) {
          byte sync[] = {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55};
          Serial2.write(sync, 8);
          Serial2.flush();
          delay(200);
        }
        
        // 方法3：发送配置命令
        Serial.println(">>> 方法3：发送配置命令...");
        byte config[] = {0xFD, 0x01, 0x00, 0x00, 0x00, 0x00, 0xDF};
        Serial2.write(config, 7);
        Serial2.flush();
        delay(1000);
        
        // 方法4：发送配对命令
        Serial.println(">>> 方法4：发送配对命令...");
        byte pair[] = {0xFD, 0x0E, 0x00, 0x00, 0x00, 0x00, 0xDF};
        Serial2.write(pair, 7);
        Serial2.flush();
        delay(1000);
        
        // 方法5：发送测试数据
        Serial.println(">>> 方法5：发送测试数据...");
        byte test[] = {0xFD, 0x03, 0xD5, 0xAA, 0x55, 0x96, 0xDF};
        Serial2.write(test, 7);
        Serial2.flush();
        delay(1000);
        
        Serial.println(">>> 模块重新完整初始化完成");
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
      Serial.print(">>> 最终统计: 总接收");
      Serial.print(totalReceived);
      Serial.print("次, 有效帧");
      Serial.print(validFrames);
      Serial.print("个, 错误");
      Serial.print(errorCount);
      Serial.println("个");
      pairingMode = false;
      return;
    }
    
    // 每5秒发送一次配对请求
    if (currentTime - lastPairingSendTime >= 5000) {
      Serial.println(">>> 配对模式：发送配对请求");
      
      // 清空接收缓冲区
      while (Serial2.available()) {
        Serial2.read();
      }
      
      // 使用成功的发送方式：直接发送字节数组
      Serial.println(">>> 发送配对请求：FD 03 D5 AA 55 96 DF");
      
      byte pairingRequest[] = {0xFD, 0x03, 0xD5, 0xAA, 0x55, 0x96, 0xDF};
      Serial2.write(pairingRequest, 7);
      Serial2.flush();
      
      Serial.println(">>> 配对请求发送完成");
      
      // 等待模块处理延迟
      Serial.println(">>> 等待模块处理延迟...");
      delay(3000); // 等待3秒，让模块处理数据
      
      lastPairingSendTime = currentTime;
    }
    
    // 使用正常工作模式的接收逻辑
    if (Serial2.available()) {
      totalReceived++;
      
      Serial.print(">>> 收到数据 #");
      Serial.print(totalReceived);
      Serial.print(" (时间: ");
      Serial.print(currentTime);
      Serial.print("ms): ");
      
      // 读取所有可用数据
      byte receivedData[100];
      int dataCount = 0;
      while (Serial2.available() && dataCount < 100) {
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
      Serial.println("字节)");
      
      // 检查0xF0错误代码
      for (int i = 0; i < dataCount; i++) {
        if (receivedData[i] == 0xF0) {
          errorCount++;
          Serial.print(">>> 检测到F0错误代码 #");
          Serial.print(errorCount);
          Serial.println(" - 这表示433MHz模块有问题");
        }
      }
      
      // 检查数据帧
      for (int i = 0; i <= dataCount - 7; i++) {
        if (receivedData[i] == 0xFD && receivedData[i + 6] == 0xDF) {
          validFrames++;
          Serial.print(">>> 找到有效数据帧 #");
          Serial.print(validFrames);
          Serial.print(": ");
          for (int j = 0; j < 7; j++) {
            Serial.print("0x");
            if (receivedData[i + j] < 0x10) Serial.print("0");
            Serial.print(receivedData[i + j], HEX);
            Serial.print(" ");
          }
          Serial.println();
          
          // 分析数据帧内容
          if (receivedData[i + 1] == 0x03) {
            Serial.print(">>> 命令: 0x03 (测试数据)");
            if (receivedData[i + 5] == 0x96) {
              Serial.println(" - 配对请求");
              Serial.println(">>> 收到配对请求，发送配对确认");
              // 发送配对确认
              byte pairingConfirm[] = {0xFD, 0x03, 0xD5, 0xAA, 0x55, 0x97, 0xDF};
              Serial2.write(pairingConfirm, 7);
              Serial2.flush();
              Serial.println(">>> 配对确认已发送");
              
              // 等待模块处理延迟
              delay(3000);
            } else if (receivedData[i + 5] == 0x97) {
              Serial.println(" - 配对确认");
              Serial.println(">>> 收到配对确认，配对成功！");
              isPaired = true;
              pairingMode = false;
              Serial.println(">>> 退出配对模式，进入正常工作状态");
              Serial.println(">>> 配对状态已更新为：已配对");
            } else {
              Serial.print(" - 未知数据: 0x");
              Serial.println(receivedData[i + 5], HEX);
            }
          } else {
            Serial.print(">>> 命令: 0x");
            Serial.println(receivedData[i + 1], HEX);
          }
          break;
        }
      }
      
      // 显示统计信息
      Serial.print(">>> 统计: 总接收");
      Serial.print(totalReceived);
      Serial.print("次, 有效帧");
      Serial.print(validFrames);
      Serial.print("个, 错误");
      Serial.print(errorCount);
      Serial.println("个");
    }
  } else {
    // 正常工作模式 - 持续监听接收数据
    static unsigned long lastReceiveCheck = 0;
    if (currentTime - lastReceiveCheck >= 2000) {
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
      if (isPaired) {
        Serial.println(">>> 已配对状态，通信正常");
      } else {
        Serial.println(">>> 未配对状态，长按bt1键3秒进入配对模式");
      }
      lastStatusTime = currentTime;
    }
  }
}
