/*
 * ESP32 BLE简单测试版
 * 功能：最简单的BLE测试，避免重启问题
 * 特点：最小化BLE功能，逐步测试
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
      String rxValueStr = String(pCharacteristic->getValue().c_str());

      if (rxValueStr.length() > 0) {
        Serial.println("收到数据: " + rxValueStr);
        messageCount++;
        
        // 简单回显
        pCharacteristic->setValue("echo: " + rxValueStr);
        pCharacteristic->notify();
      }
    }
};

void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(2000);
  
  Serial.println();
  Serial.println("==========================================");
  Serial.println("ESP32 BLE简单测试版");
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
  
  // 延迟初始化BLE
  Serial.println("等待5秒后初始化BLE...");
  delay(5000);
  
  // 初始化BLE
  Serial.println("开始初始化BLE服务器...");
  
  // 初始化BLE设备
  BLEDevice::init("ESP32_BLE_Test");
  Serial.println("✓ BLE设备初始化成功");
  
  // 创建BLE服务器
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  Serial.println("✓ BLE服务器创建成功");
  
  // 创建BLE服务
  BLEService *pService = pServer->createService(SERVICE_UUID);
  Serial.println("✓ BLE服务创建成功");
  
  // 创建BLE特征
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  Serial.println("✓ BLE特征创建成功");
  
  // 设置特征回调
  pCharacteristic->setCallbacks(new MyCallbacks());
  Serial.println("✓ BLE特征回调设置成功");
  
  // 添加描述符
  pCharacteristic->addDescriptor(new BLE2902());
  Serial.println("✓ BLE描述符添加成功");
  
  // 启动服务
  pService->start();
  Serial.println("✓ BLE服务启动成功");
  
  // 开始广播
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("✓ BLE广播启动成功");
  
  Serial.println("==========================================");
  Serial.println("BLE服务器初始化完成！");
  Serial.println("设备名称: ESP32_BLE_Test");
  Serial.println("等待客户端连接...");
  Serial.println("==========================================");
}

void loop() {
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
  
  // 定期发送心跳
  static unsigned long lastHeartbeat = 0;
  if (deviceConnected && millis() - lastHeartbeat > 5000) {
    lastHeartbeat = millis();
    
    String heartbeat = "heartbeat:" + String(millis()) + ":server";
    pCharacteristic->setValue(heartbeat);
    pCharacteristic->notify();
    Serial.println("发送心跳: " + heartbeat);
  }
  
  // 显示状态信息
  static unsigned long lastStatusDisplay = 0;
  if (millis() - lastStatusDisplay > 15000) {
    lastStatusDisplay = millis();
    
    Serial.println();
    Serial.println("==========================================");
    Serial.println("BLE服务器状态");
    Serial.println("==========================================");
    Serial.println("连接状态: " + String(deviceConnected ? "已连接" : "未连接"));
    Serial.println("消息计数: " + String(messageCount));
    Serial.println("可用内存: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("运行时间: " + String(millis() / 1000) + " 秒");
    Serial.println("==========================================");
    Serial.println();
  }
  
  delay(1000);
}
