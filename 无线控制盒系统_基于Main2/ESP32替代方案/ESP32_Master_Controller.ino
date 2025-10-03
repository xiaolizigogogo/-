/*
 * ESP32 主控制器
 * 替代nRF24L01主控制器，通过WiFi或蓝牙与Mega 2560通信
 * 支持Web界面控制和移动端控制
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

// WiFi配置
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";

// Web服务器
WebServer server(80);

// 与Mega 2560的串口通信
SoftwareSerial megaSerial(16, 17); // RX=16, TX=17

// 控制状态
struct ControlState {
  bool motor1_enabled = false;
  bool motor2_enabled = false;
  int motor1_speed = 0;
  int motor2_speed = 0;
  bool led_status = false;
  int temperature = 0;
  int humidity = 0;
} controlState;

// 设备状态
struct DeviceStatus {
  bool connected = false;
  unsigned long lastHeartbeat = 0;
  String deviceName = "Mega2560_Device";
} deviceStatus;

void setup() {
  Serial.begin(115200);
  megaSerial.begin(9600);
  
  Serial.println("=== ESP32 主控制器启动 ===");
  
  // 初始化WiFi
  initWiFi();
  
  // 设置Web服务器路由
  setupWebServer();
  
  // 启动Web服务器
  server.begin();
  Serial.println("Web服务器已启动");
  Serial.println("访问地址: http://" + WiFi.localIP().toString());
  
  // 发送初始化命令到Mega 2560
  sendToMega("INIT");
  
  Serial.println("ESP32主控制器初始化完成");
}

void loop() {
  // 处理Web请求
  server.handleClient();
  
  // 处理来自Mega 2560的数据
  handleMegaData();
  
  // 发送心跳
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 5000) {
    lastHeartbeat = millis();
    sendHeartbeat();
  }
  
  delay(10);
}

void initWiFi() {
  Serial.println("正在连接WiFi...");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi连接成功！");
    Serial.print("IP地址: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("WiFi连接失败，使用AP模式");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32_Controller", "12345678");
    Serial.print("AP IP地址: ");
    Serial.println(WiFi.softAPIP());
  }
}

void setupWebServer() {
  // 主页面
  server.on("/", handleRoot);
  
  // API接口
  server.on("/api/status", HTTP_GET, handleGetStatus);
  server.on("/api/control", HTTP_POST, handleControl);
  server.on("/api/motor", HTTP_POST, handleMotorControl);
  server.on("/api/led", HTTP_POST, handleLedControl);
  
  // 处理OPTIONS请求（CORS）
  server.onNotFound([]() {
    if (server.method() == HTTP_OPTIONS) {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
      server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
      server.send(200, "text/plain", "");
    } else {
      server.send(404, "text/plain", "Not Found");
    }
  });
}

void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 无线控制器</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .status { background: #e8f5e8; padding: 15px; border-radius: 5px; margin: 10px 0; }
        .control-group { margin: 20px 0; padding: 15px; border: 1px solid #ddd; border-radius: 5px; }
        button { padding: 10px 20px; margin: 5px; border: none; border-radius: 5px; cursor: pointer; }
        .btn-primary { background: #007bff; color: white; }
        .btn-success { background: #28a745; color: white; }
        .btn-danger { background: #dc3545; color: white; }
        .btn-warning { background: #ffc107; color: black; }
        input[type="range"] { width: 100%; margin: 10px 0; }
        .status-indicator { display: inline-block; width: 12px; height: 12px; border-radius: 50%; margin-right: 8px; }
        .status-online { background: #28a745; }
        .status-offline { background: #dc3545; }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32 无线控制器</h1>
        
        <div class="status">
            <h3>设备状态</h3>
            <p><span id="connectionStatus" class="status-indicator status-offline"></span>
            <span id="deviceName">Mega2560_Device</span></p>
            <p>温度: <span id="temperature">--</span>°C</p>
            <p>湿度: <span id="humidity">--</span>%</p>
        </div>
        
        <div class="control-group">
            <h3>电机控制</h3>
            <div>
                <label>电机1: </label>
                <button id="motor1On" class="btn-success">开启</button>
                <button id="motor1Off" class="btn-danger">关闭</button>
                <input type="range" id="motor1Speed" min="0" max="255" value="0">
                <span id="motor1SpeedValue">0</span>
            </div>
            <div>
                <label>电机2: </label>
                <button id="motor2On" class="btn-success">开启</button>
                <button id="motor2Off" class="btn-danger">关闭</button>
                <input type="range" id="motor2Speed" min="0" max="255" value="0">
                <span id="motor2SpeedValue">0</span>
            </div>
        </div>
        
        <div class="control-group">
            <h3>LED控制</h3>
            <button id="ledToggle" class="btn-warning">切换LED</button>
            <span id="ledStatus">关闭</span>
        </div>
        
        <div class="control-group">
            <h3>系统控制</h3>
            <button id="emergencyStop" class="btn-danger">紧急停止</button>
            <button id="resetSystem" class="btn-primary">重置系统</button>
        </div>
    </div>
    
    <script>
        // 更新状态显示
        function updateStatus() {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('temperature').textContent = data.temperature;
                    document.getElementById('humidity').textContent = data.humidity;
                    document.getElementById('deviceName').textContent = data.deviceName;
                    
                    const statusIndicator = document.getElementById('connectionStatus');
                    if (data.connected) {
                        statusIndicator.className = 'status-indicator status-online';
                    } else {
                        statusIndicator.className = 'status-indicator status-offline';
                    }
                    
                    document.getElementById('motor1Speed').value = data.motor1_speed;
                    document.getElementById('motor1SpeedValue').textContent = data.motor1_speed;
                    document.getElementById('motor2Speed').value = data.motor2_speed;
                    document.getElementById('motor2SpeedValue').textContent = data.motor2_speed;
                    
                    document.getElementById('ledStatus').textContent = data.led_status ? '开启' : '关闭';
                })
                .catch(error => console.error('Error:', error));
        }
        
        // 发送控制命令
        function sendCommand(endpoint, data) {
            fetch(endpoint, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(data)
            })
            .then(response => response.json())
            .then(result => {
                console.log('Success:', result);
                updateStatus();
            })
            .catch(error => console.error('Error:', error));
        }
        
        // 事件监听器
        document.getElementById('motor1On').onclick = () => sendCommand('/api/motor', {motor: 1, action: 'on'});
        document.getElementById('motor1Off').onclick = () => sendCommand('/api/motor', {motor: 1, action: 'off'});
        document.getElementById('motor2On').onclick = () => sendCommand('/api/motor', {motor: 2, action: 'on'});
        document.getElementById('motor2Off').onclick = () => sendCommand('/api/motor', {motor: 2, action: 'off'});
        
        document.getElementById('motor1Speed').oninput = function() {
            document.getElementById('motor1SpeedValue').textContent = this.value;
            sendCommand('/api/motor', {motor: 1, speed: parseInt(this.value)});
        };
        
        document.getElementById('motor2Speed').oninput = function() {
            document.getElementById('motor2SpeedValue').textContent = this.value;
            sendCommand('/api/motor', {motor: 2, speed: parseInt(this.value)});
        };
        
        document.getElementById('ledToggle').onclick = () => sendCommand('/api/led', {action: 'toggle'});
        document.getElementById('emergencyStop').onclick = () => sendCommand('/api/control', {action: 'emergency_stop'});
        document.getElementById('resetSystem').onclick = () => sendCommand('/api/control', {action: 'reset'});
        
        // 定期更新状态
        setInterval(updateStatus, 2000);
        updateStatus();
    </script>
</body>
</html>
)";
  
  server.send(200, "text/html", html);
}

void handleGetStatus() {
  StaticJsonDocument<200> doc;
  doc["connected"] = deviceStatus.connected;
  doc["deviceName"] = deviceStatus.deviceName;
  doc["temperature"] = controlState.temperature;
  doc["humidity"] = controlState.humidity;
  doc["motor1_enabled"] = controlState.motor1_enabled;
  doc["motor1_speed"] = controlState.motor1_speed;
  doc["motor2_enabled"] = controlState.motor2_enabled;
  doc["motor2_speed"] = controlState.motor2_speed;
  doc["led_status"] = controlState.led_status;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleControl() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
    return;
  }
  
  StaticJsonDocument<200> doc;
  deserializeJson(doc, server.arg("plain"));
  
  String action = doc["action"];
  
  if (action == "emergency_stop") {
    sendToMega("EMERGENCY_STOP");
    controlState.motor1_enabled = false;
    controlState.motor2_enabled = false;
    controlState.motor1_speed = 0;
    controlState.motor2_speed = 0;
  } else if (action == "reset") {
    sendToMega("RESET");
    // 重置所有状态
    controlState = ControlState();
  }
  
  server.send(200, "application/json", "{\"status\":\"success\"}");
}

void handleMotorControl() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
    return;
  }
  
  StaticJsonDocument<200> doc;
  deserializeJson(doc, server.arg("plain"));
  
  int motor = doc["motor"];
  String action = doc["action"];
  int speed = doc["speed"];
  
  String command = "MOTOR_" + String(motor) + "_";
  
  if (action == "on") {
    command += "ON";
    if (motor == 1) controlState.motor1_enabled = true;
    else controlState.motor2_enabled = true;
  } else if (action == "off") {
    command += "OFF";
    if (motor == 1) {
      controlState.motor1_enabled = false;
      controlState.motor1_speed = 0;
    } else {
      controlState.motor2_enabled = false;
      controlState.motor2_speed = 0;
    }
  } else if (speed >= 0) {
    command += "SPEED_" + String(speed);
    if (motor == 1) controlState.motor1_speed = speed;
    else controlState.motor2_speed = speed;
  }
  
  sendToMega(command);
  server.send(200, "application/json", "{\"status\":\"success\"}");
}

void handleLedControl() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
    return;
  }
  
  StaticJsonDocument<200> doc;
  deserializeJson(doc, server.arg("plain"));
  
  String action = doc["action"];
  
  if (action == "toggle") {
    controlState.led_status = !controlState.led_status;
    sendToMega("LED_" + String(controlState.led_status ? "ON" : "OFF"));
  }
  
  server.send(200, "application/json", "{\"status\":\"success\"}");
}

void sendToMega(String command) {
  megaSerial.println(command);
  Serial.println("发送到Mega 2560: " + command);
}

void handleMegaData() {
  if (megaSerial.available()) {
    String data = megaSerial.readStringUntil('\n');
    data.trim();
    
    Serial.println("收到Mega 2560数据: " + data);
    
    // 解析数据
    if (data.startsWith("STATUS:")) {
      parseStatusData(data);
    } else if (data.startsWith("HEARTBEAT")) {
      deviceStatus.connected = true;
      deviceStatus.lastHeartbeat = millis();
    }
  }
  
  // 检查连接状态
  if (millis() - deviceStatus.lastHeartbeat > 10000) {
    deviceStatus.connected = false;
  }
}

void parseStatusData(String data) {
  // 解析格式: STATUS:TEMP:25:HUM:60:MOTOR1:ON:SPEED1:128:MOTOR2:OFF:SPEED2:0:LED:ON
  int tempIndex = data.indexOf("TEMP:");
  int humIndex = data.indexOf("HUM:");
  int motor1Index = data.indexOf("MOTOR1:");
  int speed1Index = data.indexOf("SPEED1:");
  int motor2Index = data.indexOf("MOTOR2:");
  int speed2Index = data.indexOf("SPEED2:");
  int ledIndex = data.indexOf("LED:");
  
  if (tempIndex != -1) {
    controlState.temperature = data.substring(tempIndex + 5, data.indexOf(":", tempIndex + 5)).toInt();
  }
  
  if (humIndex != -1) {
    controlState.humidity = data.substring(humIndex + 4, data.indexOf(":", humIndex + 4)).toInt();
  }
  
  if (motor1Index != -1) {
    String motor1State = data.substring(motor1Index + 7, data.indexOf(":", motor1Index + 7));
    controlState.motor1_enabled = (motor1State == "ON");
  }
  
  if (speed1Index != -1) {
    controlState.motor1_speed = data.substring(speed1Index + 7, data.indexOf(":", speed1Index + 7)).toInt();
  }
  
  if (motor2Index != -1) {
    String motor2State = data.substring(motor2Index + 7, data.indexOf(":", motor2Index + 7));
    controlState.motor2_enabled = (motor2State == "ON");
  }
  
  if (speed2Index != -1) {
    controlState.motor2_speed = data.substring(speed2Index + 7, data.indexOf(":", speed2Index + 7)).toInt();
  }
  
  if (ledIndex != -1) {
    String ledState = data.substring(ledIndex + 4);
    controlState.led_status = (ledState == "ON");
  }
}

void sendHeartbeat() {
  sendToMega("HEARTBEAT");
}





