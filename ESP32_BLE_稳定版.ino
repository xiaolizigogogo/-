/*
 * ESP32 BLE稳定版
 * 功能：修复重启问题，更稳定的BLE通信
 * 特点：添加错误处理，防止系统重启
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

// 目标设备名称
#define TARGET_DEVICE_NAME "ESP32_BLE_Server"

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
bool bleInitialized = false;

// 前向声明
void handleReceivedData(String data);
void displaySystemInfo();
void initBLE();
bool connectToServer();
void sendHeartbeat();
void sendTestCommands();
void showStatus();

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
      try {
        String rxValueStr = String(pCharacteristic->getValue().c_str());

        if (rxValueStr.length() > 0) {
          Serial.println("收到数据: " + rxValueStr);
          messageCount++;
          
          // 处理接收到的数据
          handleReceivedData(rxValueStr);
        }
      } catch (...) {
        Serial.println("处理接收数据时发生错误");
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

// 扫描回调类
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
        pClient->setClientCallbacks(new MyClientCallback());
        
        // 连接到服务器
        pClient->connect(&advertisedDevice);
        doConnect = true;
      }
    }
};

// 通知回调函数
void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                   uint8_t* pData, size_t length, bool isNotify) {
  try {
    String receivedData = String((char*)pData);
    Serial.println("收到服务器数据: " + receivedData);
    messageCount++;
    
    // 处理接收到的数据
    handleReceivedData(receivedData);
  } catch (...) {
    Serial.println("处理通知数据时发生错误");
  }
}

void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(2000); // 增加延迟确保串口稳定
  
  Serial.println();
  Serial.println("==========================================");
  Serial.println("ESP32 BLE稳定版");
  Serial.println("==========================================");
  
  // 显示系统信息
  displaySystemInfo();
  
  // 延迟初始化BLE
  Serial.println("等待3秒后初始化BLE...");
  delay(3000);
  
  // 初始化BLE
  initBLE();
  
  Serial.println("==========================================");
}

void loop() {
  // 检查BLE是否已初始化
  if (!bleInitialized) {
    Serial.println("BLE未初始化，跳过循环");
    delay(1000);
    return;
  }

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
  
  // 显示状态信息
  showStatus();
  
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

// 初始化BLE
void initBLE() {
  Serial.println("开始初始化BLE客户端...");
  
  try {
    // 初始化BLE设备
    BLEDevice::init("");
    Serial.println("✓ BLE设备初始化成功");
    
    // 创建BLE扫描
    BLEScan* pBLEScan = BLEDevice::getScan();
    if (pBLEScan == nullptr) {
      Serial.println("✗ 创建BLE扫描失败");
      return;
    }
    
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    
    Serial.println("✓ BLE扫描配置完成");
    Serial.println("✓ 开始扫描BLE设备");
    Serial.println("目标设备: " + String(TARGET_DEVICE_NAME));
    
    // 开始扫描
    pBLEScan->start(5, false);
    bleInitialized = true;
    
    Serial.println("✓ BLE客户端初始化完成");
    
  } catch (...) {
    Serial.println("✗ BLE初始化失败，发生异常");
    bleInitialized = false;
  }
}

// 连接到服务器
bool connectToServer() {
  Serial.println("连接到BLE服务器...");
  
  try {
    // 连接到远程BLE服务器
    if (!pClient->isConnected()) {
      Serial.println("客户端未连接，无法连接到服务器");
      return false;
    }
    
    // 获取远程服务
    BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
    if (pRemoteService == nullptr) {
      Serial.println("未找到服务");
      pClient->disconnect();
      return false;
    }
    
    // 获取远程特征
    pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.println("未找到特征");
      pClient->disconnect();
      return false;
    }
    
    // 注册通知回调
    if (pRemoteCharacteristic->canNotify()) {
      pRemoteCharacteristic->registerForNotify(notifyCallback);
    }
    
    Serial.println("✓ 成功连接到BLE服务器");
    return true;
    
  } catch (...) {
    Serial.println("✗ 连接服务器时发生异常");
    return false;
  }
}

// 发送心跳
void sendHeartbeat() {
  if (millis() - lastHeartbeat > 5000) { // 每5秒发送一次
    lastHeartbeat = millis();
    
    if (deviceConnected && pRemoteCharacteristic != nullptr) {
      try {
        String heartbeat = "heartbeat:" + String(millis()) + ":client";
        pRemoteCharacteristic->writeValue(heartbeat.c_str(), heartbeat.length());
        Serial.println("发送心跳到服务器: " + heartbeat);
      } catch (...) {
        Serial.println("发送心跳时发生错误");
      }
    }
  }
}

// 发送测试命令
void sendTestCommands() {
  static unsigned long lastCommand = 0;
  if (millis() - lastCommand > 10000) { // 每10秒发送一次测试命令
    lastCommand = millis();
    
    if (deviceConnected && pRemoteCharacteristic != nullptr) {
      try {
        // 轮换发送不同的测试命令
        static int commandIndex = 0;
        String commands[] = {"test", "status", "ping", "info"};
        
        String command = commands[commandIndex % 4];
        pRemoteCharacteristic->writeValue(command.c_str(), command.length());
        Serial.println("发送命令到服务器: " + command);
        
        commandIndex++;
      } catch (...) {
        Serial.println("发送命令时发生错误");
      }
    }
  }
}

// 处理接收到的数据
void handleReceivedData(String data) {
  // 解析响应
  if (data.startsWith("test_response:")) {
    Serial.println("收到测试响应: " + data);
  } else if (data.startsWith("status_response:")) {
    Serial.println("收到状态响应: " + data);
  } else if (data.startsWith("pong:")) {
    Serial.println("收到Pong响应: " + data);
  } else if (data.startsWith("echo:")) {
    Serial.println("收到回显响应: " + data);
  } else if (data.startsWith("system_info:")) {
    Serial.println("收到系统信息响应: " + data);
  } else if (data.startsWith("heartbeat:")) {
    Serial.println("收到服务器心跳: " + data);
  } else if (data.startsWith("error:")) {
    Serial.println("收到错误响应: " + data);
  } else {
    Serial.println("收到未知响应: " + data);
  }
}

// 显示状态信息
void showStatus() {
  static unsigned long lastStatusDisplay = 0;
  
  if (millis() - lastStatusDisplay > 15000) { // 每15秒显示一次
    lastStatusDisplay = millis();
    
    Serial.println();
    Serial.println("==========================================");
    Serial.println("BLE客户端状态");
    Serial.println("==========================================");
    
    Serial.println("连接状态:");
    Serial.println("  服务器连接: " + String(deviceConnected ? "已连接" : "未连接"));
    Serial.println("  消息计数: " + String(messageCount));
    Serial.println("  BLE初始化: " + String(bleInitialized ? "成功" : "失败"));
    
    Serial.println("系统状态:");
    Serial.println("  可用内存: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("  CPU温度: " + String(temperatureRead()) + "°C");
    Serial.println("  运行时间: " + String(millis() / 1000) + " 秒");
    
    if (!deviceConnected && bleInitialized) {
      Serial.println();
      Serial.println("正在搜索BLE服务器...");
      Serial.println("目标设备: " + String(TARGET_DEVICE_NAME));
    }
    
    Serial.println("==========================================");
    Serial.println();
  }
}
