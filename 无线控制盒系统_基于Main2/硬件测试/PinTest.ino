/*
 * Arduino引脚测试程序
 * 用于测试nRF24L01相关引脚是否正常工作
 */

#define CE_PIN 16
#define CSN_PIN 17
#define SCK_PIN 13
#define MOSI_PIN 11
#define MISO_PIN 12

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("=== Arduino引脚测试程序 ===");
  Serial.println();
  
  // 设置引脚模式
  pinMode(CE_PIN, OUTPUT);
  pinMode(CSN_PIN, OUTPUT);
  pinMode(SCK_PIN, OUTPUT);
  pinMode(MOSI_PIN, OUTPUT);
  pinMode(MISO_PIN, INPUT); // MISO是输入引脚
  
  Serial.println("引脚设置完成，开始测试...");
  Serial.println();
}

void loop() {
  // 测试HIGH状态
  Serial.println("--- HIGH状态测试 ---");
  digitalWrite(CE_PIN, HIGH);
  digitalWrite(CSN_PIN, HIGH);
  digitalWrite(SCK_PIN, HIGH);
  digitalWrite(MOSI_PIN, HIGH);
  
  Serial.print("CE (Pin "); Serial.print(CE_PIN); Serial.println("): 设置为HIGH");
  Serial.print("CSN (Pin "); Serial.print(CSN_PIN); Serial.println("): 设置为HIGH");
  Serial.print("SCK (Pin "); Serial.print(SCK_PIN); Serial.println("): 设置为HIGH");
  Serial.print("MOSI (Pin "); Serial.print(MOSI_PIN); Serial.println("): 设置为HIGH");
  Serial.print("MISO (Pin "); Serial.print(MISO_PIN); Serial.print("): 读取值 = ");
  Serial.println(digitalRead(MISO_PIN));
  
  Serial.println("请用万用表测量各引脚电压，应该显示5V");
  Serial.println();
  
  delay(3000);
  
  // 测试LOW状态
  Serial.println("--- LOW状态测试 ---");
  digitalWrite(CE_PIN, LOW);
  digitalWrite(CSN_PIN, LOW);
  digitalWrite(SCK_PIN, LOW);
  digitalWrite(MOSI_PIN, LOW);
  
  Serial.print("CE (Pin "); Serial.print(CE_PIN); Serial.println("): 设置为LOW");
  Serial.print("CSN (Pin "); Serial.print(CSN_PIN); Serial.println("): 设置为LOW");
  Serial.print("SCK (Pin "); Serial.print(SCK_PIN); Serial.println("): 设置为LOW");
  Serial.print("MOSI (Pin "); Serial.print(MOSI_PIN); Serial.println("): 设置为LOW");
  Serial.print("MISO (Pin "); Serial.print(MISO_PIN); Serial.print("): 读取值 = ");
  Serial.println(digitalRead(MISO_PIN));
  
  Serial.println("请用万用表测量各引脚电压，应该显示0V");
  Serial.println();
  
  delay(3000);
}