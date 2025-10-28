#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "PARADOX";
const char* password = "shouryakumar";

const char* serverUrl = "http://172.20.10.3:3000/api/data/latest";

const int R = 220;  // ohm

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Simulate random LDR readings
  int LDR = random(0, 1000);  // 0–1000
  int percentage = (LDR * 100) / 1000;
  float voltage = ((float)percentage / 100.0) * 12.0;
  float current = voltage / R;
  float power = voltage * current;

  String status;
  if (voltage >= 4.0) {
    status = "OK";
  } else if (voltage >= 2.0) {
    status = "LOW";
  } else {
    status = "CRITICAL";
  }

  // Prepare JSON data
  String jsonData = "{";
  jsonData += "\"percentage\":" + String(percentage) + ",";
  jsonData += "\"voltage\":" + String(voltage, 3) + ",";
  jsonData += "\"current\":" + String(current, 3) + ",";
  jsonData += "\"power\":" + String(power, 3) + ",";
  jsonData += "\"status\":\"" + status + "\"";
  jsonData += "}";

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonData);

    Serial.print("Sent: ");
    Serial.println(jsonData);
    Serial.print("Response code: ");
    Serial.println(httpResponseCode);

    http.end();
  } else {
    Serial.println("WiFi not connected!");
  }

  delay(1000);
}
