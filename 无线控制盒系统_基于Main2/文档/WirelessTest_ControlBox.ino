/*
 * 无线控制盒通信测试程序
 * 用于验证433MHz通信和配对功能
 * 引脚配置：RX=28, TX=29
 */

#include <SoftwareSerial.h>

// 无线通信引脚配置
SoftwareSerial wirelessSerial(28, 29); // RX=28, TX=29

// 测试变量
unsigned long lastHeartbeat = 0;
const unsigned long HEARTBEAT_INTERVAL = 2000; // 2秒心跳
bool isPaired = false;

void setup() {
  // 初始化串口通信
  Serial.begin(115200);
  Serial.println("=== 无线控制盒通信测试 ===");
  
  // 初始化无线通信
  wirelessSerial.begin(9600);
  Serial.println("无线串口初始化完成");
  Serial.println("RX=28, TX=29");
}

void loop() {
  // 处理接收到的数据
  if (wirelessSerial.available()) {
    processReceivedData();
  }
  
  // 定期发送心跳包
  if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL) {
    sendHeartbeat();
    lastHeartbeat = millis();
  }
  
  delay(100);
}

// 发送配对确认
void sendPairingConfirm() {
  byte pairingData[] = {0xFD, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xDF};
  wirelessSerial.write(pairingData, 7);
  Serial.println("发送配对确认: FD 0F 00 00 0000 DF");
}

// 发送心跳包
void sendHeartbeat() {
  byte heartbeatData[] = {0xFD, 0x30, 0x00, 0x00, 0x00, 0x00, 0xDF};
  wirelessSerial.write(heartbeatData, 7);
  Serial.println("发送心跳包: FD 30 00 00 0000 DF");
}

// 处理接收到的数据
void processReceivedData() {
  byte data[7];
  int bytesRead = 0;
  
  // 读取数据
  while (wirelessSerial.available() && bytesRead < 7) {
    data[bytesRead] = wirelessSerial.read();
    bytesRead++;
  }
  
  // 检查数据格式
  if (bytesRead == 7 && data[0] == 0xFD && data[6] == 0xDF) {
    Serial.print("接收到数据: ");
    for (int i = 0; i < 7; i++) {
      Serial.print(data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    
    // 处理配对请求
    if (data[1] == 0x0E) {
      Serial.println("收到配对请求，发送确认...");
      sendPairingConfirm();
      isPaired = true;
    }
    
    // 处理心跳包
    if (data[1] == 0x30) {
      Serial.println("收到心跳包");
    }
  }
}
