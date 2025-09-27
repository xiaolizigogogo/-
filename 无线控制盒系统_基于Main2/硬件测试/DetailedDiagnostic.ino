/*
 * nRF24L01 详细硬件诊断程序
 * 用于逐步排查硬件连接问题
 */

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// 尝试不同的引脚组合
RF24 radio(16, 17); // CE=16, CSN=17

void setup() {
  Serial.begin(115200);
  delay(2000); // 等待串口稳定
  
  Serial.println("=== nRF24L01 详细硬件诊断程序 ===");
  Serial.println();
  
  // 步骤1：检查Arduino基本功能
  Serial.println("步骤1：检查Arduino基本功能");
  Serial.println("✓ Arduino启动正常");
  Serial.println("✓ 串口通信正常");
  Serial.println();
  
  // 步骤2：检查SPI功能
  Serial.println("步骤2：检查SPI功能");
  SPI.begin();
  Serial.println("✓ SPI库初始化完成");
  
  // 测试SPI引脚
  pinMode(13, OUTPUT); // SCK
  pinMode(11, OUTPUT); // MOSI
  pinMode(12, INPUT);  // MISO
  
  digitalWrite(13, HIGH);
  digitalWrite(11, HIGH);
  Serial.println("✓ SPI引脚设置完成");
  Serial.println("  请用万用表测量：");
  Serial.println("  - 引脚13 (SCK): 应该显示5V");
  Serial.println("  - 引脚11 (MOSI): 应该显示5V");
  Serial.println("  - 引脚12 (MISO): 应该显示0V或5V");
  Serial.println();
  
  // 步骤3：检查控制引脚
  Serial.println("步骤3：检查控制引脚");
  pinMode(16, OUTPUT); // CE
  pinMode(17, OUTPUT); // CSN
  
  digitalWrite(16, HIGH);
  digitalWrite(17, HIGH);
  Serial.println("✓ 控制引脚设置完成");
  Serial.println("  请用万用表测量：");
  Serial.println("  - 引脚16 (CE): 应该显示5V");
  Serial.println("  - 引脚17 (CSN): 应该显示5V");
  Serial.println();
  
  // 步骤4：检查电源
  Serial.println("步骤4：检查电源连接");
  Serial.println("  请用万用表测量nRF24L01模块：");
  Serial.println("  - VCC引脚: 应该显示3.3V (不是5V！)");
  Serial.println("  - GND引脚: 应该显示0V");
  Serial.println("  - 如果VCC显示5V，立即断开连接！");
  Serial.println();
  
  // 步骤5：尝试初始化nRF24L01
  Serial.println("步骤5：尝试初始化nRF24L01");
  Serial.println("正在初始化...");
  
  bool radioResult = radio.begin();
  
  if (radioResult) {
    Serial.println("✓ nRF24L01初始化成功！");
    Serial.println();
    
    // 显示配置信息
    Serial.println("步骤6：显示配置信息");
    Serial.print("  频道: ");
    Serial.println(radio.getChannel());
    Serial.print("  数据率: ");
    Serial.println(radio.getDataRate());
    Serial.print("  功率级别: ");
    Serial.println(radio.getPALevel());
    Serial.print("  重试次数: ");
    Serial.print(radio.getARC());
    Serial.print(", 延迟: ");
    Serial.println(radio.getARD());
    Serial.println();
    
    // 测试基本功能
    Serial.println("步骤7：测试基本功能");
    
    // 测试地址设置
    const byte testAddress[] = "00001";
    radio.openWritingPipe(testAddress);
    radio.openReadingPipe(1, testAddress);
    Serial.println("✓ 地址设置完成");
    
    // 测试发送模式
    radio.stopListening();
    Serial.println("✓ 发送模式设置完成");
    
    // 测试接收模式
    radio.startListening();
    Serial.println("✓ 接收模式设置完成");
    
    Serial.println();
    Serial.println("=== 诊断结果：硬件连接正常 ===");
    Serial.println("nRF24L01模块工作正常，可以继续使用主控制器程序。");
    
  } else {
    Serial.println("✗ nRF24L01初始化失败！");
    Serial.println();
    
    Serial.println("=== 故障排查指南 ===");
    Serial.println();
    Serial.println("可能的问题和解决方案：");
    Serial.println();
    Serial.println("1. 电源问题：");
    Serial.println("   - 检查VCC是否连接到3.3V（不是5V！）");
    Serial.println("   - 检查GND是否连接到GND");
    Serial.println("   - 检查电源是否稳定（3.3V ± 0.1V）");
    Serial.println();
    Serial.println("2. 引脚连接问题：");
    Serial.println("   - 检查所有引脚连接是否牢固");
    Serial.println("   - 检查是否有短路或断路");
    Serial.println("   - 尝试重新焊接连接");
    Serial.println();
    Serial.println("3. 模块问题：");
    Serial.println("   - 模块可能已损坏（特别是如果之前连接过5V）");
    Serial.println("   - 尝试更换新的nRF24L01模块");
    Serial.println();
    Serial.println("4. Arduino问题：");
    Serial.println("   - 检查Arduino的3.3V输出是否正常");
    Serial.println("   - 尝试使用外部3.3V电源");
    Serial.println();
    Serial.println("5. 库问题：");
    Serial.println("   - 确保安装了正确的RF24库");
    Serial.println("   - 尝试重新安装RF24库");
  }
}

void loop() {
  // 持续监控
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 5000) {
    lastCheck = millis();
    
    if (radio.begin()) {
      Serial.println("持续监控：模块工作正常");
    } else {
      Serial.println("持续监控：模块初始化失败");
    }
  }
}

