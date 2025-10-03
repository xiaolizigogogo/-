/*
 * ESP32 BLE服务器
 * 功能：作为BLE从设备，提供服务和特征，等待客户端连接
 * 特点：低功耗、稳定连接、数据双向传输
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// BLE配置
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-cba987654321"

// 全局变量
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long lastHeartbeat = 0;
int messageCount = 0;

// 服务器回调类
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("✓ 客户端已连接");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("✗ 客户端已断开");
    }
};

// 特征回调类
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("收到数据: " + String(rxValue.c_str()));
        messageCount++;
        
        // 处理接收到的数据
        handleReceivedData(String(rxValue.c_str()));
      }
    }
};

void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(1000);
  
  Serial.println();
  Serial.println("==========================================");
  Serial.println("ESP32 BLE服务器");
  Serial.println("==========================================");
  
  // 显示系统信息
  displaySystemInfo();
  
  // 初始化BLE
  initBLE();
  
  Serial.println();
  Serial.println("BLE服务器已启动");
  Serial.println("设备名称: ESP32_BLE_Server");
  Serial.println("服务UUID: " + String(SERVICE_UUID));
  Serial.println("特征UUID: " + String(CHARACTERISTIC_UUID));
  Serial.println("等待客户端连接...");
  Serial.println("==========================================");
}

void loop() {
  // 处理连接状态变化
  handleConnectionStatus();
  
  // 发送心跳
  sendHeartbeat();
  
  // 显示状态信息
  showStatus();
  
  delay(1000);
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

// 初始化BLE
void initBLE() {
  Serial.println("初始化BLE服务器...");
  
  // 初始化BLE设备
  BLEDevice::init("ESP32_BLE_Server");
  
  // 创建BLE服务器
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // 创建BLE服务
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 创建BLE特征
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  // 设置特征回调
  pCharacteristic->setCallbacks(new MyCallbacks());
  
  // 添加描述符
  pCharacteristic->addDescriptor(new BLE2902());

  // 启动服务
  pService->start();

  // 开始广播
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("✓ BLE服务器初始化完成");
  Serial.println("✓ 开始广播，等待客户端连接");
}

// 处理连接状态变化
void handleConnectionStatus() {
  // 如果设备断开连接，重新开始广播
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // 给蓝牙栈一个机会
    pServer->startAdvertising(); // 重新开始广播
    Serial.println("重新开始广播");
    oldDeviceConnected = deviceConnected;
  }
  
  // 如果设备连接，停止广播
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
}

// 发送心跳
void sendHeartbeat() {
  if (millis() - lastHeartbeat > 5000) { // 每5秒发送一次
    lastHeartbeat = millis();
    
    if (deviceConnected) {
      String heartbeat = "heartbeat:" + String(millis()) + ":server";
      pCharacteristic->setValue(heartbeat.c_str());
      pCharacteristic->notify();
      Serial.println("发送心跳: " + heartbeat);
    }
  }
}

// 处理接收到的数据
void handleReceivedData(String data) {
  // 解析命令
  if (data == "test") {
    sendTestResponse();
  } else if (data == "status") {
    sendStatusResponse();
  } else if (data == "ping") {
    sendPingResponse();
  } else if (data.startsWith("echo:")) {
    sendEchoResponse(data.substring(5));
  } else if (data == "info") {
    sendSystemInfoResponse();
  } else {
    sendUnknownCommandResponse(data);
  }
}

// 发送测试响应
void sendTestResponse() {
  String response = "test_response:success:BLE服务器通信正常:" + String(millis());
  pCharacteristic->setValue(response.c_str());
  pCharacteristic->notify();
  Serial.println("发送测试响应: " + response);
}

// 发送状态响应
void sendStatusResponse() {
  String response = "status_response:";
  response += "connected:" + String(deviceConnected ? "yes" : "no") + ":";
  response += "device:server:";
  response += "messages:" + String(messageCount) + ":";
  response += "memory:" + String(ESP.getFreeHeap()) + ":";
  response += "temp:" + String(temperatureRead()) + ":";
  response += "time:" + String(millis());
  
  pCharacteristic->setValue(response.c_str());
  pCharacteristic->notify();
  Serial.println("发送状态响应: " + response);
}

// 发送Ping响应
void sendPingResponse() {
  String response = "pong:server:" + String(millis());
  pCharacteristic->setValue(response.c_str());
  pCharacteristic->notify();
  Serial.println("发送Pong响应: " + response);
}

// 发送回显响应
void sendEchoResponse(String message) {
  String response = "echo:server:" + message + ":" + String(millis());
  pCharacteristic->setValue(response.c_str());
  pCharacteristic->notify();
  Serial.println("发送回显响应: " + response);
}

// 发送系统信息响应
void sendSystemInfoResponse() {
  String response = "system_info:";
  response += "chip:" + String(ESP.getChipModel()) + ":";
  response += "revision:" + String(ESP.getChipRevision()) + ":";
  response += "freq:" + String(getCpuFrequencyMhz()) + ":";
  response += "memory:" + String(ESP.getFreeHeap()) + ":";
  response += "temp:" + String(temperatureRead()) + ":";
  response += "uptime:" + String(millis());
  
  pCharacteristic->setValue(response.c_str());
  pCharacteristic->notify();
  Serial.println("发送系统信息响应: " + response);
}

// 发送未知命令响应
void sendUnknownCommandResponse(String command) {
  String response = "error:server:未知命令:" + command + ":支持的命令:test,status,ping,echo:消息,info";
  pCharacteristic->setValue(response.c_str());
  pCharacteristic->notify();
  Serial.println("发送错误响应: " + response);
}

// 显示状态信息
void showStatus() {
  static unsigned long lastStatusDisplay = 0;
  
  if (millis() - lastStatusDisplay > 10000) { // 每10秒显示一次
    lastStatusDisplay = millis();
    
    Serial.println();
    Serial.println("==========================================");
    Serial.println("BLE服务器状态");
    Serial.println("==========================================");
    
    Serial.println("连接状态:");
    Serial.println("  客户端连接: " + String(deviceConnected ? "已连接" : "未连接"));
    Serial.println("  消息计数: " + String(messageCount));
    
    Serial.println("系统状态:");
    Serial.println("  可用内存: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("  CPU温度: " + String(temperatureRead()) + "°C");
    Serial.println("  运行时间: " + String(millis() / 1000) + " 秒");
    
    if (!deviceConnected) {
      Serial.println();
      Serial.println("等待BLE客户端连接...");
      Serial.println("设备名称: ESP32_BLE_Server");
      Serial.println("服务UUID: " + String(SERVICE_UUID));
    }
    
    Serial.println("==========================================");
    Serial.println();
  }
}
