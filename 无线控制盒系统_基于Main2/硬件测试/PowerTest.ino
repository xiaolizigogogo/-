/*
 * 电源测试程序
 * 用于测试nRF24L01模块的电源供应
 */

void setup() {
  Serial.begin(115200);
  Serial.println("=== nRF24L01 电源测试程序 ===");
  
  // 设置测试引脚
  pinMode(16, OUTPUT); // CE引脚
  pinMode(17, OUTPUT); // CSN引脚
  
  Serial.println("正在测试电源供应...");
  Serial.println("");
  Serial.println("测试步骤：");
  Serial.println("1. 确保nRF24L01模块的VCC连接到Arduino的3.3V");
  Serial.println("2. 确保nRF24L01模块的GND连接到Arduino的GND");
  Serial.println("3. 用万用表测量nRF24L01模块的VCC引脚电压");
  Serial.println("4. 电压应该在3.2V-3.4V之间");
  Serial.println("");
  Serial.println("如果电压不正确，请检查：");
  Serial.println("- Arduino的3.3V输出是否正常");
  Serial.println("- 连接线是否良好");
  Serial.println("- nRF24L01模块是否损坏");
  Serial.println("");
  Serial.println("开始引脚测试...");
}

void loop() {
  static unsigned long lastTest = 0;
  if (millis() - lastTest > 2000) {
    lastTest = millis();
    
    // 测试CE引脚
    digitalWrite(16, HIGH);
    Serial.println("CE引脚设置为HIGH");
    delay(100);
    digitalWrite(16, LOW);
    Serial.println("CE引脚设置为LOW");
    
    // 测试CSN引脚
    digitalWrite(17, HIGH);
    Serial.println("CSN引脚设置为HIGH");
    delay(100);
    digitalWrite(17, LOW);
    Serial.println("CSN引脚设置为LOW");
    
    Serial.println("---");
  }
}
