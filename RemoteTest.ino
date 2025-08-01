/*
 * 遥控器按键代码测试程序
 * 用于获取433MHz遥控器的按键代码
 */

#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

// 按键代码记录
long buttonCodes[12] = {0}; // 最多记录12个按键
int buttonCount = 0;

void setup() {
  Serial.begin(9600);
  
  // 初始化无线接收器
  mySwitch.enableReceive(0); // 使用中断引脚0
  
  Serial.println("=== 遥控器按键代码测试程序 ===");
  Serial.println("请按下遥控器按键，程序将显示按键代码");
  Serial.println("按下的按键代码将自动记录");
  Serial.println("=====================================");
}

void loop() {
  if (mySwitch.available()) {
    long value = mySwitch.getReceivedValue();
    
    Serial.print("收到按键代码: ");
    Serial.println(value);
    
    // 检查是否是新按键
    bool isNewButton = true;
    for (int i = 0; i < buttonCount; i++) {
      if (buttonCodes[i] == value) {
        isNewButton = false;
        break;
      }
    }
    
    // 如果是新按键，记录下来
    if (isNewButton && buttonCount < 12) {
      buttonCodes[buttonCount] = value;
      buttonCount++;
      Serial.print("新按键已记录！当前已记录 ");
      Serial.print(buttonCount);
      Serial.println(" 个按键");
    }
    
    // 显示所有已记录的按键
    if (buttonCount > 0) {
      Serial.println("已记录的按键代码：");
      for (int i = 0; i < buttonCount; i++) {
        Serial.print("按键");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.println(buttonCodes[i]);
      }
      Serial.println("------------------------");
    }
    
    mySwitch.resetAvailable();
    
    // 防抖延时
    delay(200);
  }
  
  delay(10);
}

// 生成代码映射函数
void generateCodeMapping() {
  Serial.println("\n=== 代码映射建议 ===");
  Serial.println("请将以下代码复制到您的主程序中：");
  Serial.println();
  
  Serial.println("enum RemoteCommands {");
  Serial.println("  REMOTE_NONE = 0,");
  
  for (int i = 0; i < buttonCount; i++) {
    Serial.print("  REMOTE_BUTTON");
    Serial.print(i + 1);
    Serial.print(" = ");
    Serial.print(buttonCodes[i]);
    Serial.println(",");
  }
  
  Serial.println("};");
  Serial.println();
  
  Serial.println("// 在 processRemoteCommand 函数中添加：");
  for (int i = 0; i < buttonCount; i++) {
    Serial.print("    case REMOTE_BUTTON");
    Serial.print(i + 1);
    Serial.println(":");
    Serial.print("      // 处理按键");
    Serial.print(i + 1);
    Serial.println(" 的功能");
    Serial.println("      break;");
  }
} 