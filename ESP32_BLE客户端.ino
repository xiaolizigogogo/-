/*
 * ESP32 BLE客户端
 * 功能：作为BLE主设备，扫描并连接服务器，进行数据通信
 * 特点：自动扫描、连接管理、数据双向传输
 */

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEUtils.h>

// BLE配置
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-cba987654321"

// 目标设备名称
#define TARGET_DEVICE_NAME "ESP32_BLE_Server"

// 全局变量
BLEClient* pClient = NULL;
BLERemoteCharacteristic* pRemoteCharacteristic = NULL;
bool doConnect = false;
bool connected = false;
bool doScan = false;
unsigned long lastHeartbeat = 0;
unsigned long lastCommand = 0;
int messageCount = 0;
String lastReceivedMessage = "";

// 扫描回调类
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.print("发现BLE设备: ");
      Serial.println(advertisedDevice.toString().c_str());

      // 检查是否是目标设备
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

// 客户端回调类
class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
      Serial.println("✓ 已连接到BLE服务器");
      connected = true;
    }

    void onDisconnect(BLEClient* pclient) {
      connected = false;
      Serial.println("✗ 与BLE服务器断开连接");
      doScan = true;
    }
};

// 特征回调类
class MyNotifyCallback: public BLERemoteCharacteristicCallbacks {
    void onNotify(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                  uint8_t* pData, size_t length, bool isNotify) {
      String receivedData = String((char*)pData);
      Serial.println("收到服务器数据: " + receivedData);
      lastReceivedMessage = receivedData;
      messageCount++;
      
      // 处理接收到的数据
      handleReceivedData(receivedData);
    }
};

void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(1000);
  
  Serial.println();
  Serial.println("==========================================");
  Serial.println("ESP32 BLE客户端");
  Serial.println("==========================================");
  
  // 显示系统信息
  displaySystemInfo();
  
  // 初始化BLE
  initBLE();
  
  Serial.println();
  Serial.println("BLE客户端已启动");
  Serial.println("目标设备: " + String(TARGET_DEVICE_NAME));
  Serial.println("服务UUID: " + String(SERVICE_UUID));
  Serial.println("特征UUID: " + String(CHARACTERISTIC_UUID));
  Serial.println("开始扫描BLE设备...");
  Serial.println("==========================================");
}

void loop() {
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
  Serial.println("初始化BLE客户端...");
  
  // 初始化BLE设备
  BLEDevice::init("");
  
  // 创建BLE扫描
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
  
  Serial.println("✓ BLE客户端初始化完成");
  Serial.println("✓ 开始扫描BLE设备");
}

// 连接到服务器
bool connectToServer() {
  Serial.println("连接到BLE服务器...");
  
  // 连接到远程BLE服务器
  if (!pClient->connect()) {
    Serial.println("连接失败");
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
    pRemoteCharacteristic->registerForNotify(new MyNotifyCallback());
  }
  
  Serial.println("✓ 成功连接到BLE服务器");
  return true;
}

// 发送心跳
void sendHeartbeat() {
  if (millis() - lastHeartbeat > 5000) { // 每5秒发送一次
    lastHeartbeat = millis();
    
    if (connected && pRemoteCharacteristic != nullptr) {
      String heartbeat = "heartbeat:" + String(millis()) + ":client";
      pRemoteCharacteristic->writeValue(heartbeat.c_str(), heartbeat.length());
      Serial.println("发送心跳到服务器: " + heartbeat);
    }
  }
}

// 发送测试命令
void sendTestCommands() {
  if (millis() - lastCommand > 10000) { // 每10秒发送一次测试命令
    lastCommand = millis();
    
    if (connected && pRemoteCharacteristic != nullptr) {
      // 轮换发送不同的测试命令
      static int commandIndex = 0;
      String commands[] = {"test", "status", "ping", "info"};
      
      String command = commands[commandIndex % 4];
      pRemoteCharacteristic->writeValue(command.c_str(), command.length());
      Serial.println("发送命令到服务器: " + command);
      
      commandIndex++;
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

// 发送自定义命令
void sendCommand(String command) {
  if (connected && pRemoteCharacteristic != nullptr) {
    pRemoteCharacteristic->writeValue(command.c_str(), command.length());
    Serial.println("发送命令: " + command);
  } else {
    Serial.println("未连接到服务器，无法发送命令");
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
    Serial.println("  服务器连接: " + String(connected ? "已连接" : "未连接"));
    Serial.println("  消息计数: " + String(messageCount));
    
    if (lastReceivedMessage.length() > 0) {
      Serial.println("  最后消息: " + lastReceivedMessage);
    }
    
    Serial.println("系统状态:");
    Serial.println("  可用内存: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("  CPU温度: " + String(temperatureRead()) + "°C");
    Serial.println("  运行时间: " + String(millis() / 1000) + " 秒");
    
    if (!connected) {
      Serial.println();
      Serial.println("正在搜索BLE服务器...");
      Serial.println("目标设备: " + String(TARGET_DEVICE_NAME));
    }
    
    Serial.println("==========================================");
    Serial.println();
  }
}
