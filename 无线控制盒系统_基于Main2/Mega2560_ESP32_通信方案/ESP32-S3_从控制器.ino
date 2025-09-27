/*
 * ESP32 逐步恢复功能测试版本
 * 在超最小化版本基础上逐步添加功能
 */

// 基本引脚定义
#define LED_PIN 2        // LED引脚
#define RELAY_PIN 5      // 继电器引脚
#define MOTOR_PIN 6      // 电机引脚
#define SENSOR_PIN A0    // 传感器引脚

// 全局变量
int execution_count = 0;
int sensor_value = 0;
unsigned long last_check = 0;

void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(2000); // 延长等待时间
  
  Serial.println("=== ESP32 逐步恢复功能测试 ===");
  Serial.println("系统启动成功");
  
  // 初始化LED和继电器引脚，电机引脚暂时不初始化
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(RELAY_PIN, LOW);
  
  Serial.println("引脚初始化完成");
  Serial.println("LED引脚: " + String(LED_PIN));
  Serial.println("继电器引脚: " + String(RELAY_PIN));
  
  // 简单闪烁测试
  Serial.println("开始LED闪烁测试...");
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("LED ON");
    delay(200);
    digitalWrite(LED_PIN, LOW);
    Serial.println("LED OFF");
    delay(200);
  }
  
  Serial.println("=== 初始化完成 ===");
  Serial.println("等待串口命令...");
  Serial.println("支持命令：");
  Serial.println("  led on/off - LED控制");
  Serial.println("  relay on/off - 继电器控制");
  Serial.println("  status - 系统状态");
  Serial.println();
}

void loop() {
  // 处理串口命令
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command.length() > 0) {
      Serial.println("收到命令: " + command);
      processCommand(command);
    }
  }
  
  // 定期状态检查
  if (millis() - last_check > 10000) {
    last_check = millis();
    Serial.println("系统运行正常 - 运行时间: " + String(millis()) + "ms");
  }
  
  delay(10);
}

// 处理命令
void processCommand(String command) {
  if (command.indexOf("led") >= 0) {
    if (command.indexOf("on") >= 0) {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("LED开启");
      execution_count++;
    } else if (command.indexOf("off") >= 0) {
      digitalWrite(LED_PIN, LOW);
      Serial.println("LED关闭");
      execution_count++;
    }
  } else if (command.indexOf("relay") >= 0) {
    if (command.indexOf("on") >= 0) {
      digitalWrite(RELAY_PIN, HIGH);
      Serial.println("继电器开启");
      execution_count++;
    } else if (command.indexOf("off") >= 0) {
      digitalWrite(RELAY_PIN, LOW);
      Serial.println("继电器关闭");
      execution_count++;
    }
  } else if (command.indexOf("status") >= 0) {
    sendStatus();
  } else {
    Serial.println("未知命令: " + command);
  }
}

// 发送状态
void sendStatus() {
  Serial.println("=== 系统状态 ===");
  Serial.println("执行次数: " + String(execution_count));
  Serial.println("运行时间: " + String(millis()) + "ms");
  Serial.println("可用内存: " + String(ESP.getFreeHeap()) + " bytes");
  Serial.println("CPU频率: " + String(getCpuFrequencyMhz()) + " MHz");
  Serial.println("================");
}


