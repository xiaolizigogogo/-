/*
 * 电源供应测试程序
 * 专门测试3.3V电源供应是否正常
 */

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("=== 3.3V电源供应测试程序 ===");
  Serial.println();
  
  Serial.println("请按照以下步骤进行测试：");
  Serial.println();
  
  Serial.println("步骤1：测试Arduino的3.3V输出");
  Serial.println("  1. 将万用表设置为直流电压档");
  Serial.println("  2. 红表笔连接到Arduino的3.3V引脚");
  Serial.println("  3. 黑表笔连接到Arduino的GND引脚");
  Serial.println("  4. 读取电压值");
  Serial.println("  期望值：3.3V ± 0.1V");
  Serial.println();
  
  Serial.println("步骤2：测试nRF24L01模块的电源");
  Serial.println("  1. 确保nRF24L01模块已连接");
  Serial.println("  2. 红表笔连接到nRF24L01的VCC引脚");
  Serial.println("  3. 黑表笔连接到nRF24L01的GND引脚");
  Serial.println("  4. 读取电压值");
  Serial.println("  期望值：3.3V ± 0.1V");
  Serial.println("  警告：如果显示5V，立即断开连接！");
  Serial.println();
  
  Serial.println("步骤3：测试电源稳定性");
  Serial.println("  1. 保持万用表连接");
  Serial.println("  2. 观察电压是否稳定");
  Serial.println("  3. 电压波动应该在±0.05V以内");
  Serial.println();
  
  Serial.println("步骤4：测试电源电流能力");
  Serial.println("  1. 将万用表设置为直流电流档");
  Serial.println("  2. 断开nRF24L01的VCC连接");
  Serial.println("  3. 将万用表串联在VCC连接中");
  Serial.println("  4. 重新连接nRF24L01");
  Serial.println("  5. 读取电流值");
  Serial.println("  期望值：10-20mA（正常工作电流）");
  Serial.println();
  
  Serial.println("=== 常见问题及解决方案 ===");
  Serial.println();
  Serial.println("问题1：3.3V电压为0V");
  Serial.println("  原因：Arduino的3.3V稳压器损坏");
  Serial.println("  解决：使用外部3.3V电源或更换Arduino");
  Serial.println();
  
  Serial.println("问题2：3.3V电压为5V");
  Serial.println("  原因：连接错误，连接到了5V引脚");
  Serial.println("  解决：检查连接，确保连接到3.3V引脚");
  Serial.println();
  
  Serial.println("问题3：电压不稳定，波动很大");
  Serial.println("  原因：电源负载过重或连接不良");
  Serial.println("  解决：检查连接，使用外部电源");
  Serial.println();
  
  Serial.println("问题4：电流过大（>50mA）");
  Serial.println("  原因：nRF24L01模块可能损坏");
  Serial.println("  解决：更换nRF24L01模块");
  Serial.println();
  
  Serial.println("=== 外部电源解决方案 ===");
  Serial.println();
  Serial.println("如果Arduino的3.3V输出有问题，可以使用外部电源：");
  Serial.println("1. 使用3.3V稳压器（如AMS1117-3.3）");
  Serial.println("2. 使用3.3V电池组");
  Serial.println("3. 使用USB转3.3V模块");
  Serial.println();
  Serial.println("连接方法：");
  Serial.println("- 外部3.3V电源的正极 → nRF24L01的VCC");
  Serial.println("- 外部3.3V电源的负极 → nRF24L01的GND");
  Serial.println("- 外部3.3V电源的负极 → Arduino的GND（共地）");
}

void loop() {
  // 持续显示测试状态
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 5000) {
    lastUpdate = millis();
    Serial.println("电源测试程序运行中...");
    Serial.println("请按照上述步骤进行测试");
    Serial.println();
  }
}

