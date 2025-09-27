# RCSwitch库安装指南

## 概述

RCSwitch是一个用于Arduino的433MHz无线通信库，支持发送和接收433MHz无线信号。本指南将帮助您安装和配置RCSwitch库。

## 安装方法

### 方法1：通过Arduino IDE库管理器安装（推荐）

#### 步骤1：打开Arduino IDE
1. 启动Arduino IDE
2. 确保Arduino IDE版本为1.6.0或更高版本

#### 步骤2：打开库管理器
1. 在Arduino IDE中，选择 `工具` → `管理库...`
2. 或者选择 `项目` → `加载库` → `管理库...`

#### 步骤3：搜索RCSwitch
1. 在库管理器的搜索框中输入 `RCSwitch`
2. 找到 `RCSwitch` 库（作者：Suat Özgür）
3. 点击 `安装` 按钮

#### 步骤4：确认安装
1. 等待安装完成
2. 安装成功后，库会显示为 `已安装` 状态

### 方法2：手动安装

#### 步骤1：下载RCSwitch库
1. 访问GitHub：https://github.com/sui77/rc-switch
2. 点击 `Code` 按钮，选择 `Download ZIP`
3. 或者直接访问：https://github.com/sui77/rc-switch/archive/refs/heads/master.zip

#### 步骤2：解压文件
1. 下载完成后，解压ZIP文件
2. 将解压后的文件夹重命名为 `RCSwitch`

#### 步骤3：复制到Arduino库文件夹
1. 找到Arduino库文件夹位置：
   - **Windows**：`C:\Users\[用户名]\Documents\Arduino\libraries\`
   - **macOS**：`~/Documents/Arduino/libraries/`
   - **Linux**：`~/Arduino/libraries/`

2. 将 `RCSwitch` 文件夹复制到库文件夹中

#### 步骤4：重启Arduino IDE
1. 关闭Arduino IDE
2. 重新打开Arduino IDE
3. 检查库是否已安装：`项目` → `加载库` → 查看是否有 `RCSwitch`

## 验证安装

### 步骤1：创建测试项目
1. 在Arduino IDE中，选择 `文件` → `新建`
2. 输入以下测试代码：

```cpp
#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

void setup() {
  Serial.begin(9600);
  
  // 初始化发送器（连接到引脚10）
  mySwitch.enableTransmit(10);
  
  // 初始化接收器（连接到引脚2）
  mySwitch.enableReceive(0);  // 使用中断引脚0
  
  Serial.println("RCSwitch库测试程序");
}

void loop() {
  // 发送测试信号
  mySwitch.send(12345, 24);
  Serial.println("发送信号: 12345");
  delay(1000);
  
  // 接收信号
  if (mySwitch.available()) {
    Serial.print("收到信号: ");
    Serial.println(mySwitch.getReceivedValue());
    mySwitch.resetAvailable();
  }
}
```

### 步骤2：编译测试
1. 点击 `验证` 按钮
2. 如果编译成功，说明RCSwitch库安装正确
3. 如果出现错误，请检查库是否正确安装

## 常见问题

### 问题1：找不到RCSwitch库
**解决方案**：
1. 确认库已正确安装
2. 重启Arduino IDE
3. 检查库文件夹路径是否正确

### 问题2：编译错误
**解决方案**：
1. 检查Arduino IDE版本是否兼容
2. 确认库版本是否正确
3. 检查代码语法

### 问题3：库版本不兼容
**解决方案**：
1. 卸载旧版本库
2. 重新安装最新版本
3. 检查代码兼容性

## 库文件结构

安装完成后，RCSwitch库的文件结构如下：

```
Arduino/libraries/RCSwitch/
├── RCSwitch.cpp
├── RCSwitch.h
├── keywords.txt
├── library.properties
└── examples/
    ├── ReceiveDemo_Simple/
    ├── SendDemo/
    └── ...
```

## 主要功能

### 发送功能
```cpp
// 初始化发送器
mySwitch.enableTransmit(10);  // 连接到引脚10

// 发送数据
mySwitch.send(12345, 24);     // 发送24位数据
```

### 接收功能
```cpp
// 初始化接收器
mySwitch.enableReceive(0);    // 使用中断引脚0

// 接收数据
if (mySwitch.available()) {
  unsigned long value = mySwitch.getReceivedValue();
  mySwitch.resetAvailable();
}
```

## 版本信息

- **当前版本**：2.6.3
- **作者**：Suat Özgür
- **许可证**：GPL v2
- **兼容性**：Arduino 1.6.0+

## 技术支持

如果遇到问题，请：

1. 查看官方文档：https://github.com/sui77/rc-switch
2. 检查Arduino IDE版本兼容性
3. 确认硬件连接正确
4. 查看串口输出的错误信息

## 注意事项

1. **引脚选择**：
   - 发送引脚：可以使用任意数字引脚
   - 接收引脚：建议使用中断引脚（0、1、2、3）

2. **电源要求**：
   - 确保433MHz模块供电稳定
   - 推荐使用5V电源

3. **天线连接**：
   - 连接天线可以提高传输距离
   - 天线长度建议17.3cm

4. **干扰避免**：
   - 避免与其他433MHz设备干扰
   - 保持适当的设备距离
