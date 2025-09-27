/*
 * 缓冲区接收测试程序
 * 使用缓冲区来组合完整的数据帧
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

#define BUFFER_SIZE 100

byte receiveBuffer[BUFFER_SIZE];
int bufferIndex = 0;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);
  
  Serial.println("=== 缓冲区接收测试程序 ===");
  Serial.println("使用缓冲区来组合完整的数据帧");
  Serial.println("===============================");
  
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
    // 读取所有可用数据到缓冲区
    while (Serial2.available() && bufferIndex < BUFFER_SIZE) {
      receiveBuffer[bufferIndex] = Serial2.read();
      bufferIndex++;
    }
    
    // 检查缓冲区中是否有完整的数据帧
    for (int i = 0; i <= bufferIndex - 7; i++) {
      if (receiveBuffer[i] == 0xFD && receiveBuffer[i + 6] == 0xDF) {
        // 找到完整的数据帧
        totalReceived++;
        validFrames++;
        
        Serial.print(">>> 收到完整数据帧 #");
        Serial.print(validFrames);
        Serial.print(" (时间: ");
        Serial.print(currentTime);
        Serial.print("ms): ");
        
        // 显示完整的数据帧
        for (int j = 0; j < 7; j++) {
          Serial.print("0x");
          if (receiveBuffer[i + j] < 0x10) Serial.print("0");
          Serial.print(receiveBuffer[i + j], HEX);
          Serial.print(" ");
        }
        Serial.println();
        
        // 分析数据帧内容
        if (receiveBuffer[i + 1] == 0x03) {
          Serial.print(">>> 命令: 0x03 (测试数据)");
          if (receiveBuffer[i + 5] == 0x96) {
            Serial.println(" - 配对请求");
          } else if (receiveBuffer[i + 5] == 0x97) {
            Serial.println(" - 配对确认");
          } else {
            Serial.print(" - 未知数据: 0x");
            Serial.println(receiveBuffer[i + 5], HEX);
          }
        } else {
          Serial.print(">>> 命令: 0x");
          Serial.println(receiveBuffer[i + 1], HEX);
        }
        
        // 移除已处理的数据帧
        for (int k = i + 7; k < bufferIndex; k++) {
          receiveBuffer[k - 7] = receiveBuffer[k];
        }
        bufferIndex -= 7;
        i = -1; // 重新开始检查
      }
    }
    
    // 检查0xF0错误代码
    for (int i = 0; i < bufferIndex; i++) {
      if (receiveBuffer[i] == 0xF0) {
        errorCount++;
        Serial.print(">>> 检测到F0错误代码 #");
        Serial.print(errorCount);
        Serial.println(" - 这表示433MHz模块有问题");
        
        // 移除F0错误代码
        for (int k = i + 1; k < bufferIndex; k++) {
          receiveBuffer[k - 1] = receiveBuffer[k];
        }
        bufferIndex--;
        i = -1; // 重新开始检查
      }
    }
    
    // 如果缓冲区满了，清空一半
    if (bufferIndex >= BUFFER_SIZE - 10) {
      Serial.println(">>> 缓冲区接近满，清空一半");
      for (int i = 0; i < bufferIndex / 2; i++) {
        receiveBuffer[i] = receiveBuffer[i + bufferIndex / 2];
      }
      bufferIndex = bufferIndex / 2;
    }
    
    // 显示统计信息
    Serial.print(">>> 统计: 总接收");
    Serial.print(totalReceived);
    Serial.print("次, 有效帧");
    Serial.print(validFrames);
    Serial.print("个, 错误");
    Serial.print(errorCount);
    Serial.print("个, 缓冲区");
    Serial.print(bufferIndex);
    Serial.println("字节");
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
    Serial.print("个, 缓冲区");
    Serial.print(bufferIndex);
    Serial.println("字节");
    lastStatusTime = currentTime;
  }
}
