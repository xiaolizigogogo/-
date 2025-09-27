/*
 * Arduino Mega 2560 从控制器
 * 通过串口与ESP32通信，执行具体的硬件控制任务
 * 替代nRF24L01从控制器功能
 */

#include <SoftwareSerial.h>

// 与ESP32的串口通信
SoftwareSerial esp32Serial(18, 19); // RX=18, TX=19

// 电机控制引脚
const int MOTOR1_ENABLE = 2;
const int MOTOR1_IN1 = 3;
const int MOTOR1_IN2 = 4;
const int MOTOR1_PWM = 5;

const int MOTOR2_ENABLE = 6;
const int MOTOR2_IN1 = 7;
const int MOTOR2_IN2 = 8;
const int MOTOR2_PWM = 9;

// LED控制引脚
const int LED_PIN = 13;

// 传感器引脚
const int TEMP_SENSOR_PIN = A0;
const int HUMIDITY_SENSOR_PIN = A1;

// 状态变量
struct MotorState {
  bool enabled = false;
  int speed = 0;
  bool direction = true; // true = 正向, false = 反向
} motor1, motor2;

struct SystemState {
  bool led_status = false;
  int temperature = 0;
  int humidity = 0;
  bool emergency_stop = false;
  unsigned long lastHeartbeat = 0;
} systemState;

// 定时器
unsigned long lastStatusSend = 0;
unsigned long lastHeartbeat = 0;
const unsigned long STATUS_INTERVAL = 2000; // 2秒发送一次状态
const unsigned long HEARTBEAT_INTERVAL = 5000; // 5秒发送一次心跳

void setup() {
  Serial.begin(115200);
  esp32Serial.begin(9600);
  
  Serial.println("=== Arduino Mega 2560 从控制器启动 ===");
  
  // 初始化引脚
  initPins();
  
  // 发送初始化完成信号
  sendToESP32("INIT_OK");
  
  Serial.println("Mega 2560从控制器初始化完成");
  Serial.println("等待ESP32命令...");
}

void loop() {
  // 处理来自ESP32的命令
  handleESP32Commands();
  
  // 定期发送状态信息
  if (millis() - lastStatusSend > STATUS_INTERVAL) {
    lastStatusSend = millis();
    sendStatusToESP32();
  }
  
  // 定期发送心跳
  if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL) {
    lastHeartbeat = millis();
    sendHeartbeatToESP32();
  }
  
  // 更新传感器读数
  updateSensors();
  
  // 更新电机控制
  updateMotors();
  
  // 更新LED状态
  updateLED();
  
  delay(10);
}

void initPins() {
  // 电机控制引脚
  pinMode(MOTOR1_ENABLE, OUTPUT);
  pinMode(MOTOR1_IN1, OUTPUT);
  pinMode(MOTOR1_IN2, OUTPUT);
  pinMode(MOTOR1_PWM, OUTPUT);
  
  pinMode(MOTOR2_ENABLE, OUTPUT);
  pinMode(MOTOR2_IN1, OUTPUT);
  pinMode(MOTOR2_IN2, OUTPUT);
  pinMode(MOTOR2_PWM, OUTPUT);
  
  // LED引脚
  pinMode(LED_PIN, OUTPUT);
  
  // 传感器引脚
  pinMode(TEMP_SENSOR_PIN, INPUT);
  pinMode(HUMIDITY_SENSOR_PIN, INPUT);
  
  // 初始状态
  digitalWrite(MOTOR1_ENABLE, LOW);
  digitalWrite(MOTOR2_ENABLE, LOW);
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("引脚初始化完成");
}

void handleESP32Commands() {
  if (esp32Serial.available()) {
    String command = esp32Serial.readStringUntil('\n');
    command.trim();
    
    Serial.println("收到ESP32命令: " + command);
    
    // 解析命令
    if (command.startsWith("MOTOR_1_")) {
      handleMotor1Command(command);
    } else if (command.startsWith("MOTOR_2_")) {
      handleMotor2Command(command);
    } else if (command.startsWith("LED_")) {
      handleLEDCommand(command);
    } else if (command == "EMERGENCY_STOP") {
      handleEmergencyStop();
    } else if (command == "RESET") {
      handleReset();
    } else if (command == "HEARTBEAT") {
      handleHeartbeat();
    } else if (command == "INIT") {
      sendToESP32("INIT_OK");
    }
  }
}

void handleMotor1Command(String command) {
  if (command == "MOTOR_1_ON") {
    motor1.enabled = true;
    Serial.println("电机1开启");
  } else if (command == "MOTOR_1_OFF") {
    motor1.enabled = false;
    motor1.speed = 0;
    Serial.println("电机1关闭");
  } else if (command.startsWith("MOTOR_1_SPEED_")) {
    int speed = command.substring(14).toInt();
    motor1.speed = constrain(speed, 0, 255);
    Serial.println("电机1速度设置为: " + String(motor1.speed));
  }
}

void handleMotor2Command(String command) {
  if (command == "MOTOR_2_ON") {
    motor2.enabled = true;
    Serial.println("电机2开启");
  } else if (command == "MOTOR_2_OFF") {
    motor2.enabled = false;
    motor2.speed = 0;
    Serial.println("电机2关闭");
  } else if (command.startsWith("MOTOR_2_SPEED_")) {
    int speed = command.substring(14).toInt();
    motor2.speed = constrain(speed, 0, 255);
    Serial.println("电机2速度设置为: " + String(motor2.speed));
  }
}

void handleLEDCommand(String command) {
  if (command == "LED_ON") {
    systemState.led_status = true;
    Serial.println("LED开启");
  } else if (command == "LED_OFF") {
    systemState.led_status = false;
    Serial.println("LED关闭");
  }
}

void handleEmergencyStop() {
  systemState.emergency_stop = true;
  motor1.enabled = false;
  motor2.enabled = false;
  motor1.speed = 0;
  motor2.speed = 0;
  systemState.led_status = false;
  
  Serial.println("紧急停止激活！");
  sendToESP32("EMERGENCY_STOP_ACTIVATED");
}

void handleReset() {
  systemState.emergency_stop = false;
  motor1.enabled = false;
  motor2.enabled = false;
  motor1.speed = 0;
  motor2.speed = 0;
  systemState.led_status = false;
  
  Serial.println("系统重置");
  sendToESP32("SYSTEM_RESET");
}

void handleHeartbeat() {
  systemState.lastHeartbeat = millis();
  sendToESP32("HEARTBEAT_OK");
}

void updateSensors() {
  // 读取温度传感器 (模拟值，需要根据实际传感器调整)
  int tempRaw = analogRead(TEMP_SENSOR_PIN);
  systemState.temperature = map(tempRaw, 0, 1023, -10, 50); // 假设范围-10到50度
  
  // 读取湿度传感器 (模拟值，需要根据实际传感器调整)
  int humidityRaw = analogRead(HUMIDITY_SENSOR_PIN);
  systemState.humidity = map(humidityRaw, 0, 1023, 0, 100); // 假设范围0到100%
}

void updateMotors() {
  // 更新电机1
  if (motor1.enabled && !systemState.emergency_stop) {
    digitalWrite(MOTOR1_ENABLE, HIGH);
    analogWrite(MOTOR1_PWM, motor1.speed);
    
    if (motor1.direction) {
      digitalWrite(MOTOR1_IN1, HIGH);
      digitalWrite(MOTOR1_IN2, LOW);
    } else {
      digitalWrite(MOTOR1_IN1, LOW);
      digitalWrite(MOTOR1_IN2, HIGH);
    }
  } else {
    digitalWrite(MOTOR1_ENABLE, LOW);
    analogWrite(MOTOR1_PWM, 0);
  }
  
  // 更新电机2
  if (motor2.enabled && !systemState.emergency_stop) {
    digitalWrite(MOTOR2_ENABLE, HIGH);
    analogWrite(MOTOR2_PWM, motor2.speed);
    
    if (motor2.direction) {
      digitalWrite(MOTOR2_IN1, HIGH);
      digitalWrite(MOTOR2_IN2, LOW);
    } else {
      digitalWrite(MOTOR2_IN1, LOW);
      digitalWrite(MOTOR2_IN2, HIGH);
    }
  } else {
    digitalWrite(MOTOR2_ENABLE, LOW);
    analogWrite(MOTOR2_PWM, 0);
  }
}

void updateLED() {
  digitalWrite(LED_PIN, systemState.led_status ? HIGH : LOW);
}

void sendStatusToESP32() {
  String status = "STATUS:";
  status += "TEMP:" + String(systemState.temperature) + ":";
  status += "HUM:" + String(systemState.humidity) + ":";
  status += "MOTOR1:" + String(motor1.enabled ? "ON" : "OFF") + ":";
  status += "SPEED1:" + String(motor1.speed) + ":";
  status += "MOTOR2:" + String(motor2.enabled ? "ON" : "OFF") + ":";
  status += "SPEED2:" + String(motor2.speed) + ":";
  status += "LED:" + String(systemState.led_status ? "ON" : "OFF");
  
  sendToESP32(status);
}

void sendHeartbeatToESP32() {
  sendToESP32("HEARTBEAT");
}

void sendToESP32(String message) {
  esp32Serial.println(message);
  Serial.println("发送到ESP32: " + message);
}

// 调试功能
void printSystemStatus() {
  Serial.println("=== 系统状态 ===");
  Serial.println("电机1: " + String(motor1.enabled ? "开启" : "关闭") + 
                 " 速度: " + String(motor1.speed));
  Serial.println("电机2: " + String(motor2.enabled ? "开启" : "关闭") + 
                 " 速度: " + String(motor2.speed));
  Serial.println("LED: " + String(systemState.led_status ? "开启" : "关闭"));
  Serial.println("温度: " + String(systemState.temperature) + "°C");
  Serial.println("湿度: " + String(systemState.humidity) + "%");
  Serial.println("紧急停止: " + String(systemState.emergency_stop ? "激活" : "正常"));
  Serial.println("================");
}



