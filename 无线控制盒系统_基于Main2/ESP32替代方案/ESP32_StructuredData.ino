/*
 * ESP32结构化数据传输示例
 * 支持JSON和二进制两种格式
 */

#include <BluetoothSerial.h>
#include <ArduinoJson.h>

// 蓝牙串口
BluetoothSerial SerialBT;

// 设备角色
bool isMaster = true;

// 通信状态
bool isConnected = false;

// 结构化数据定义
struct ControlData {
  bool motorSw;
  bool steerSw;
  uint8_t SteerLeftTimeFlag;
  uint8_t motorSwFlag;
  uint8_t motorSpeed;
  uint8_t steerSpeed;
  uint8_t steerInterval;
  float steerAngle;
  float steerLVal;
  float steerRVal;
  float steerNowVal;
  uint8_t steerLTime;
  uint8_t steerRTime;
  uint16_t steerLTimeAction;
  uint16_t steerRTimeAction;
  uint16_t steerMidVal;
  uint8_t steerAngle_show;
  uint16_t ecCnt[5];
  uint32_t timestamp;
  uint8_t checksum;
};

ControlData controlData;

// 数据格式选择
enum DataFormat {
  FORMAT_JSON,
  FORMAT_BINARY
};

DataFormat currentFormat = FORMAT_JSON;

void setup() {
  Serial.begin(115200);
  
  // 初始化蓝牙
  String deviceName = isMaster ? "焊接小车主控" : "焊接小车子控";
  SerialBT.begin(deviceName);
  
  Serial.println("=== ESP32结构化数据传输系统 ===");
  Serial.println("设备角色: " + String(isMaster ? "主控制器" : "子控制器"));
  Serial.println("数据格式: " + String(currentFormat == FORMAT_JSON ? "JSON" : "二进制"));
  
  // 初始化控制数据
  initControlData();
  
  if (isMaster) {
    Serial.println("等待子控制器连接...");
  } else {
    Serial.println("等待主控制器连接...");
  }
}

void loop() {
  // 处理蓝牙通信
  handleBluetoothCommunication();
  
  if (isMaster) {
    // 主控制器：发送数据
    masterLoop();
  } else {
    // 子控制器：接收数据
    slaveLoop();
  }
  
  delay(100);
}

// 初始化控制数据
void initControlData() {
  controlData.motorSw = false;
  controlData.steerSw = false;
  controlData.SteerLeftTimeFlag = 0;
  controlData.motorSwFlag = 0;
  controlData.motorSpeed = 0;
  controlData.steerSpeed = 0;
  controlData.steerInterval = 0;
  controlData.steerAngle = 0.0;
  controlData.steerLVal = 0.0;
  controlData.steerRVal = 0.0;
  controlData.steerNowVal = 0.0;
  controlData.steerLTime = 0;
  controlData.steerRTime = 0;
  controlData.steerLTimeAction = 0;
  controlData.steerRTimeAction = 0;
  controlData.steerMidVal = 0;
  controlData.steerAngle_show = 0;
  
  for (int i = 0; i < 5; i++) {
    controlData.ecCnt[i] = 0;
  }
  
  controlData.timestamp = 0;
  controlData.checksum = 0;
}

// 主控制器循环
void masterLoop() {
  static unsigned long lastSendTime = 0;
  if (millis() - lastSendTime > 1000) {
    lastSendTime = millis();
    
    // 模拟数据更新
    updateControlData();
    
    // 发送数据
    if (isConnected) {
      sendStructuredData();
    }
  }
}

// 子控制器循环
void slaveLoop() {
  // 子控制器主要处理接收到的数据
  // 在handleBluetoothCommunication中处理
}

// 更新控制数据
void updateControlData() {
  // 模拟数据变化
  controlData.motorSpeed = (controlData.motorSpeed + 1) % 100;
  controlData.steerAngle = sin(millis() / 1000.0) * 50;
  controlData.steerSpeed = (controlData.steerSpeed + 2) % 100;
  
  // 更新编码器数据
  for (int i = 0; i < 5; i++) {
    controlData.ecCnt[i] = (controlData.ecCnt[i] + i + 1) % 1000;
  }
  
  controlData.timestamp = millis();
  controlData.checksum = calculateChecksum();
}

// 计算校验和
uint8_t calculateChecksum() {
  uint8_t checksum = 0;
  uint8_t* data = (uint8_t*)&controlData;
  
  // 计算除校验和字段外的所有字节
  for (int i = 0; i < sizeof(ControlData) - sizeof(uint8_t); i++) {
    checksum ^= data[i];
  }
  
  return checksum;
}

// 发送结构化数据
void sendStructuredData() {
  if (currentFormat == FORMAT_JSON) {
    sendJSONData();
  } else {
    sendBinaryData();
  }
}

// 发送JSON数据
void sendJSONData() {
  DynamicJsonDocument doc(1024);
  
  // 填充JSON数据
  doc["motorSw"] = controlData.motorSw;
  doc["steerSw"] = controlData.steerSw;
  doc["SteerLeftTimeFlag"] = controlData.SteerLeftTimeFlag;
  doc["motorSwFlag"] = controlData.motorSwFlag;
  doc["motorSpeed"] = controlData.motorSpeed;
  doc["steerSpeed"] = controlData.steerSpeed;
  doc["steerInterval"] = controlData.steerInterval;
  doc["steerAngle"] = controlData.steerAngle;
  doc["steerLVal"] = controlData.steerLVal;
  doc["steerRVal"] = controlData.steerRVal;
  doc["steerNowVal"] = controlData.steerNowVal;
  doc["steerLTime"] = controlData.steerLTime;
  doc["steerRTime"] = controlData.steerRTime;
  doc["steerLTimeAction"] = controlData.steerLTimeAction;
  doc["steerRTimeAction"] = controlData.steerRTimeAction;
  doc["steerMidVal"] = controlData.steerMidVal;
  doc["steerAngle_show"] = controlData.steerAngle_show;
  doc["timestamp"] = controlData.timestamp;
  doc["checksum"] = controlData.checksum;
  
  // 添加编码器数组
  JsonArray ecArray = doc.createNestedArray("ecCnt");
  for (int i = 0; i < 5; i++) {
    ecArray.add(controlData.ecCnt[i]);
  }
  
  // 序列化并发送
  String jsonString;
  serializeJson(doc, jsonString);
  
  SerialBT.println("JSON:" + jsonString);
  Serial.println("发送JSON数据: " + jsonString);
}

// 发送二进制数据
void sendBinaryData() {
  // 发送数据头
  SerialBT.print("BIN:");
  
  // 发送二进制数据
  uint8_t* data = (uint8_t*)&controlData;
  for (int i = 0; i < sizeof(ControlData); i++) {
    SerialBT.write(data[i]);
  }
  
  SerialBT.println(); // 结束符
  
  Serial.println("发送二进制数据，大小: " + String(sizeof(ControlData)) + " 字节");
}

// 处理蓝牙通信
void handleBluetoothCommunication() {
  if (SerialBT.available()) {
    String message = SerialBT.readString();
    message.trim();
    
    if (message == "CONNECT") {
      isConnected = true;
      Serial.println("设备已连接");
      SerialBT.println("CONNECTED");
      
    } else if (message == "DISCONNECT") {
      isConnected = false;
      Serial.println("设备已断开");
      
    } else if (message.startsWith("JSON:")) {
      // 处理JSON数据
      String jsonData = message.substring(5);
      processJSONData(jsonData);
      
    } else if (message.startsWith("BIN:")) {
      // 处理二进制数据
      String binaryData = message.substring(4);
      processBinaryData(binaryData);
      
    } else if (message == "FORMAT_JSON") {
      // 切换为JSON格式
      currentFormat = FORMAT_JSON;
      Serial.println("切换到JSON格式");
      
    } else if (message == "FORMAT_BINARY") {
      // 切换为二进制格式
      currentFormat = FORMAT_BINARY;
      Serial.println("切换到二进制格式");
    }
  }
}

// 处理JSON数据
void processJSONData(String jsonData) {
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, jsonData);
  
  if (error) {
    Serial.println("JSON解析错误: " + String(error.c_str()));
    return;
  }
  
  // 解析数据
  controlData.motorSw = doc["motorSw"];
  controlData.steerSw = doc["steerSw"];
  controlData.SteerLeftTimeFlag = doc["SteerLeftTimeFlag"];
  controlData.motorSwFlag = doc["motorSwFlag"];
  controlData.motorSpeed = doc["motorSpeed"];
  controlData.steerSpeed = doc["steerSpeed"];
  controlData.steerInterval = doc["steerInterval"];
  controlData.steerAngle = doc["steerAngle"];
  controlData.steerLVal = doc["steerLVal"];
  controlData.steerRVal = doc["steerRVal"];
  controlData.steerNowVal = doc["steerNowVal"];
  controlData.steerLTime = doc["steerLTime"];
  controlData.steerRTime = doc["steerRTime"];
  controlData.steerLTimeAction = doc["steerLTimeAction"];
  controlData.steerRTimeAction = doc["steerRTimeAction"];
  controlData.steerMidVal = doc["steerMidVal"];
  controlData.steerAngle_show = doc["steerAngle_show"];
  controlData.timestamp = doc["timestamp"];
  controlData.checksum = doc["checksum"];
  
  // 解析编码器数组
  JsonArray ecArray = doc["ecCnt"];
  for (int i = 0; i < 5 && i < ecArray.size(); i++) {
    controlData.ecCnt[i] = ecArray[i];
  }
  
  // 验证校验和
  uint8_t calculatedChecksum = calculateChecksum();
  if (calculatedChecksum == controlData.checksum) {
    Serial.println("JSON数据接收成功，校验和正确");
    executeControl();
  } else {
    Serial.println("JSON数据校验和错误");
  }
}

// 处理二进制数据
void processBinaryData(String binaryData) {
  if (binaryData.length() >= sizeof(ControlData)) {
    // 复制数据到结构体
    memcpy(&controlData, binaryData.c_str(), sizeof(ControlData));
    
    // 验证校验和
    uint8_t calculatedChecksum = calculateChecksum();
    if (calculatedChecksum == controlData.checksum) {
      Serial.println("二进制数据接收成功，校验和正确");
      executeControl();
    } else {
      Serial.println("二进制数据校验和错误");
    }
  } else {
    Serial.println("二进制数据长度不足");
  }
}

// 执行控制指令
void executeControl() {
  Serial.println("执行控制指令:");
  Serial.println("  电机开关: " + String(controlData.motorSw));
  Serial.println("  舵机开关: " + String(controlData.steerSw));
  Serial.println("  电机速度: " + String(controlData.motorSpeed));
  Serial.println("  舵机速度: " + String(controlData.steerSpeed));
  Serial.println("  舵机角度: " + String(controlData.steerAngle));
  Serial.println("  时间戳: " + String(controlData.timestamp));
  
  // 这里添加实际的控制逻辑
  // 例如：控制电机、舵机等
}







