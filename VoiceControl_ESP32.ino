/*
 * ESP32 语音控制Arduino示例
 * 支持语音指令控制LED、舵机、电机等设备
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <driver/i2s.h>

// 硬件定义
#define LED_PIN 2
#define SERVO_PIN 13
#define MOTOR_PIN1 14
#define MOTOR_PIN2 15
#define MIC_PIN 34

// WiFi配置
const char* ssid = "您的WiFi名称";
const char* password = "您的WiFi密码";

// 百度AI语音识别配置
const String API_KEY = "您的百度AI API_KEY";
const String SECRET_KEY = "您的百度AI SECRET_KEY";
String access_token = "";

// 语音指令映射
struct VoiceCommand {
  String keyword;
  void (*action)();
  String description;
};

// 全局变量
bool isListening = false;
String lastCommand = "";

// 函数声明
void setupWiFi();
void getAccessToken();
String recordAudio();
String speechToText(String audioData);
void processVoiceCommand(String command);

// 语音指令动作函数
void turnOnLED() {
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED已开启");
}

void turnOffLED() {
  digitalWrite(LED_PIN, LOW);
  Serial.println("LED已关闭");
}

void servoRotate() {
  // 舵机旋转90度
  analogWrite(SERVO_PIN, 90);
  Serial.println("舵机已旋转");
}

void motorForward() {
  digitalWrite(MOTOR_PIN1, HIGH);
  digitalWrite(MOTOR_PIN2, LOW);
  Serial.println("电机正转");
}

void motorStop() {
  digitalWrite(MOTOR_PIN1, LOW);
  digitalWrite(MOTOR_PIN2, LOW);
  Serial.println("电机停止");
}

// 语音指令映射表
VoiceCommand commands[] = {
  {"开灯", turnOnLED, "开启LED"},
  {"关灯", turnOffLED, "关闭LED"},
  {"旋转", servoRotate, "舵机旋转"},
  {"前进", motorForward, "电机正转"},
  {"停止", motorStop, "电机停止"}
};

void setup() {
  Serial.begin(115200);
  
  // 初始化引脚
  pinMode(LED_PIN, OUTPUT);
  pinMode(SERVO_PIN, OUTPUT);
  pinMode(MOTOR_PIN1, OUTPUT);
  pinMode(MOTOR_PIN2, OUTPUT);
  
  // 初始化I2S麦克风
  setupI2S();
  
  // 连接WiFi
  setupWiFi();
  
  // 获取百度AI访问令牌
  getAccessToken();
  
  Serial.println("语音控制系统已启动");
  Serial.println("支持的指令：开灯、关灯、旋转、前进、停止");
}

void loop() {
  // 检测语音输入
  if (analogRead(MIC_PIN) > 2000) { // 检测到声音
    Serial.println("检测到语音输入，开始录音...");
    
    // 录音3秒
    String audioData = recordAudio();
    
    if (audioData.length() > 0) {
      // 语音转文字
      String text = speechToText(audioData);
      
      if (text.length() > 0) {
        Serial.println("识别结果: " + text);
        processVoiceCommand(text);
      }
    }
    
    delay(1000); // 避免重复触发
  }
  
  delay(100);
}

void setupWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("连接WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi连接成功");
  Serial.print("IP地址: ");
  Serial.println(WiFi.localIP());
}

void getAccessToken() {
  HTTPClient http;
  String url = "https://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials&client_id=" + API_KEY + "&client_secret=" + SECRET_KEY;
  
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    access_token = doc["access_token"].as<String>();
    Serial.println("获取访问令牌成功");
  } else {
    Serial.println("获取访问令牌失败");
  }
  
  http.end();
}

String recordAudio() {
  // 这里实现录音功能
  // 使用I2S接口从麦克风读取音频数据
  // 返回Base64编码的音频数据
  
  // 简化实现，实际需要配置I2S参数
  Serial.println("录音功能需要根据具体硬件实现");
  return "";
}

String speechToText(String audioData) {
  if (access_token.length() == 0) {
    Serial.println("访问令牌无效");
    return "";
  }
  
  HTTPClient http;
  String url = "https://vop.baidu.com/server_api";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  // 构建请求JSON
  String postData = "{\"format\":\"pcm\",\"rate\":16000,\"channel\":1,\"token\":\"" + access_token + "\",\"speech\":\"" + audioData + "\",\"len\":" + String(audioData.length()) + "}";
  
  int httpCode = http.POST(postData);
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, payload);
    
    if (doc["err_no"] == 0) {
      String result = doc["result"][0].as<String>();
      return result;
    } else {
      Serial.println("语音识别失败: " + String(doc["err_no"].as<int>()));
    }
  }
  
  http.end();
  return "";
}

void processVoiceCommand(String command) {
  for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
    if (command.indexOf(commands[i].keyword) != -1) {
      Serial.println("执行指令: " + commands[i].description);
      commands[i].action();
      return;
    }
  }
  
  Serial.println("未识别的指令: " + command);
}

void setupI2S() {
  // I2S配置
  const i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };
  
  const i2s_pin_config_t pin_config = {
    .bck_io_num = 26,    // BCLK
    .ws_io_num = 25,     // LRCL
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = 34    // SD
  };
  
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
} 