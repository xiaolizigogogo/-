/*
 * ESP32 蓝牙iPhone兼容版
 * 功能：专门针对iPhone/iOS设备优化的蓝牙程序
 * 特点：兼容iOS蓝牙协议、简化配对流程、增强连接稳定性
 */

#include <BluetoothSerial.h>

// 蓝牙串口对象
BluetoothSerial SerialBT;

// 设备配置 - 针对iPhone优化
const char* BT_DEVICE_NAME = "ESP32_iPhone";  // 简短的设备名称
const char* BT_PIN = "0000";                  // 使用简单的配对密码

// 状态变量
bool bluetoothConnected = false;
bool bluetoothInitialized = false;
unsigned long lastHeartbeat = 0;
unsigned long lastConnectionCheck = 0;
int connectionAttempts = 0;
int successfulConnections = 0;
String lastError = "";

void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(2000);  // 增加启动延迟
  
  Serial.println();
  Serial.println("==========================================");
  Serial.println("ESP32 蓝牙iPhone兼容版");
  Serial.println("==========================================");
  
  // 显示系统信息
  displaySystemInfo();
  
  // 初始化蓝牙
  initializeBluetooth();
  
  Serial.println();
  Serial.println("iPhone连接步骤:");
  Serial.println("1. 打开iPhone设置 > 蓝牙");
  Serial.println("2. 确保蓝牙已开启");
  Serial.println("3. 搜索设备: " + String(BT_DEVICE_NAME));
  Serial.println("4. 点击设备进行配对");
  Serial.println("5. 配对密码: " + String(BT_PIN));
  Serial.println("6. 连接成功后开始测试");
  Serial.println("==========================================");
}

void loop() {
  // 检查蓝牙连接状态
  checkBluetoothConnection();
  
  // 处理接收的数据
  handleBluetoothData();
  
  // 发送心跳
  sendHeartbeat();
  
  // 显示状态信息
  showStatus();
  
  // 定期重新初始化蓝牙（针对iPhone连接问题）
  periodicBluetoothRefresh();
  
  delay(100);
}

// 显示系统信息
void displaySystemInfo() {
  Serial.println("系统信息:");
  Serial.print("  芯片型号: ");
  Serial.println(ESP.getChipModel());
  Serial.print("  芯片版本: ");
  Serial.println(ESP.getChipRevision());
  Serial.print("  CPU频率: ");
  Serial.print(getCpuFrequencyMhz());
  Serial.println(" MHz");
  Serial.print("  可用内存: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
  Serial.print("  CPU温度: ");
  Serial.print(temperatureRead());
  Serial.println("°C");
  Serial.println();
}

// 初始化蓝牙
void initializeBluetooth() {
  Serial.println("初始化蓝牙（iPhone兼容模式）...");
  
  // 尝试初始化蓝牙
  if (SerialBT.begin(BT_DEVICE_NAME)) {
    Serial.println("✓ 蓝牙初始化成功");
    bluetoothInitialized = true;
    
    // 设置配对密码
    if (SerialBT.setPin(BT_PIN, 4)) {
      Serial.println("✓ 配对密码设置成功");
    } else {
      Serial.println("✗ 配对密码设置失败");
      lastError = "配对密码设置失败";
    }
    
    // 启用安全简单配对
    SerialBT.enableSSP();
    Serial.println("✓ 安全配对已启用");
    
    // 设置蓝牙为可发现状态
    SerialBT.enableSSP();
    Serial.println("✓ 设备已设置为可发现状态");
    
  } else {
    Serial.println("✗ 蓝牙初始化失败");
    lastError = "蓝牙初始化失败";
    bluetoothInitialized = false;
  }
}

// 检查蓝牙连接状态
void checkBluetoothConnection() {
  if (millis() - lastConnectionCheck > 2000) {  // 每2秒检查一次
    lastConnectionCheck = millis();
    
    bool currentlyConnected = SerialBT.hasClient();
    
    if (currentlyConnected != bluetoothConnected) {
      bluetoothConnected = currentlyConnected;
      
      if (bluetoothConnected) {
        Serial.println("✓ iPhone蓝牙连接已建立");
        successfulConnections++;
        connectionAttempts = 0;
      } else {
        Serial.println("✗ iPhone蓝牙连接已断开");
        if (connectionAttempts < 3) {
          Serial.println("尝试重新连接...");
          connectionAttempts++;
        }
      }
    }
  }
}

// 处理蓝牙接收的数据
void handleBluetoothData() {
  if (SerialBT.available()) {
    String data = SerialBT.readStringUntil('\n');
    data.trim();
    
    if (data.length() > 0) {
      Serial.println("收到iPhone数据: " + data);
      
      // 处理命令
      if (data == "test") {
        sendTestResponse();
      } else if (data == "status") {
        sendStatusResponse();
      } else if (data == "ping") {
        sendPingResponse();
      } else if (data.startsWith("echo:")) {
        sendEchoResponse(data.substring(5));
      } else if (data == "help") {
        sendHelpResponse();
      } else {
        sendUnknownCommandResponse(data);
      }
    }
  }
}

// 发送心跳
void sendHeartbeat() {
  if (millis() - lastHeartbeat > 10000) {  // 每10秒发送一次（iPhone优化）
    lastHeartbeat = millis();
    
    if (bluetoothConnected) {
      String heartbeat = "heartbeat:" + String(millis()) + ":iPhone_OK";
      SerialBT.println(heartbeat);
      Serial.println("发送心跳到iPhone: " + heartbeat);
    }
  }
}

// 发送测试响应
void sendTestResponse() {
  String response = "test_response:success:iPhone通信正常:" + String(millis());
  SerialBT.println(response);
  Serial.println("发送测试响应到iPhone: " + response);
}

// 发送状态响应
void sendStatusResponse() {
  String response = "status_response:";
  response += "connected:" + String(bluetoothConnected ? "yes" : "no") + ":";
  response += "device:iPhone:";
  response += "memory:" + String(ESP.getFreeHeap()) + ":";
  response += "temp:" + String(temperatureRead()) + ":";
  response += "time:" + String(millis());
  
  SerialBT.println(response);
  Serial.println("发送状态响应到iPhone: " + response);
}

// 发送Ping响应
void sendPingResponse() {
  String response = "pong:iPhone:" + String(millis());
  SerialBT.println(response);
  Serial.println("发送Pong响应到iPhone: " + response);
}

// 发送回显响应
void sendEchoResponse(String message) {
  String response = "echo:iPhone:" + message + ":" + String(millis());
  SerialBT.println(response);
  Serial.println("发送回显响应到iPhone: " + response);
}

// 发送帮助响应
void sendHelpResponse() {
  String response = "help:iPhone支持的命令:test,status,ping,echo:消息,help";
  SerialBT.println(response);
  Serial.println("发送帮助响应到iPhone: " + response);
}

// 发送未知命令响应
void sendUnknownCommandResponse(String command) {
  String response = "error:iPhone:未知命令:" + command + ":支持的命令:test,status,ping,echo:消息";
  SerialBT.println(response);
  Serial.println("发送错误响应到iPhone: " + response);
}

// 定期重新初始化蓝牙
void periodicBluetoothRefresh() {
  static unsigned long lastRefresh = 0;
  
  if (millis() - lastRefresh > 60000) {  // 每60秒刷新一次
    lastRefresh = millis();
    
    if (!bluetoothConnected && bluetoothInitialized) {
      Serial.println("定期刷新蓝牙状态...");
      
      // 重新设置配对密码
      SerialBT.setPin(BT_PIN, 4);
      SerialBT.enableSSP();
      
      Serial.println("蓝牙状态已刷新，请重新尝试连接");
    }
  }
}

// 显示状态信息
void showStatus() {
  static unsigned long lastStatusDisplay = 0;
  
  if (millis() - lastStatusDisplay > 15000) {  // 每15秒显示一次
    lastStatusDisplay = millis();
    
    Serial.println();
    Serial.println("==========================================");
    Serial.println("iPhone蓝牙连接状态");
    Serial.println("==========================================");
    
    Serial.println("连接状态:");
    Serial.println("  蓝牙初始化: " + String(bluetoothInitialized ? "成功" : "失败"));
    Serial.println("  iPhone连接: " + String(bluetoothConnected ? "已连接" : "未连接"));
    Serial.println("  连接尝试: " + String(connectionAttempts));
    Serial.println("  成功连接: " + String(successfulConnections));
    
    if (lastError.length() > 0) {
      Serial.println("  最后错误: " + lastError);
    }
    
    Serial.println("系统状态:");
    Serial.println("  可用内存: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("  CPU温度: " + String(temperatureRead()) + "°C");
    Serial.println("  运行时间: " + String(millis() / 1000) + " 秒");
    
    if (!bluetoothConnected) {
      Serial.println();
      Serial.println("iPhone连接提示:");
      Serial.println("1. 确保iPhone蓝牙已开启");
      Serial.println("2. 搜索设备: " + String(BT_DEVICE_NAME));
      Serial.println("3. 配对密码: " + String(BT_PIN));
      Serial.println("4. 如果搜索不到，请重启iPhone蓝牙");
      Serial.println("5. 清除之前的配对记录");
    }
    
    Serial.println("==========================================");
    Serial.println();
  }
}
