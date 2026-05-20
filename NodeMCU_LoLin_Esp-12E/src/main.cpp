#include <Arduino.h>

#include <SPI.h>
#include <LoRa.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

#include "loraHelper.h"



#define NSS  D8
#define RST  D3
#define DIO0 D2

String outgoing;          // outgoing message
byte msgCount = 0;        // count of outgoing messages
byte localAddress = 0xBB; // address of this device
byte destination = 0xFF;  // destination to send to
long lastSendTime = 0;    // last send time
int interval = 2000;      // interval between sends

// ESP acts as access point. Connect laptop to this SSID.
const char *ESP_SSID = "NodeMCU_Setup";
const char *ESP_PASSWORD = "12345678";

// If laptop is connected to ESP paste ip address and api port to here or may add ip changing page
const char *lAPTOP_IP = "192.168.1.4";
const char *FASTAPI_PORT = "5000";

// Sensor data
float temperature = 0.0;
float humidity = 0.0;
float pressure = 0.0;
String valve_status = "CLOSED";
ESP8266WebServer server(80);
unsigned long last_post_time = 0;
const unsigned long POST_INTERVAL = 15000; // Post every 15 seconds

unsigned long lastPostTime = 0;
const unsigned long postIntervalMs = 5000;

void updateSensorReadings()
{
  // Replace with real sensor reads when hardware is connected.
  temperature = 18.0 + ((millis() / 1000) % 40) * 0.1;
  humidity = 45.0 + ((millis() / 900) % 30) * 0.1;
  pressure = 1010.0 + ((millis() / 1100) % 20) * 0.1;
}

void printWifiStatus()
{
  Serial.print("[WiFi] Status: ");
  Serial.println(WiFi.status());
  if (WiFi.isConnected())
  {
    Serial.print("[WiFi] Local IP: ");
    Serial.println(WiFi.localIP());
  }
}

void setValveIndicator(bool opened)
{
  // NodeMCU builtin LED is active LOW.
  digitalWrite(LED_BUILTIN, opened ? LOW : HIGH);
}

// POST sensor data to FastAPI /api/esp/telemetry
void sendSensorData()
{
  // if not connected
  if (!WiFi.isConnected())
  {
    Serial.println("[POST] Not connected, skip");
    return;
  }

  updateSensorReadings();
  String url = "http://" + String(lAPTOP_IP) + ":" + String(FASTAPI_PORT) + "/api/esp/telemetry";
  String payload = "{";
  payload += "\"temperature\":" + String(temperature, 1) + ",";
  payload += "\"humidity\":" + String(humidity, 1) + ",";
  payload += "\"pressure\":" + String(pressure, 1);
  payload += "}";

  Serial.print("[POST] URL: ");
  Serial.println(url);
  Serial.print("[POST] Payload: ");
  Serial.println(payload);

  HTTPClient http;
  WiFiClient client;
  http.setTimeout(5000);
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(payload);

  if (code > 0)
  {
    Serial.print("[POST] HTTP code: ");
    Serial.println(code);
    String response = http.getString();
    Serial.print("[POST] Response: ");
    Serial.println(response);

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    if (!err && doc["valve_command"].is<String>())
    {
      String command = doc["valve_command"].as<String>();
      if (command == "OPEN")
      {
        valve_status = "OPEN";
        setValveIndicator(true);
        Serial.println("[VALVE] OPEN");
      }
      else if (command == "CLOSE")
      {
        valve_status = "CLOSE";
        setValveIndicator(false);
        Serial.println("[VALVE] CLOSE");
      }
    }
    else if (err)
    {
      Serial.print("[POST] JSON parse error: ");
      Serial.println(err.c_str());
    }
  }
  else
  {
    Serial.print("[POST] Failed: ");
    Serial.println(http.errorToString(code).c_str());
    Serial.println("[POST] Check: 1) Laptop IP correct? 2) FastAPI server running? 3) Firewall allowing port 5000?");
    printWifiStatus();
  }
  http.end();
}


void fetchLatestData() {
  if (!WiFi.isConnected()) {
    Serial.println("[GET] Not connected, skip");
    return;
  }

  String url = "http://" + String(lAPTOP_IP) + ":" + String(FASTAPI_PORT) + "/api/web/data";
  Serial.print("[GET] URL: ");
  Serial.println(url);

  HTTPClient http;
  http.begin(url);
  int code = http.GET();

  if (code > 0) {
    Serial.print("[GET] HTTP code: ");
    Serial.println(code);
    String response = http.getString();
    Serial.print("[GET] Response (first 200 chars): ");
    if (response.length() > 200) {
      Serial.println(response.substring(0, 200));
    } else {
      Serial.println(response);
    }
  } else {
    Serial.print("[GET] Failed: ");
    Serial.println(http.errorToString(code).c_str());
  }
  http.end();
}

// Web page handlers
// Root page handler
void handleRoot()
{
  // Scan for nearby WiFi networks (requires AP+STA mode)
  WiFi.mode(WIFI_AP_STA);
  int n = WiFi.scanNetworks(); //gives network count

  String html = "<!DOCTYPE html><html lang=\"tr\"><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";
  html += "<title>ESP32 WiFi Setup</title>";
  html += "<style>body{font-family:Arial,Helvetica,sans-serif;background:#f6f8fb;color:#1f2937;padding:18px} .card{background:#fff;border-radius:8px;box-shadow:0 6px 18px rgba(31,41,55,0.08);padding:18px;max-width:720px;margin:12px auto} h1{margin:0 0 8px 0;font-size:20px} .net{display:flex;align-items:center;justify-content:space-between;padding:8px;border-bottom:1px solid #eef2f7} .net input{margin-right:12px} .muted{color:#6b7280;font-size:13px} .btn{background:#2563eb;color:#fff;padding:10px 14px;border-radius:6px;border:0;cursor:pointer} .field{margin:8px 0} .small{font-size:13px;color:#475569} .sig{font-weight:600;color:#111827;padding-left:8px}</style></head><body>";
  html += "<div class=\"card\"><h1>WiFi Ayarlari</h1>";
  html += "<p class=\"muted\">ESP is in AP mode. Select the network and enter password to connect.</p>";
  html += "<form method=\"POST\" action=\"/connect\">";

  if (n <= 0) {
    html += "<p class=\"small\">No networks found</p>";
  } else {
    html += "<div class=\"small\"><b>Available Networks:</b></div>";
    for (int i = 0; i < n; ++i) {
      String item = WiFi.SSID(i);
      int rssi = WiFi.RSSI(i);
      String enc = (WiFi.encryptionType(i) == 0) ? "Open" : "Secured"; // I solved it in worst way possible look at encryption types
      String sig = "";
      if (rssi >= -50) sig = "▂▄▆█";
      else if (rssi >= -60) sig = "▂▄▆";
      else if (rssi >= -70) sig = "▂▄";
      else sig = "▂";
      html += "<label class=\"net\"><span><input type=\"radio\" name=\"ssid\" value=\"" + item + "\">" + item + " <span class=\"muted\">" + enc + "</span></span><span class=\"sig\">" + sig + "</span></label>";
    }
  }

  html += "<div class=\"field\"><label class=\"small\">Password</label><br><input style=\"width:100%;padding:8px;border-radius:6px;border:1px solid #e6eef6\" type=\"password\" name=\"pass\"></div>";
  html += "<div style=\"display:flex;gap:12px;align-items:center\"><button class=\"btn\" type=\"submit\">Connect</button><span class=\"small muted\">Or keep AP mode to retry later</span></div>";
  html += "</form></div></body></html>";

  server.send(200, "text/html; charset=UTF-8", html);
}
//Board test page handler
void handleTestStatus() {
  String json = "{";
  json += "\"wifi\":\"" + String(WiFi.isConnected() ? "connected" : "disconnected") + "\",";
  json += "\"ssid\":\"" + String(WiFi.SSID()) + "\",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"valve\":\"" + valve_status + "\",";
  json += "\"temperature\":" + String(temperature, 1) + ",";
  json += "\"humidity\":" + String(humidity, 1) + ",";
  json += "\"pressure\":" + String(pressure, 1);
  json += "}";
  server.send(200, "application/json", json);
}
// Connection handlers
// WiFi connection hadler
void handleConnect()
{
  String targetSsid = server.arg("ssid");
  String targetPass = server.arg("pass");

  Serial.print("Requested connect to: ");
  Serial.println(targetSsid);

  if (targetSsid.length() == 0) {
    server.send(400, "text/plain", "Missing SSID");
    return;
  }

  server.send(200, "text/html", "Attempting to connect... Please check serial logs.");

  // Keep AP alive + connect to STA (STA+AP mode)
  // This allows changing WiFi in future without losing AP connectivity
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(targetSsid.c_str(), targetPass.c_str());

  Serial.print("Connecting to target WiFi");
  unsigned long start = millis();
  const unsigned long timeout = 20000; // 20s
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeout) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    IPAddress ip = WiFi.localIP();
    Serial.print("Connected, IP: ");
    Serial.println(ip);

    // Switch to STA+AP so AP remains active for future WiFi changes
    delay(1000);
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ESP_SSID, ESP_PASSWORD);
    Serial.println("[MODE] Switched to STA+AP");
    IPAddress apIP = WiFi.softAPIP();
    Serial.print("[AP] IP: ");
    Serial.println(apIP);
    Serial.println("[CONNECT] Ready to send sensor data to FastAPI");

  } else {
    Serial.println("[CONNECT] Failed. Keeping AP for retry.");
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ESP_SSID, ESP_PASSWORD);
    IPAddress apIP = WiFi.softAPIP();
    Serial.print("[AP] IP: ");
    Serial.println(apIP);
  }
}
// Fastapi connection test handler
void handleTestConnect() {
  Serial.println("[CONN_TEST] Testing connection to FastAPI...");
  String url = "http://" + String(lAPTOP_IP) + ":" + String(FASTAPI_PORT) + "/";
  Serial.print("[CONN_TEST] URL: "); Serial.println(url);
  
  HTTPClient http;
  WiFiClient client;
  http.setTimeout(3000);
  http.begin(client, url);
  int code = http.GET();
  
  String result = "{\"test\":\"connection\",";
  if (code > 0) {
    result += "\"status\":\"success\",\"code\":" + String(code);
    Serial.println("[CONN_TEST] SUCCESS - Server is reachable!");
  } else {
    result += "\"status\":\"failed\",\"error\":\"" + http.errorToString(code) + "\"";
    Serial.print("[CONN_TEST] FAILED - "); Serial.println(http.errorToString(code).c_str());
  }
  http.end();
  result += "}";
  
  server.send(200, "application/json", result);
}
// Test endpoints for validation
void handleTestPost() {
  Serial.println("[TEST] POST /test-post triggered");
  sendSensorData();
  server.send(200, "application/json", "{\"status\":\"posted\",\"air_temperature\":" + String(temperature, 1) + ",\"soil_temperature\":" + String(humidity, 1) + ",\"soil_moisture\":" + String(pressure, 1) + "}");
}
void handleTestGet() {
  Serial.println("[TEST] GET /test-get triggered");
  fetchLatestData();
  server.send(200, "application/json", "{\"status\":\"fetched\"}");
}

void setup()
{
  Serial.begin(9600); // initialize serial
  while (!Serial);

  Serial.println("WiFi Starting...");
  WiFi.softAP(ESP_SSID, ESP_PASSWORD);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("IP Address: ");
  Serial.println(IP);

  server.on("/", handleRoot);
  server.on("/connect", HTTP_POST, handleConnect);
  server.on("/test-post", HTTP_GET, handleTestPost);
  server.on("/test-get", HTTP_GET, handleTestGet);
  server.on("/status", HTTP_GET, handleTestStatus);
  server.on("/test-conn", HTTP_GET, handleTestConnect);

  server.begin();
  Serial.println("[SETUP] HTTP server started.");
  Serial.println("[SETUP] Test endpoints:");
  Serial.println("  GET http://ESP32_AP_IP/status");
  Serial.println("  GET http://ESP32_AP_IP/test-post");
  Serial.println("  GET http://ESP32_AP_IP/test-get");
  Serial.println("  GET http://ESP32_AP_IP/test-conn (connectivity test)");

  Serial.println("LoRa Duplex with callback");

  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(NSS, RST, DIO0); // set CS, reset, IRQ pin

  LoRa.setSPIFrequency(8E6);

  setDeviceAddress(localAddress);


  if (!LoRa.begin(433E6)) {             // initialize ratio at 433 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true)
      ; // if failed, do nothing
  }

  LoRa.onReceive(onStringReceive);
  LoRa.receive();
  Serial.println("LoRa init succeeded.");

  pinMode(LED_BUILTIN, OUTPUT);
  setValveIndicator(false);
}

void loop() {
  
  if (millis() - lastSendTime > interval) {
    String message = "Message from LoL1n esp8266!";   // send a message
    sendString(message, &msgCount, localAddress, destination);
    Serial.println("Sending " + message);
    lastSendTime = millis();        // timestamp the message
    interval = random(2000) + 1000; // 2-3 seconds
    LoRa.receive();                 // go back into receive mode
  }

  // Periodically send sensor data to FastAPI
  if (WiFi.isConnected() && (millis() - last_post_time) >= POST_INTERVAL) {
    Serial.println("\n[LOOP] Sending sensor data...");
    sendSensorData();
    last_post_time = millis();
  }

  if (millis() - lastPostTime > postIntervalMs)
  {
    lastPostTime = millis();

    // Placeholder values; replace with real sensor reads when available.
    float airTemperature = 18.0 + ((millis() / 1000) % 40) * 0.1;
    float soilTemperature = 12.0 + ((millis() / 1200) % 30) * 0.1;
    float soilMoisture = 40.0 + ((millis() / 1500) % 20) * 0.5;
  }
}
