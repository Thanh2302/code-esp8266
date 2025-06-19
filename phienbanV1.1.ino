#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define DHTPIN D4
#define BUTTON1_PIN D2
#define BUTTON2_PIN D3
#define BUTTON3_PIN D7
#define MODE_BUTTON_PIN D8
#define DHTTYPE DHT11
#define RELAY_PIN D1
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
ESP8266WebServer server(80);

const char* ssid = "QTT";
const char* password = "wifiDihoilamchi";
bool relayState = false;
bool btn1State = false;
bool btn2State = false;
bool btn3State = false;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000);

float lastTemp = 0.0;
float lastHum = 0.0;
unsigned long lastSensorUpdate = 0;
unsigned long lastOLEDUpdate = 0;
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 500; // chống dội 500ms
const unsigned long sensorInterval = 2000;
const unsigned long oledInterval = 2000;

void setup() {
  SPIFFS.begin();
  server.serveStatic("/space.jpg", SPIFFS, "/space.jpg");
  Serial.begin(115200);
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUTTON1_PIN, OUTPUT);
  pinMode(BUTTON2_PIN, OUTPUT);
  pinMode(BUTTON3_PIN, OUTPUT);
  digitalWrite(BUTTON1_PIN, LOW);
  digitalWrite(BUTTON2_PIN, LOW);
  digitalWrite(BUTTON3_PIN, LOW);
  digitalWrite(RELAY_PIN, LOW);
  server.on("/toggleMode", handleToggleMode);
  server.on("/btn1", handleBtn1);
  server.on("/btn2", handleBtn2);
  server.on("/btn3", handleBtn3);
  dht.begin();
  Wire.begin(14, 12);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("Dang ket noi WiFi..."));
  display.display();

  WiFi.begin(ssid, password);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("Chuyen sang che do AP"));
    WiFi.softAP("ESP8266_DHT11", "12345678");
  }

  IPAddress ip = WiFi.isConnected() ? WiFi.localIP() : WiFi.softAPIP();
  Serial.print(F("IP address: "));
  Serial.println(ip);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.print(F("Che do: "));
  display.println(WiFi.isConnected() ? F("STA") : F("AP"));
  display.setCursor(0, 10);
  display.print(F("IP: "));
  display.println(ip);
  display.display();
  delay(1000);

  timeClient.begin();

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/toggle", handleToggle);
  server.begin();
  Serial.println(F("Web server started"));
}

void loop() {
  server.handleClient();
  timeClient.update();

  unsigned long currentMillis = millis();

  if (currentMillis - lastSensorUpdate >= sensorInterval) {
    lastSensorUpdate = currentMillis;
    readSensor();  // cập nhật lastTemp và lastHum
  }

  if (currentMillis - lastOLEDUpdate >= oledInterval) {
    lastOLEDUpdate = currentMillis;
    showToOLED();
  }
}

void handleToggleMode() {
  // Nếu đang ở chế độ STA, chuyển sang AP và ngược lại
  if (WiFi.getMode() == WIFI_STA) {
    Serial.println("Chuyen sang AP mode");
    WiFi.disconnect();
    delay(1000);
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP8266_AP", "12345678");
  } else {
    Serial.println("Chuyen sang STA mode");
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
  }
  server.send(200, "text/plain", "Chuyen che do ket noi");
}


void readSensor() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) lastTemp = t;
  if (!isnan(h)) lastHum = h;
}

void handleBtn1() {
  btn1State = !btn1State;
  digitalWrite(BUTTON1_PIN, btn1State);
  server.send(200, "text/plain", "BTN1 TOGGLED");
}

void handleBtn2() {
  btn2State = !btn2State;
  digitalWrite(BUTTON2_PIN, btn2State);
  server.send(200, "text/plain", "BTN2 TOGGLED");
}

void handleBtn3() {
  btn3State = !btn3State;
  digitalWrite(BUTTON3_PIN, btn3State);
  server.send(200, "text/plain", "BTN3 TOGGLED");
}


void showToOLED() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(WiFi.isConnected() ? F("STA") : F("AP"));
  display.print(F(" | IP: "));
  display.println(WiFi.isConnected() ? WiFi.localIP() : WiFi.softAPIP());

  display.setCursor(0, 12);
  display.print(F("Time: "));
  display.println(timeClient.getFormattedTime());

  display.setCursor(0, 24);
  display.setTextSize(1);
  display.println(F("Nhiet do:"));
  display.setTextSize(2);
  display.setCursor(0, 34);
  display.print(lastTemp, 1);
  display.print(F(" C"));

  display.setTextSize(1);
  display.setCursor(80, 34);
  display.println(F("Do am:"));
  display.setCursor(80, 44);
  display.print(lastHum, 1);
  display.print(F(" %"));

  display.display();
}
void handleRoot() {
  String html = F(R"====(
  <!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <title>ESP8266 DHT11 Dashboard</title>
  <style>
    body {
      margin: 0;
      font-family: 'Segoe UI', sans-serif;
      background: linear-gradient(to right, #0f2027, #203a43, #2c5364);
      color: #ffffff;
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
    }

    .container {
      background: rgba(0, 0, 0, 0.6);
      border-radius: 20px;
      padding: 40px;
      max-width: 700px;
      width: 100%;
      box-shadow: 0 10px 30px rgba(0, 0, 0, 0.6);
      backdrop-filter: blur(10px);
    }

    .container h2 {
      text-align: center;
      font-size: 2em;
      margin-bottom: 30px;
      color: #00eaff;
      text-shadow: 0 0 10px #00eaff;
    }

    .data-row {
      display: flex;
      justify-content: space-between;
      margin-bottom: 15px;
      font-size: 1.2em;
      padding: 10px;
      border-bottom: 1px solid rgba(255, 255, 255, 0.1);
    }

    .data-row span {
      font-weight: bold;
      color: #ffd700;
    }

    .buttons {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
      gap: 12px;
      margin-top: 30px;
    }

    button {
      padding: 12px;
      font-size: 1em;
      font-weight: bold;
      background: #1e88e5;
      border: none;
      border-radius: 10px;
      color: white;
      cursor: pointer;
      transition: all 0.2s ease;
      box-shadow: 0 5px 15px rgba(0, 0, 0, 0.3);
    }

    button:hover {
      background: #1565c0;
      transform: translateY(-2px);
    }

    button:active {
      transform: scale(0.98);
    }
  </style>

  <script>
    function updateData() {
      fetch('/data')
        .then(response => response.json())
        .then(data => {
          document.getElementById('temp').innerText = data.temp + " °C";
          document.getElementById('hum').innerText = data.hum + " %";
          document.getElementById('relay').innerText = data.relay ? "BẬT" : "TẮT";
          document.getElementById('btn1').innerText = data.btn1 ? "BẬT" : "TẮT";
          document.getElementById('btn2').innerText = data.btn2 ? "BẬT" : "TẮT";
          document.getElementById('btn3').innerText = data.btn3 ? "BẬT" : "TẮT";
          document.getElementById('netmode').innerText = data.mode;
        });
    }

    setInterval(updateData, 2000);
    window.onload = updateData;

    function toggleRelay() { fetch('/toggle'); }
  </script>
</head>
<body>
  <div class="container">
    <h2>Bảng điều khiển thiết bị</h2>

    <div class="data-row">Nhiệt độ: <span id="temp">-- °C</span></div>
    <div class="data-row">Độ ẩm: <span id="hum">-- %</span></div>
    <div class="data-row">Đèn: <span id="btn1">--</span></div>
    <div class="data-row">Quạt: <span id="btn2">--</span></div>
    <div class="data-row">Cảm biến: <span id="btn3">--</span></div>
    <div class="data-row">Relay chính: <span id="relay">--</span></div>
    <div class="data-row">Chế độ mạng: <span id="netmode">--</span></div>

    <div class="buttons">
      <button onclick="fetch('/toggleMode')">🔄 Chuyển chế độ kết nối</button>
      <button onclick="toggleRelay()">⚡ Bật / Tắt Relay chính</button>
      <button onclick="fetch('/btn1')">💡 Bật / Tắt Đèn</button>
      <button onclick="fetch('/btn2')">🌀 Bật / Tắt Quạt</button>
      <button onclick="fetch('/btn3')">♻️ Reset cảm biến</button>
    </div>
  </div>
</body>
</html>

 )====");
    server.send(200, "text/html", html);
}

void handleData() {
  String json = "{";
  json += "\"mode\":\"" + String(WiFi.getMode() == WIFI_STA ? "STA" : "AP") + "\",";
  json += "\"temp\":" + String(lastTemp, 1) + ",";
  json += "\"hum\":" + String(lastHum, 1) + ",";
  json += "\"relay\":" + String(relayState ? "true" : "false") + ",";
  json += "\"btn1\":" + String(btn1State ? "true" : "false") + ",";
  json += "\"btn2\":" + String(btn2State ? "true" : "false") + ",";
  json += "\"btn3\":" + String(btn3State ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

void handleToggle() {
  relayState = !relayState;
  digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
  Serial.println(relayState ? F("RELAY: ON") : F("RELAY: OFF"));
  server.send(200, "text/plain", relayState ? "ON" : "OFF");
}
