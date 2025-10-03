/*
 * ESP32 BLE简化测试版
 * 功能：简化的BLE通信测试程序，可以配置为服务器或客户端
 * 特点：一键切换模式、快速测试、易于调试
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// BLE配置
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-cba987654321"

// 设备模式配置
// 设置为 true 为服务器模式，false 为客户端模式
#define SERVER_MODE true

// 目标设备名称（客户端模式使用）
#define TARGET_DEVICE_NAME "ESP32_BLE_Test"

// 全局变量
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
BLEClient* pClient = NULL;
BLERemoteCharacteristic* pRemoteCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
bool doConnect = false;
bool doScan = false;
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

// 扫描回调类（客户端模式）
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.print("发现BLE设备: ");
      Serial.println(advertisedDevice.toString().c_str());

      if (advertisedDevice.haveName() && 
          advertisedDevice.getName() == TARGET_DEVICE_NAME) {
        
        Serial.println("✓ 找到目标设备: " + String(TARGET_DEVICE_NAME));
        
        BLEDevice::getScan()->stop();
        pRemoteCharacteristic = nullptr;
        pClient = BLEDevice::createClient();
        
        // 连接到服务器
        pClient->connect(&advertisedDevice);
        doConnect = true;
      }
    }
};

// 客户端回调类
class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
      Serial.println("✓ 已连接到BLE服务器");
      deviceConnected = true;
    }

    void onDisconnect(BLEClient* pclient) {
      deviceConnected = false;
      Serial.println("✗ 与BLE服务器断开连接");
      doScan = true;
    }
};

// 通知回调类
class MyNotifyCallback: public BLERemoteCharacteristicCallbacks {
    void onNotify(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                  uint8_t* pData, size_t length, bool isNotify) {
      String receivedData = String((char*)pData);
      Serial.println("收到服务器数据: " + receivedData);
      messageCount++;
    }
};

void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(1000);
  
  Serial.println();
  Serial.println("==========================================");
  Serial.println("ESP32 BLE简化测试版");
  Serial.println("==========================================");
  
  // 显示系统信息
  displaySystemInfo();
  
  // 显示模式信息
  if (SERVER_MODE) {
    Serial.println("运行模式: BLE服务器");
    Serial.println("设备名称: ESP32_BLE_Test");
    initBLEServer();
  } else {
    Serial.println("运行模式: BLE客户端");
    Serial.println("目标设备: " + String(TARGET_DEVICE_NAME));
    initBLEClient();
  }
  
  Serial.println("==========================================");
}

void loop() {
  if (SERVER_MODE) {
    // 服务器模式循环
    serverLoop();
  } else {
    // 客户端模式循环
    clientLoop();
  }
  
  delay(1000);
}

// 显示系统信息
void displaySystemInfo() {
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
}

// 初始化BLE服务器
void initBLEServer() {
  Serial.println("初始化BLE服务器...");
  
  BLEDevice::init("ESP32_BLE_Test");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("✓ BLE服务器初始化完成");
  Serial.println("✓ 开始广播，等待客户端连接");
}

// 初始化BLE客户端
void initBLEClient() {
  Serial.println("初始化BLE客户端...");
  
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
  
  Serial.println("✓ BLE客户端初始化完成");
  Serial.println("✓ 开始扫描BLE设备");
}

// 服务器模式循环
void serverLoop() {
  // 处理连接状态变化
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("重新开始广播");
    oldDeviceConnected = deviceConnected;
  }
  
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
  
  // 发送心跳
  sendHeartbeat();
  
  // 显示状态
  showServerStatus();
}

// 客户端模式循环
void clientLoop() {
  // 处理连接
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("✓ 成功连接到BLE服务器");
    } else {
      Serial.println("✗ 连接BLE服务器失败");
    }
    doConnect = false;
  }

  // 如果断开连接，重新扫描
  if (doScan) {
    BLEDevice::getScan()->start(0);
    doScan = false;
  }

  // 发送心跳
  sendHeartbeat();
  
  // 发送测试命令
  sendTestCommands();
  
  // 显示状态
  showClientStatus();
}

// 连接到服务器
bool connectToServer() {
  Serial.println("连接到BLE服务器...");
  
  if (!pClient->connect()) {
    Serial.println("连接失败");
    return false;
  }
  
  BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
  if (pRemoteService == nullptr) {
    Serial.println("未找到服务");
    pClient->disconnect();
    return false;
  }
  
  pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.println("未找到特征");
    pClient->disconnect();
    return false;
  }
  
  if (pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(new MyNotifyCallback());
  }
  
  Serial.println("✓ 成功连接到BLE服务器");
  return true;
}

// 发送心跳
void sendHeartbeat() {
  if (millis() - lastHeartbeat > 5000) {
    lastHeartbeat = millis();
    
    if (deviceConnected) {
      String heartbeat = "heartbeat:" + String(millis());
      if (SERVER_MODE) {
        heartbeat += ":server";
        pCharacteristic->setValue(heartbeat.c_str());
        pCharacteristic->notify();
      } else {
        heartbeat += ":client";
        if (pRemoteCharacteristic != nullptr) {
          pRemoteCharacteristic->writeValue(heartbeat.c_str(), heartbeat.length());
        }
      }
      Serial.println("发送心跳: " + heartbeat);
    }
  }
}

// 发送测试命令（客户端模式）
void sendTestCommands() {
  static unsigned long lastCommand = 0;
  if (millis() - lastCommand > 10000) {
    lastCommand = millis();
    
    if (deviceConnected && pRemoteCharacteristic != nullptr) {
      static int commandIndex = 0;
      String commands[] = {"test", "status", "ping"};
      
      String command = commands[commandIndex % 3];
      pRemoteCharacteristic->writeValue(command.c_str(), command.length());
      Serial.println("发送命令: " + command);
      
      commandIndex++;
    }
  }
}

// 处理接收到的数据
void handleReceivedData(String data) {
  if (data == "test") {
    sendResponse("test_response:success:BLE通信正常");
  } else if (data == "status") {
    sendResponse("status_response:connected:yes:memory:" + String(ESP.getFreeHeap()));
  } else if (data == "ping") {
    sendResponse("pong:server:" + String(millis()));
  } else {
    sendResponse("echo:" + data);
  }
}

// 发送响应
void sendResponse(String response) {
  if (SERVER_MODE && pCharacteristic != nullptr) {
    pCharacteristic->setValue(response.c_str());
    pCharacteristic->notify();
    Serial.println("发送响应: " + response);
  }
}

// 显示服务器状态
void showServerStatus() {
  static unsigned long lastDisplay = 0;
  if (millis() - lastDisplay > 10000) {
    lastDisplay = millis();
    
    Serial.println("服务器状态: " + String(deviceConnected ? "已连接" : "未连接") + 
                   " | 消息: " + String(messageCount) + 
                   " | 内存: " + String(ESP.getFreeHeap()));
  }
}

// 显示客户端状态
void showClientStatus() {
  static unsigned long lastDisplay = 0;
  if (millis() - lastDisplay > 10000) {
    lastDisplay = millis();
    
    Serial.println("客户端状态: " + String(deviceConnected ? "已连接" : "未连接") + 
                   " | 消息: " + String(messageCount) + 
                   " | 内存: " + String(ESP.getFreeHeap()));
  }
}
