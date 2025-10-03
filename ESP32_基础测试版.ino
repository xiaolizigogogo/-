/*
 * ESP32基础测试版
 * 功能：测试ESP32基本功能，不使用BLE
 * 特点：验证硬件和基本库是否正常
 */

#include <WiFi.h>

void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(2000);
  
  Serial.println();
  Serial.println("==========================================");
  Serial.println("ESP32基础测试版");
  Serial.println("==========================================");
  
  // 显示系统信息
  Serial.println("系统信息:");
  Serial.print("  芯片型号: ");
  Serial.println(ESP.getChipModel());
  Serial.print("  可用内存: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
  Serial.print("  CPU温度: ");
  Serial.print(temperatureRead());
  Serial.println("°C");
  Serial.print("  CPU频率: ");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println(" MHz");
  Serial.print("  Flash大小: ");
  Serial.print(ESP.getFlashChipSize());
  Serial.println(" bytes");
  Serial.println();
  
  // 测试GPIO
  Serial.println("GPIO测试:");
  pinMode(2, OUTPUT); // 内置LED
  Serial.println("✓ GPIO初始化成功");
  
  // 测试WiFi库（不连接）
  Serial.println("WiFi库测试:");
  WiFi.mode(WIFI_OFF);
  Serial.println("✓ WiFi库加载成功");
  
  Serial.println("==========================================");
  Serial.println("基础测试完成！");
  Serial.println("如果看到这条消息，说明ESP32基本功能正常");
  Serial.println("问题可能出现在BLE库上");
  Serial.println("==========================================");
}

void loop() {
  // 闪烁LED
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 1000) {
    lastBlink = millis();
    digitalWrite(2, !digitalRead(2));
  }
  
  // 显示状态信息
  static unsigned long lastStatusDisplay = 0;
  if (millis() - lastStatusDisplay > 10000) {
    lastStatusDisplay = millis();
    
    Serial.println();
    Serial.println("==========================================");
    Serial.println("ESP32运行状态");
    Serial.println("==========================================");
    Serial.println("运行时间: " + String(millis() / 1000) + " 秒");
    Serial.println("可用内存: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("CPU温度: " + String(temperatureRead()) + "°C");
    Serial.println("LED状态: " + String(digitalRead(2) ? "开" : "关"));
    Serial.println("==========================================");
    Serial.println();
  }
  
  delay(100);
}
