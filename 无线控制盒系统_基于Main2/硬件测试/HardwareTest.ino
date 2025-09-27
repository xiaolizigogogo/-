/*
 * nRF24L01 硬件测试程序
 * 用于诊断硬件连接问题
 */

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// nRF24L01配置 - 尝试不同的引脚组合
RF24 radio(16, 17); // CE=16, CSN=17

void setup() {
  Serial.begin(115200);
  Serial.println("=== nRF24L01 硬件测试程序 ===");
  Serial.println("正在测试硬件连接...");
  
  // 测试SPI连接
  Serial.println("1. 测试SPI连接...");
  SPI.begin();
  Serial.println("   SPI初始化完成");
  
  // 测试nRF24L01初始化
  Serial.println("2. 测试nRF24L01初始化...");
  bool radioResult = radio.begin();
  
  if (radioResult) {
    Serial.println("   ✓ nRF24L01初始化成功！");
    
    // 测试基本配置
    Serial.println("3. 测试基本配置...");
    radio.setChannel(76);
    radio.setDataRate(RF24_1MBPS);
    radio.setPALevel(RF24_PA_HIGH);
    radio.setRetries(15, 15);
    Serial.println("   ✓ 基本配置完成");
    
    // 测试地址设置
    Serial.println("4. 测试地址设置...");
    const byte testAddress[] = "00001";
    radio.openWritingPipe(testAddress);
    radio.openReadingPipe(1, testAddress);
    Serial.println("   ✓ 地址设置完成");
    
    // 测试发送模式
    Serial.println("5. 测试发送模式...");
    radio.stopListening();
    Serial.println("   ✓ 发送模式设置完成");
    
    // 测试接收模式
    Serial.println("6. 测试接收模式...");
    radio.startListening();
    Serial.println("   ✓ 接收模式设置完成");
    
    // 显示配置信息
    Serial.println("7. 配置信息:");
    Serial.print("   频道: ");
    Serial.println(radio.getChannel());
    Serial.print("   数据率: ");
    Serial.println(radio.getDataRate());
    Serial.print("   功率级别: ");
    Serial.println(radio.getPALevel());
    
    // 测试数据发送
    Serial.println("8. 测试数据发送...");
    radio.stopListening();
    const char testMessage[] = "Hello World";
    bool sendResult = radio.write(testMessage, strlen(testMessage));
    
    if (sendResult) {
      Serial.println("   ✓ 数据发送成功！");
    } else {
      Serial.println("   ✗ 数据发送失败");
    }
    
    Serial.println("=== 硬件测试完成 ===");
    Serial.println("如果所有测试都通过，说明硬件连接正常");
    
  } else {
    Serial.println("   ✗ nRF24L01初始化失败！");
    Serial.println("=== 硬件连接问题排查 ===");
    Serial.println("请检查以下连接：");
    Serial.println("1. VCC → 3.3V (不是5V！)");
    Serial.println("2. GND → GND");
    Serial.println("3. CE → 数字引脚16");
    Serial.println("4. CSN → 数字引脚17");
    Serial.println("5. SCK → 数字引脚13");
    Serial.println("6. MOSI → 数字引脚11");
    Serial.println("7. MISO → 数字引脚12");
    Serial.println("");
    Serial.println("常见问题：");
    Serial.println("- 电源电压错误（必须3.3V）");
    Serial.println("- 引脚连接错误");
    Serial.println("- 模块损坏");
    Serial.println("- 电源电流不足");
  }
}

void loop() {
  // 持续测试数据发送
  static unsigned long lastTest = 0;
  if (millis() - lastTest > 2000) {
    lastTest = millis();
    
    if (radio.begin()) {
      radio.stopListening();
      const char testMessage[] = "Test";
      bool result = radio.write(testMessage, strlen(testMessage));
      
      if (result) {
        Serial.println("持续测试：发送成功");
      } else {
        Serial.println("持续测试：发送失败");
      }
    } else {
      Serial.println("持续测试：模块初始化失败");
    }
  }
}
