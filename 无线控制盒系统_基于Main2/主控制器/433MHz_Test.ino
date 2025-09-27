/*
 * 433MHz模块测试代码
 * 用于诊断主控制器的硬件连接问题
 */

#include <SoftwareSerial.h>

// 使用与主控制器相同的引脚定义
SoftwareSerial wirelessSerial(28, 29); // RX=28, TX=29

void setup() {
  Serial.begin(115200);
  wirelessSerial.begin(9600);
  
  Serial.println("=== 433MHz模块测试 ===");
  Serial.println("RX=28, TX=29");
  Serial.println("波特率=9600");
  Serial.println("等待数据...");
  Serial.println("=====================");
}

void loop() {
  // 检查是否有数据
  if (wirelessSerial.available() > 0) {
    Serial.print("收到数据: ");
    while (wirelessSerial.available()) {
      uint8_t data = wirelessSerial.read();
      Serial.print("0x");
      if (data < 0x10) Serial.print("0");
      Serial.print(data, HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
  
  // 每5秒输出一次状态
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus >= 5000) {
    Serial.print("可用数据: ");
    Serial.println(wirelessSerial.available());
    lastStatus = millis();
  }
  
  delay(100);
}
