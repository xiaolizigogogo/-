/*
 * 引脚电压测试程序
 * 用于测试所有相关引脚的电压输出
 */

#define CE_PIN 16
#define CSN_PIN 17
#define SCK_PIN 13
#define MOSI_PIN 11
#define MISO_PIN 12

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("=== 引脚电压测试程序 ===");
  Serial.println();
  
  // 设置引脚模式
  pinMode(CE_PIN, OUTPUT);
  pinMode(CSN_PIN, OUTPUT);
  pinMode(SCK_PIN, OUTPUT);
  pinMode(MOSI_PIN, OUTPUT);
  pinMode(MISO_PIN, INPUT); // MISO是输入引脚
  
  Serial.println("引脚设置完成，请用万用表测试以下引脚电压：");
  Serial.println();
  
  // 测试HIGH状态
  Serial.println("--- HIGH状态测试 ---");
  digitalWrite(CE_PIN, HIGH);
  digitalWrite(CSN_PIN, HIGH);
  digitalWrite(SCK_PIN, HIGH);
  digitalWrite(MOSI_PIN, HIGH);
  
  Serial.print("CE (Pin "); Serial.print(CE_PIN); Serial.println("): 应该显示5V");
  Serial.print("CSN (Pin "); Serial.print(CSN_PIN); Serial.println("): 应该显示5V");
  Serial.print("SCK (Pin "); Serial.print(SCK_PIN); Serial.println("): 应该显示5V");
  Serial.print("MOSI (Pin "); Serial.print(MOSI_PIN); Serial.println("): 应该显示5V");
  Serial.print("MISO (Pin "); Serial.print(MISO_PIN); Serial.println("): 应该显示0V或5V");
  Serial.println();
  
  delay(3000);
  
  // 测试LOW状态
  Serial.println("--- LOW状态测试 ---");
  digitalWrite(CE_PIN, LOW);
  digitalWrite(CSN_PIN, LOW);
  digitalWrite(SCK_PIN, LOW);
  digitalWrite(MOSI_PIN, LOW);
  
  Serial.print("CE (Pin "); Serial.print(CE_PIN); Serial.println("): 应该显示0V");
  Serial.print("CSN (Pin "); Serial.print(CSN_PIN); Serial.println("): 应该显示0V");
  Serial.print("SCK (Pin "); Serial.print(SCK_PIN); Serial.println("): 应该显示0V");
  Serial.print("MOSI (Pin "); Serial.print(MOSI_PIN); Serial.println("): 应该显示0V");
  Serial.println();
  
  delay(3000);
  
  // 测试3.3V电源
  Serial.println("--- 3.3V电源测试 ---");
  Serial.println("请测量Arduino的3.3V引脚：");
  Serial.println("- 3.3V引脚: 应该显示3.3V ± 0.1V");
  Serial.println("- 如果电压不正确，Arduino可能有电源问题");
  Serial.println();
  
  // 测试GND
  Serial.println("--- GND测试 ---");
  Serial.println("请测量Arduino的GND引脚：");
  Serial.println("- GND引脚: 应该显示0V");
  Serial.println();
  
  Serial.println("=== 测试完成 ===");
  Serial.println("如果所有电压都正确，但nRF24L01仍然无法初始化，");
  Serial.println("可能是模块本身的问题，建议更换模块。");
}

void loop() {
  // 持续切换引脚状态，方便测试
  static unsigned long lastToggle = 0;
  static bool state = false;
  
  if (millis() - lastToggle > 2000) {
    lastToggle = millis();
    state = !state;
    
    digitalWrite(CE_PIN, state);
    digitalWrite(CSN_PIN, state);
    digitalWrite(SCK_PIN, state);
    digitalWrite(MOSI_PIN, state);
    
    Serial.print("引脚状态切换为: ");
    Serial.println(state ? "HIGH" : "LOW");
  }
}

