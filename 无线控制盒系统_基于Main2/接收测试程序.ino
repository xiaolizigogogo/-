/*
 * 接收测试程序
 * 专门测试433MHz模块的接收功能
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
 * 2. 观察接收数据
 */

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);
  
  Serial.println("=== 接收测试程序 ===");
  Serial.println("专门测试433MHz模块的接收功能");
  Serial.println("=========================");
  
  delay(2000);
  
  // 清空接收缓冲区
  while (Serial2.available()) {
    Serial2.read();
  }
  
  Serial.println(">>> 接收缓冲区已清空，开始监听...");
}

void loop() {
  static unsigned long lastReceiveTime = 0;
  static int totalReceived = 0;
  static int validFrames = 0;
  static int errorCount = 0;
  
  unsigned long currentTime = millis();
  
  // 检查是否有数据到达
  if (Serial2.available()) {
    totalReceived++;
    lastReceiveTime = currentTime;
    
    Serial.print(">>> 收到数据 #");
    Serial.print(totalReceived);
    Serial.print(" (时间: ");
    Serial.print(currentTime);
    Serial.print("ms): ");
    
    // 等待更多数据到达
    delay(100); // 等待100ms
    
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
          } else if (receivedData[i + 5] == 0x97) {
            Serial.println(" - 配对确认");
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
  
  // 每10秒显示一次状态
  static unsigned long lastStatusTime = 0;
  if (currentTime - lastStatusTime >= 10000) {
    Serial.print(">>> 状态: 已收到");
    Serial.print(totalReceived);
    Serial.print("次数据, 有效帧");
    Serial.print(validFrames);
    Serial.print("个, 错误");
    Serial.print(errorCount);
    Serial.println("个");
    lastStatusTime = currentTime;
  }
}
