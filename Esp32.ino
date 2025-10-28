#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WebServer.h>

// ====== WiFi Credentials ======
const char* WIFI_SSID = "PARADOX";
const char* WIFI_PASS = "shouryakumar";

// ====== Backend URL ======
const char* BACKEND_URL = "http://172.20.10.3:3000/api/data/latest";

// ====== Pin Definitions ======
#define LDR_PIN 34
#define LOAD_PIN 26   // Relay controlling the single bulb
#define PWM_PIN 19    // PWM pin for brightness control (same bulb)

// ====== Variables ======
int brightness = 0;           // 0–255
String bulbState = "OFF";     // ON/OFF state

// ====== LCD Setup ======
LiquidCrystal_I2C lcd(0x27, 16, 2);
WebServer server(80);

// ====== Function Declarations ======
void connectWiFi();
void sendData();
void handleSetBulb();
void handleSetBrightness();

// ====== Setup ======
void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MASTERS©");

  // Relay and PWM setup
  pinMode(LOAD_PIN, OUTPUT);
  digitalWrite(LOAD_PIN, LOW); // Bulb OFF initially

  ledcSetup(0, 5000, 8); // PWM channel 0, 5kHz, 8-bit
  ledcAttachPin(PWM_PIN, 0);
  ledcWrite(0, 0);

  connectWiFi();

  // Web server routes
  server.on("/setBulb", handleSetBulb);
  server.on("/setBrightness", handleSetBrightness);
  server.on("/ping", []() {
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  });
  server.begin();
  Serial.println("Local control server started!");
}

// ====== WiFi Connection ======
void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// ====== Send Sensor Data ======
void sendData() {
  int rawValue = analogRead(LDR_PIN);
  float percentage = (rawValue / 4095.0) * 100;
  float voltage = (percentage / 100.0) * 12.0;
  float current = voltage / 220.0;
  float power = voltage * current;

  String status;
  if (voltage >= 4.0) status = "OK";
  else if (voltage >= 2.0) status = "LOW";
  else status = "CRITICAL";

  String jsonData = "{";
  jsonData += "\"percentage\":" + String(percentage, 2) + ",";
  jsonData += "\"voltage\":" + String(voltage, 3) + ",";
  jsonData += "\"current\":" + String(current, 3) + ",";
  jsonData += "\"power\":" + String(power, 3) + ",";
  jsonData += "\"status\":\"" + status + "\"";
  jsonData += "}";

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(BACKEND_URL);
    http.addHeader("Content-Type", "application/json");
    http.POST(jsonData);
    http.end();
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("V:");
  lcd.print(voltage, 1);
  lcd.print(" C:");
  lcd.print(current, 1);
  lcd.setCursor(0, 1);
  lcd.print("P:");
  lcd.print(power, 1);
  lcd.print("W ");
  lcd.print(status);
}

// ====== Handle Bulb ON/OFF ======
void handleSetBulb() {
  if (server.hasArg("state")) {
    String state = server.arg("state");
    bulbState = state;
    if (state == "ON") {
      digitalWrite(LOAD_PIN, HIGH);
      ledcWrite(0, brightness > 0 ? brightness : 255); // if no brightness set, full
    } else {
      digitalWrite(LOAD_PIN, LOW);
      ledcWrite(0, 0);
    }

    Serial.println("Bulb State: " + bulbState);
    server.send(200, "application/json", "{\"success\":true,\"device\":\"bulb\",\"state\":\"" + bulbState + "\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"missing state\"}");
  }
}

// ====== Handle Brightness ======
void handleSetBrightness() {
  if (server.hasArg("value")) {
    brightness = constrain(server.arg("value").toInt(), 0, 255);

    if (brightness > 0) {
      bulbState = "ON";
      digitalWrite(LOAD_PIN, HIGH);
      ledcWrite(0, brightness);
    } else {
      bulbState = "OFF";
      digitalWrite(LOAD_PIN, LOW);
      ledcWrite(0, 0);
    }

    Serial.println("Brightness: " + String(brightness));
    server.send(200, "application/json", "{\"success\":true,\"device\":\"bulb\",\"brightness\":" + String(brightness) + "}");
  } else {
    server.send(400, "application/json", "{\"error\":\"missing value\"}");
  }
}

// ====== LOOP ======
void loop() {
  server.handleClient();

  static unsigned long lastSend = 0;
  if (millis() - lastSend > 5000) {
    sendData();
    lastSend = millis();
  }
}
