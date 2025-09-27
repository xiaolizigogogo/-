/*
 * 完整配对成功程序
 * 实现完整的配对逻辑、状态持久化和通信识别
 * 
 * 接线：
 * Arduino Mega 2560    →    433MHz发射接收一体模块
 * 引脚16 (TX)         →    RXD
 * 引脚17 (RX)         →    TXD
 * GND                 →    GND
 * 5V/3.3V             →    VCC
 * 
 * 使用方法：
 * 1. 上传此程序到两个Arduino设备
 * 2. 长按bt1键3秒进入配对模式
 * 3. 观察配对过程
 */

#include <EEPROM.h>  // 添加EEPROM库

#define bt1 31  // 配对按键

// 配对状态管理
struct PairingState {
  bool isPaired;
  byte deviceId[4];  // 设备ID
  unsigned long lastHeartbeat;
  int heartbeatCount;
};

PairingState pairingState = {false, {0, 0, 0, 0}, 0, 0};

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);
  
  pinMode(bt1, INPUT_PULLUP);
  
  Serial.println("=== 完整配对成功程序 ===");
  Serial.println("实现完整的配对逻辑、状态持久化和通信识别");
  Serial.println("长按bt1键3秒进入配对模式");
  Serial.println("===============================");
  
  delay(2000);
  
  // 从EEPROM加载配对状态
  loadPairingState();
  
  if (pairingState.isPaired) {
    Serial.println(">>> 已配对状态，设备ID: ");
    for (int i = 0; i < 4; i++) {
      Serial.print("0x");
      if (pairingState.deviceId[i] < 0x10) Serial.print("0");
      Serial.print(pairingState.deviceId[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  } else {
    Serial.println(">>> 未配对状态，长按bt1键3秒进入配对模式");
  }
  
  // 完整初始化模块
  Serial.println(">>> 完整初始化模块...");
  initializeModule();
}

void loop() {
  static unsigned long bt1_press_time = 0;
  static bool bt1_pressed = false;
  static bool pairingMode = false;
  static unsigned long pairingStartTime = 0;
  static unsigned long lastPairingSendTime = 0;
  static unsigned long lastHeartbeatTime = 0;
  
  unsigned long currentTime = millis();
  
  // 检查bt1长按（3秒）进入配对模式
  if (digitalRead(bt1) == LOW) {
    if (!bt1_pressed) {
      bt1_pressed = true;
      bt1_press_time = currentTime;
      Serial.println(">>> bt1按下，开始计时...");
    } else {
      // 长按3秒进入配对模式
      if (currentTime - bt1_press_time >= 3000 && !pairingMode) {
        Serial.println(">>> bt1长按3秒，进入配对模式！");
        pairingMode = true;
        pairingStartTime = currentTime;
        
        // 进入配对模式时重新完整初始化模块
        Serial.println(">>> 重新完整初始化模块...");
        initializeModule();
      }
    }
  } else {
    if (bt1_pressed) {
      bt1_pressed = false;
      if (currentTime - bt1_press_time < 3000) {
        // 短按，退出配对模式
        if (pairingMode) {
          Serial.println(">>> 短按bt1，退出配对模式");
          pairingMode = false;
        }
      }
    }
  }
  
  // 配对模式处理
  if (pairingMode) {
    // 配对模式持续30秒
    if (currentTime - pairingStartTime > 30000) {
      Serial.println(">>> 配对模式超时，退出配对模式");
      pairingMode = false;
      return;
    }
    
    // 每10秒发送一次配对请求
    if (currentTime - lastPairingSendTime >= 10000) {
      Serial.println(">>> 配对模式：发送配对请求");
      
      // 使用与初始化相同的发送方式：字节数组发送
      Serial.println(">>> 发送配对请求：FD 03 D5 AA 55 96 DF");
      
      byte pairingRequest[] = {0xFD, 0x03, 0xD5, 0xAA, 0x55, 0x96, 0xDF};
      Serial2.write(pairingRequest, 7);
      Serial2.flush();
      delay(1000);
      
      Serial.println(">>> 配对请求发送完成");
      
      lastPairingSendTime = currentTime;
    }
    
    // 持续监听接收数据
    if (Serial2.available()) {
      Serial.print(">>> 配对模式收到数据: ");
      
      // 读取所有可用数据
      byte receivedData[100];
      int dataCount = 0;
      while (Serial2.available() && dataCount < 100) {
        receivedData[dataCount] = Serial2.read();
        dataCount++;
      }
      
      // 显示接收到的数据
      for (int i = 0; i < dataCount; i++) {
        Serial.print("0x");
        if (receivedData[i] < 0x10) Serial.print("0");
        Serial.print(receivedData[i], HEX);
        Serial.print(" ");
      }
      Serial.print("(共");
      Serial.print(dataCount);
      Serial.println("字节)");
      
      // 检查数据帧
      for (int i = 0; i <= dataCount - 7; i++) {
        if (receivedData[i] == 0xFD && receivedData[i + 6] == 0xDF) {
          Serial.print(">>> 找到有效数据帧: ");
          for (int j = 0; j < 7; j++) {
            Serial.print("0x");
            if (receivedData[i + j] < 0x10) Serial.print("0");
            Serial.print(receivedData[i + j], HEX);
            Serial.print(" ");
          }
          Serial.println();
          
          // 分析数据帧内容
          if (receivedData[i + 1] == 0x03) {
            if (receivedData[i + 5] == 0x96) {
              Serial.println(">>> 收到配对请求，发送配对确认");
              // 发送配对确认
              byte pairingConfirm[] = {0xFD, 0x03, 0xD5, 0xAA, 0x55, 0x97, 0xDF};
              Serial2.write(pairingConfirm, 7);
              Serial2.flush();
              delay(1000);
              Serial.println(">>> 配对确认已发送");
            } else if (receivedData[i + 5] == 0x97) {
              Serial.println(">>> 收到配对确认，配对成功！");
              
              // 设置配对状态
              pairingState.isPaired = true;
              pairingState.deviceId[0] = 0xD5;
              pairingState.deviceId[1] = 0xAA;
              pairingState.deviceId[2] = 0x55;
              pairingState.deviceId[3] = 0x97;
              pairingState.lastHeartbeat = currentTime;
              pairingState.heartbeatCount = 0;
              
              // 保存配对状态到EEPROM
              savePairingState();
              
              Serial.println(">>> 配对状态已保存到EEPROM");
              Serial.println(">>> 退出配对模式，进入正常工作状态");
              
              pairingMode = false;
            }
          }
          break;
        }
      }
    }
  } else {
    // 正常工作模式
    if (pairingState.isPaired) {
      // 已配对状态 - 发送心跳和监听指令
      
      // 每5秒发送一次心跳
      if (currentTime - lastHeartbeatTime >= 5000) {
        Serial.println(">>> 发送心跳信号");
        
        byte heartbeat[] = {0xFD, 0x04, 0xD5, 0xAA, 0x55, 0x98, 0xDF};
        Serial2.write(heartbeat, 7);
        Serial2.flush();
        delay(1000);
        
        lastHeartbeatTime = currentTime;
        pairingState.heartbeatCount++;
        
        Serial.print(">>> 心跳计数: ");
        Serial.println(pairingState.heartbeatCount);
      }
      
      // 监听接收数据
      if (Serial2.available()) {
        Serial.print(">>> 已配对状态收到数据: ");
        
        // 读取所有可用数据
        byte receivedData[100];
        int dataCount = 0;
        while (Serial2.available() && dataCount < 100) {
          receivedData[dataCount] = Serial2.read();
          dataCount++;
        }
        
        // 显示接收到的数据
        for (int i = 0; i < dataCount; i++) {
          Serial.print("0x");
          if (receivedData[i] < 0x10) Serial.print("0");
          Serial.print(receivedData[i], HEX);
          Serial.print(" ");
        }
        Serial.print("(共");
        Serial.print(dataCount);
        Serial.println("字节)");
        
        // 检查数据帧
        for (int i = 0; i <= dataCount - 7; i++) {
          if (receivedData[i] == 0xFD && receivedData[i + 6] == 0xDF) {
            Serial.print(">>> 找到有效数据帧: ");
            for (int j = 0; j < 7; j++) {
              Serial.print("0x");
              if (receivedData[i + j] < 0x10) Serial.print("0");
              Serial.print(receivedData[i + j], HEX);
              Serial.print(" ");
            }
            Serial.println();
            
            // 分析数据帧内容
            if (receivedData[i + 1] == 0x04) {
              if (receivedData[i + 5] == 0x98) {
                Serial.println(">>> 收到心跳信号，更新心跳时间");
                pairingState.lastHeartbeat = currentTime;
              }
            } else if (receivedData[i + 1] == 0x05) {
              Serial.println(">>> 收到控制指令");
              // 这里可以添加具体的控制逻辑
            }
            break;
          }
        }
      }
      
      // 显示配对状态
      static unsigned long lastStatusTime = 0;
      if (currentTime - lastStatusTime >= 10000) {
        Serial.print(">>> 已配对状态，设备ID: ");
        for (int i = 0; i < 4; i++) {
          Serial.print("0x");
          if (pairingState.deviceId[i] < 0x10) Serial.print("0");
          Serial.print(pairingState.deviceId[i], HEX);
          Serial.print(" ");
        }
        Serial.print("，心跳计数: ");
        Serial.println(pairingState.heartbeatCount);
        lastStatusTime = currentTime;
      }
    } else {
      // 未配对状态 - 持续监听接收数据
      static unsigned long lastReceiveCheck = 0;
      if (currentTime - lastReceiveCheck >= 2000) {
        if (Serial2.available()) {
          Serial.print(">>> 未配对状态收到数据: ");
          while (Serial2.available()) {
            byte data = Serial2.read();
            Serial.print("0x");
            if (data < 0x10) Serial.print("0");
            Serial.print(data, HEX);
            Serial.print(" ");
          }
          Serial.println();
        }
        lastReceiveCheck = currentTime;
      }
      
      // 显示配对状态
      static unsigned long lastStatusTime = 0;
      if (currentTime - lastStatusTime >= 5000) {
        Serial.println(">>> 未配对状态，长按bt1键3秒进入配对模式");
        lastStatusTime = currentTime;
      }
    }
  }
}

// 完整初始化模块
void initializeModule() {
  // 方法1：发送重置命令
  Serial.println(">>> 方法1：发送重置命令...");
  for (int i = 0; i < 3; i++) {
    byte reset[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    Serial2.write(reset, 8);
    Serial2.flush();
    delay(500);
  }
  
  // 方法2：发送同步信号
  Serial.println(">>> 方法2：发送同步信号...");
  for (int i = 0; i < 5; i++) {
    byte sync[] = {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55};
    Serial2.write(sync, 8);
    Serial2.flush();
    delay(200);
  }
  
  // 方法3：发送配置命令
  Serial.println(">>> 方法3：发送配置命令...");
  byte config[] = {0xFD, 0x01, 0x00, 0x00, 0x00, 0x00, 0xDF};
  Serial2.write(config, 7);
  Serial2.flush();
  delay(1000);
  
  // 方法4：发送配对命令
  Serial.println(">>> 方法4：发送配对命令...");
  byte pair[] = {0xFD, 0x0E, 0x00, 0x00, 0x00, 0x00, 0xDF};
  Serial2.write(pair, 7);
  Serial2.flush();
  delay(1000);
  
  // 方法5：发送测试数据
  Serial.println(">>> 方法5：发送测试数据...");
  byte test[] = {0xFD, 0x03, 0xD5, 0xAA, 0x55, 0x96, 0xDF};
  Serial2.write(test, 7);
  Serial2.flush();
  delay(1000);
  
  Serial.println(">>> 模块完整初始化完成");
}

// 保存配对状态到EEPROM
void savePairingState() {
  EEPROM.write(0, pairingState.isPaired ? 1 : 0);
  for (int i = 0; i < 4; i++) {
    EEPROM.write(i + 1, pairingState.deviceId[i]);
  }
  EEPROM.write(5, pairingState.heartbeatCount & 0xFF);
  EEPROM.write(6, (pairingState.heartbeatCount >> 8) & 0xFF);
  EEPROM.write(7, (pairingState.heartbeatCount >> 16) & 0xFF);
  EEPROM.write(8, (pairingState.heartbeatCount >> 24) & 0xFF);
}

// 从EEPROM加载配对状态
void loadPairingState() {
  pairingState.isPaired = (EEPROM.read(0) == 1);
  for (int i = 0; i < 4; i++) {
    pairingState.deviceId[i] = EEPROM.read(i + 1);
  }
  pairingState.heartbeatCount = EEPROM.read(5) | 
                                (EEPROM.read(6) << 8) | 
                                (EEPROM.read(7) << 16) | 
                                (EEPROM.read(8) << 24);
  pairingState.lastHeartbeat = 0;
}
