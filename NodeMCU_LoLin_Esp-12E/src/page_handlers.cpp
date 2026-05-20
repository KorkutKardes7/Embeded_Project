#include "page_handlers.h"
#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

// Root page handler
void handleRoot()
{
  WiFi.mode(WIFI_AP_STA);
  int n = WiFi.scanNetworks();

  String html = "<!DOCTYPE html><html lang=\"tr\"><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";
  html += "<title>ESP32 WiFi Setup</title>";
  html += "<style>body{font-family:Arial,Helvetica,sans-serif;background:#f6f8fb;color:#1f2937;padding:18px} .card{background:#fff;border-radius:8px;box-shadow:0 6px 18px rgba(31,41,55,0.08);padding:18px;max-width:720px;margin:12px auto} h1{margin:0 0 8px 0;font-size:20px} .net{display:flex;align-items:center;justify-content:space-between;padding:8px;border-bottom:1px solid #eef2f7} .net input{margin-right:12px} .muted{color:#6b7280;font-size:13px} .btn{background:#2563eb;color:#fff;padding:10px 14px;border-radius:6px;border:0;cursor:pointer} .field{margin:8px 0} .small{font-size:13px;color:#475569} .sig{font-weight:600;color:#111827;padding-left:8px}</style></head><body>";
  html += "<div class=\"card\"><h1>WiFi Ayarlari</h1>";
  html += "<p class=\"muted\">ESP is in AP mode. Select the network and enter password to connect.</p>";
  html += "<form method=\"POST\" action=\"/connect\">";

  if (n <= 0)
  {
    html += "<p class=\"small\">No networks found</p>";
  }
  else
  {
    html += "<div class=\"small\"><b>Available Networks:</b></div>";
    for (int i = 0; i < n; ++i)
    {
      String item = WiFi.SSID(i);
      int rssi = WiFi.RSSI(i);
      String enc = (WiFi.encryptionType(i) == 0) ? "Open" : "Secured";
      String sig = "";
      if (rssi >= -50)
        sig = "▂▄▆█";
      else if (rssi >= -60)
        sig = "▂▄▆";
      else if (rssi >= -70)
        sig = "▂▄";
      else
        sig = "▂";
      html += "<label class=\"net\"><span><input type=\"radio\" name=\"ssid\" value=\"" + item + "\">" + item + " <span class=\"muted\">" + enc + "</span></span><span class=\"sig\">" + sig + "</span></label>";
    }
  }

  html += "<div class=\"field\"><label class=\"small\">Password</label><br><input style=\"width:100%;padding:8px;border-radius:6px;border:1px solid #e6eef6\" type=\"password\" name=\"pass\"></div>";
  html += "<div style=\"display:flex;gap:12px;align-items:center\"><button class=\"btn\" type=\"submit\">Connect</button><span class=\"small muted\">Or keep AP mode to retry later</span></div>";
  html += "</form></div></body></html>";

  server.send(200, "text/html; charset=UTF-8", html);
}

// Board test page handler
void handleTestStatus()
{
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

// WiFi connection handler
void handleConnect()
{
  String targetSsid = server.arg("ssid");
  String targetPass = server.arg("pass");

  Serial.print("Requested connect to: ");
  Serial.println(targetSsid);

  if (targetSsid.length() == 0)
  {
    server.send(400, "text/plain", "Missing SSID");
    return;
  }

  server.send(200, "text/html", "Attempting to connect... Please check serial logs.");

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(targetSsid.c_str(), targetPass.c_str());

  Serial.print("Connecting to target WiFi");
  unsigned long start = millis();
  const unsigned long timeout = 20000;
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeout)
  {
    delay(500);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    IPAddress ip = WiFi.localIP();
    Serial.print("Connected, IP: ");
    Serial.println(ip);

    delay(1000);
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ESP_SSID, ESP_PASSWORD);
    Serial.println("[MODE] Switched to STA+AP");
    IPAddress apIP = WiFi.softAPIP();
    Serial.print("[AP] IP: ");
    Serial.println(apIP);
    Serial.println("[CONNECT] Ready to send sensor data to FastAPI");
  }
  else
  {
    Serial.println("[CONNECT] Failed. Keeping AP for retry.");
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ESP_SSID, ESP_PASSWORD);
    IPAddress apIP = WiFi.softAPIP();
    Serial.print("[AP] IP: ");
    Serial.println(apIP);
  }
}

// Fastapi connection test handler
void handleTestConnect()
{
  Serial.println("[CONN_TEST] Testing connection to FastAPI...");
  String url = "http://" + String(lAPTOP_IP) + ":" + String(FASTAPI_PORT) + "/";
  Serial.print("[CONN_TEST] URL: ");
  Serial.println(url);

  HTTPClient http;
  WiFiClient client;
  http.setTimeout(3000);
  http.begin(client, url);
  int code = http.GET();

  String result = "{\"test\":\"connection\",";
  if (code > 0)
  {
    result += "\"status\":\"success\",\"code\":" + String(code);
    Serial.println("[CONN_TEST] SUCCESS - Server is reachable!");
  }
  else
  {
    result += "\"status\":\"failed\",\"error\":\"" + http.errorToString(code) + "\"";
    Serial.print("[CONN_TEST] FAILED - ");
    Serial.println(http.errorToString(code).c_str());
  }
  http.end();
  result += "}";

  server.send(200, "application/json", result);
}

// Test endpoints for validation
void handleTestPost()
{
  Serial.println("[TEST] POST /test-post triggered");
  sendSensorData(false);
  server.send(200, "application/json", "{\"status\":\"posted\",\"air_temperature\":" + String(temperature, 1) + ",\"soil_temperature\":" + String(humidity, 1) + ",\"soil_moisture\":" + String(pressure, 1) + "}");
}

void handleTestGet()
{
  Serial.println("[TEST] GET /test-get triggered");
  fetchLatestData();
  server.send(200, "application/json", "{\"status\":\"fetched\"}");
}
