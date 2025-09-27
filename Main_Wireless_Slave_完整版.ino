/*
 * 子设备 - 接收主设备的EEPROM数据并同步到本地
 * 功能：连接主设备热点，接收系统参数数据，保存到本地EEPROM
 */

#include <WiFi.h>
#include <WiFiUDP.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

// WiFi连接配置
const char* ssid = "MainController";
const char* password = "12345678";

// UDP配置
WiFiUDP udp;
const int udp_port = 1234;
IPAddress server_ip(192, 168, 4, 1); // 主设备IP

// EEPROM地址定义（与主设备保持一致）
int addr1 = 0, addr2 = 1, addr3 = 2, addr4 = 3, addr5 = 4;
int addr6 = 5, addr7 = 6, addr8 = 7, addr9 = 8, addr10 = 9;
int addr101 = 9, addr11 = 10, addr111 = 11, addr12 = 12;
int addr18 = 23;

// 数据同步相关
unsigned long lastRequestTime = 0;
const unsigned long REQUEST_INTERVAL = 5000; // 5秒请求一次数据
bool connected = false;
unsigned long lastDataReceived = 0;
const unsigned long CONNECTION_TIMEOUT = 10000; // 10秒超时

// 系统状态
typedef struct {
  bool motorSw;
  bool steerSw;
  int SteerLeftTimeFlag;
  int motorSwFlag;
  int motorSpeed;
  int steerSpeed;
  float steerAngle;
  int steerLTime;
  int steerRTime;
  int steerMidVal;
  int steerAngle_show;
  int encoders[5];
  unsigned long lastUpdate;
} systemData_t;

systemData_t receivedData;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("==========================================");
  Serial.println("子设备启动 - 数据同步接收器");
  Serial.println("==========================================");
  
  // 初始化EEPROM
  EEPROM.begin(512);
  
  // 连接到主设备热点
  connectToMaster();
  
  // 启动UDP客户端
  udp.begin(udp_port);
  
  Serial.println("子设备初始化完成！");
}

void loop() {
  // 检查连接状态
  if (!connected || (millis() - lastDataReceived > CONNECTION_TIMEOUT)) {
    if (connected) {
      Serial.println("连接超时，尝试重连...");
      connected = false;
    }
    connectToMaster();
  }
  
  // 处理接收到的数据
  handleReceivedData();
  
  // 定期请求数据
  if (connected && millis() - lastRequestTime > REQUEST_INTERVAL) {
    requestDataFromMaster();
    lastRequestTime = millis();
  }
  
  delay(100);
}

// 连接到主设备
void connectToMaster() {
  if (WiFi.status() == WL_CONNECTED) {
    connected = true;
    return;
  }
  
  Serial.print("正在连接主设备热点: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("✓ 连接主设备成功！");
    Serial.print("本机IP地址: ");
    Serial.println(WiFi.localIP());
    Serial.print("主设备IP: ");
    Serial.println(server_ip);
    Serial.print("UDP端口: ");
    Serial.println(udp_port);
    Serial.println("==========================================");
    connected = true;
    lastDataReceived = millis();
  } else {
    Serial.println();
    Serial.println("✗ 连接主设备失败！");
    connected = false;
  }
}

// 处理接收到的数据
void handleReceivedData() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char buffer[1024];
    int len = udp.read(buffer, 1024);
    if (len > 0) {
      buffer[len] = '\0';
      String receivedString = String(buffer);
      
      Serial.print("收到数据: ");
      Serial.println(receivedString);
      
      // 解析JSON数据
      if (parseSystemData(receivedString)) {
        // 保存到EEPROM
        saveDataToEEPROM();
        
        // 发送确认
        sendConfirmation();
        
        lastDataReceived = millis();
        Serial.println("数据同步完成！");
      }
    }
  }
}

// 解析系统数据
bool parseSystemData(String jsonString) {
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, jsonString);
  
  if (error) {
    Serial.print("JSON解析错误: ");
    Serial.println(error.c_str());
    return false;
  }
  
  // 检查数据类型
  if (doc["type"] != "SYSTEM_DATA") {
    Serial.println("数据类型不匹配");
    return false;
  }
  
  // 提取系统参数
  receivedData.steerAngle = doc["steerAngle"];
  receivedData.steerSpeed = doc["steerSpeed"];
  receivedData.steerLTime = doc["steerLTime"];
  receivedData.steerRTime = doc["steerRTime"];
  receivedData.motorSpeed = doc["motorSpeed"];
  receivedData.steerMidVal = doc["steerMidVal"];
  receivedData.steerAngle_show = doc["steerAngle_show"];
  receivedData.SteerLeftTimeFlag = doc["SteerLeftTimeFlag"];
  receivedData.motorSw = doc["motorSw"];
  receivedData.steerSw = doc["steerSw"];
  receivedData.motorSwFlag = doc["motorSwFlag"];
  
  // 提取编码器数据
  JsonArray encoders = doc["encoders"];
  for (int i = 0; i < 5 && i < encoders.size(); i++) {
    receivedData.encoders[i] = encoders[i];
  }
  
  receivedData.lastUpdate = millis();
  
  // 打印接收到的数据
  Serial.println("=== 接收到的系统数据 ===");
  Serial.print("舵机角度: "); Serial.println(receivedData.steerAngle);
  Serial.print("舵机速度: "); Serial.println(receivedData.steerSpeed);
  Serial.print("左转时间: "); Serial.println(receivedData.steerLTime);
  Serial.print("右转时间: "); Serial.println(receivedData.steerRTime);
  Serial.print("电机速度: "); Serial.println(receivedData.motorSpeed);
  Serial.print("中位值: "); Serial.println(receivedData.steerMidVal);
  Serial.print("显示角度: "); Serial.println(receivedData.steerAngle_show);
  Serial.print("电机开关: "); Serial.println(receivedData.motorSw ? "ON" : "OFF");
  Serial.print("舵机开关: "); Serial.println(receivedData.steerSw ? "ON" : "OFF");
  Serial.print("编码器数据: ");
  for (int i = 0; i < 5; i++) {
    Serial.print(receivedData.encoders[i]);
    if (i < 4) Serial.print(", ");
  }
  Serial.println();
  Serial.println("========================");
  
  return true;
}

// 保存数据到EEPROM
void saveDataToEEPROM() {
  Serial.println("保存数据到EEPROM...");
  
  // 保存系统参数
  EEPROM.write(addr1, (int)receivedData.steerAngle);
  EEPROM.write(addr2, receivedData.steerSpeed);
  EEPROM.write(addr3, receivedData.steerLTime);
  EEPROM.write(addr4, receivedData.steerRTime);
  EEPROM.write(addr5, receivedData.motorSpeed);
  
  // 保存编码器数据
  EEPROM.write(addr6, receivedData.encoders[0]);
  EEPROM.write(addr7, receivedData.encoders[1]);
  EEPROM.write(addr8, receivedData.encoders[2]);
  EEPROM.write(addr9, receivedData.encoders[4]);
  
  // 保存编码器3（需要分解）
  int a1 = receivedData.encoders[3] % 100;
  int a2 = receivedData.encoders[3] / 100;
  EEPROM.write(addr10, a1);
  EEPROM.write(addr101, a2);
  
  // 保存中位值（需要分解）
  int a3 = receivedData.steerMidVal % 100;
  int a4 = receivedData.steerMidVal / 100;
  EEPROM.write(addr11, a3);
  EEPROM.write(addr111, a4);
  
  // 保存其他参数
  EEPROM.write(addr12, receivedData.SteerLeftTimeFlag);
  EEPROM.write(addr18, receivedData.steerAngle_show);
  EEPROM.write(200, 1); // 标记数据有效
  
  EEPROM.commit();
  
  Serial.println("EEPROM数据保存完成！");
  
  // 验证保存的数据
  verifySavedData();
}

// 验证保存的数据
void verifySavedData() {
  Serial.println("=== 验证保存的数据 ===");
  Serial.print("舵机角度: "); Serial.println(EEPROM.read(addr1));
  Serial.print("舵机速度: "); Serial.println(EEPROM.read(addr2));
  Serial.print("左转时间: "); Serial.println(EEPROM.read(addr3));
  Serial.print("右转时间: "); Serial.println(EEPROM.read(addr4));
  Serial.print("电机速度: "); Serial.println(EEPROM.read(addr5));
  Serial.print("编码器0: "); Serial.println(EEPROM.read(addr6));
  Serial.print("编码器1: "); Serial.println(EEPROM.read(addr7));
  Serial.print("编码器2: "); Serial.println(EEPROM.read(addr8));
  Serial.print("编码器4: "); Serial.println(EEPROM.read(addr9));
  
  int a1 = EEPROM.read(addr10);
  int a2 = EEPROM.read(addr101);
  Serial.print("编码器3: "); Serial.println(a2 * 100 + a1);
  
  int a3 = EEPROM.read(addr11);
  int a4 = EEPROM.read(addr111);
  Serial.print("中位值: "); Serial.println(a4 * 100 + a3);
  
  Serial.print("状态标志: "); Serial.println(EEPROM.read(addr12));
  Serial.print("显示角度: "); Serial.println(EEPROM.read(addr18));
  Serial.print("数据有效: "); Serial.println(EEPROM.read(200));
  Serial.println("=====================");
}

// 请求数据
void requestDataFromMaster() {
  String request = "REQUEST_DATA";
  udp.beginPacket(server_ip, udp_port);
  udp.print(request);
  udp.endPacket();
  
  Serial.println("请求主设备发送数据");
}

// 发送确认
void sendConfirmation() {
  String confirm = "CONFIRM_RECEIVED";
  udp.beginPacket(server_ip, udp_port);
  udp.print(confirm);
  udp.endPacket();
  
  Serial.println("发送确认到主设备");
}

// 获取连接状态
bool isConnected() {
  return connected && (millis() - lastDataReceived < CONNECTION_TIMEOUT);
}

// 获取最后接收数据的时间
unsigned long getLastDataTime() {
  return receivedData.lastUpdate;
}

// 获取系统数据
systemData_t getSystemData() {
  return receivedData;
}
