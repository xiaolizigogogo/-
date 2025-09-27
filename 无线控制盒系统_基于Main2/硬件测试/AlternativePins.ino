/*
 * 尝试不同引脚组合的测试程序
 * 如果标准引脚不工作，可以尝试其他引脚
 */

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// 尝试不同的引脚组合
// 组合1：标准引脚
// RF24 radio(16, 17); // CE=16, CSN=17

// 组合2：备用引脚
RF24 radio(8, 9); // CE=8, CSN=9

// 组合3：其他备用引脚
// RF24 radio(6, 7); // CE=6, CSN=7

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("=== 不同引脚组合测试程序 ===");
  Serial.println();
  
  Serial.println("当前测试引脚组合：");
  Serial.println("CE = 8, CSN = 9");
  Serial.println("SPI引脚保持不变：SCK=13, MOSI=11, MISO=12");
  Serial.println();
  
  // 测试初始化
  Serial.println("正在测试初始化...");
  bool result = radio.begin();
  
  if (result) {
    Serial.println("✓ 初始化成功！");
    Serial.println("请按照以下方式重新连接：");
    Serial.println("CE → 数字引脚8");
    Serial.println("CSN → 数字引脚9");
    Serial.println("其他引脚保持不变");
    
    // 测试基本功能
    radio.setChannel(76);
    radio.setDataRate(RF24_1MBPS);
    radio.setPALevel(RF24_PA_HIGH);
    
    const byte testAddress[] = "00001";
    radio.openWritingPipe(testAddress);
    radio.openReadingPipe(1, testAddress);
    
    Serial.println("✓ 基本功能测试通过");
    
  } else {
    Serial.println("✗ 初始化失败");
    Serial.println("建议：");
    Serial.println("1. 更换nRF24L01模块");
    Serial.println("2. 检查模块是否损坏");
    Serial.println("3. 尝试其他引脚组合");
  }
}

void loop() {
  // 持续测试
  static unsigned long lastTest = 0;
  if (millis() - lastTest > 3000) {
    lastTest = millis();
    
    if (radio.begin()) {
      Serial.println("持续测试：模块工作正常");
    } else {
      Serial.println("持续测试：模块初始化失败");
    }
  }
}





