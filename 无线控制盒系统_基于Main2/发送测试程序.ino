/*
 * 发送测试程序
 * 专门测试配对模式下的发送功能
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
 * 2. 长按bt1键3秒进入配对模式
 * 3. 观察发送过程
 */

#define bt1 31  // 配对按键

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);
  
  pinMode(bt1, INPUT_PULLUP);
  
  Serial.println("=== 发送测试程序 ===");
  Serial.println("专门测试配对模式下的发送功能");
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
      Serial.print(">>> 总共发送了");
      Serial.print(sendCount);
      Serial.println("次配对请求");
      pairingMode = false;
      return;
    }
    
    // 每10秒发送一次配对请求
    if (currentTime - lastPairingSendTime >= 10000) {
      sendCount++;
      Serial.print(">>> 配对模式：发送配对请求 #");
      Serial.println(sendCount);
      
      // 使用与初始化相同的发送方式：字节数组发送
      Serial.println(">>> 发送配对请求：FD 03 D5 AA 55 96 DF");
      
      byte pairingRequest[] = {0xFD, 0x03, 0xD5, 0xAA, 0x55, 0x96, 0xDF};
      Serial2.write(pairingRequest, 7);  // 使用字节数组发送
      Serial2.flush();
      delay(1000);  // 使用与初始化相同的延迟
      
      Serial.println(">>> 配对请求发送完成");
      
      lastPairingSendTime = currentTime;
    }
    
    // 简单的接收监听
    if (Serial2.available()) {
      Serial.print(">>> 配对模式收到数据: ");
      while (Serial2.available()) {
        byte data = Serial2.read();
        Serial.print("0x");
        if (data < 0x10) Serial.print("0");
        Serial.print(data, HEX);
        Serial.print(" ");
      }
      Serial.println();
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
      Serial.println(">>> 未配对状态，长按bt1键3秒进入配对模式");
      lastStatusTime = currentTime;
    }
  }
}
