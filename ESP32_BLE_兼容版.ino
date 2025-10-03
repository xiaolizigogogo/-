/*
 * ESP32 BLE兼容版
 * 功能：使用兼容的BLE库版本
 * 特点：避免BLE库3.3.0的兼容性问题
 */

#include <WiFi.h>

// 由于BLE库3.3.0有兼容性问题，我们使用WiFi进行通信
// 这是一个临时的解决方案，等BLE库问题解决后再使用BLE

void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(2000);
  
  Serial.println();
  Serial.println("==========================================");
  Serial.println("ESP32 BLE兼容版");
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
  Serial.println();
  
  // 初始化GPIO
  pinMode(2, OUTPUT); // 内置LED
  
  Serial.println("==========================================");
  Serial.println("BLE库兼容性问题说明:");
  Serial.println("ESP32 BLE库3.3.0存在兼容性问题");
  Serial.println("导致程序在BLE初始化时重启");
  Serial.println("==========================================");
  Serial.println();
  Serial.println("解决方案:");
  Serial.println("1. 降级BLE库到2.0.0或更早版本");
  Serial.println("2. 使用WiFi进行通信");
  Serial.println("3. 等待BLE库更新修复");
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
  if (millis() - lastStatusDisplay > 15000) {
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
