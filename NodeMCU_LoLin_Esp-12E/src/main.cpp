#include <Arduino.h>

#include <SPI.h>
#include <LoRa.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

#include "loraHelper.h"
#include "page_handlers.h"

#define NSS D8
#define RST D3
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
bool loraReady = false;

bool extractThreeFloats(const String &input, float *outValues)
{
  size_t count = 0;
  String token = "";

  for (size_t i = 0; i <= input.length(); ++i)
  {
    char c = (i < input.length()) ? input.charAt(i) : ' ';
    bool isNum = (c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E';
    if (isNum)
    {
      token += c;
    }
    else if (token.length() > 0)
    {
      outValues[count++] = token.toFloat();
      token = "";
      if (count >= 3)
      {
        return true;
      }
    }
  }

  return count >= 3;
}

bool updateSensorReadingsFromLora(const String &payload)
{
  float values[3];
  if (!extractThreeFloats(payload, values))
  {
    Serial.println("[LoRa] Failed to parse payload (need 3 numbers).");
    return false;
  }

  humidity = values[0];
  temperature = values[1];
  pressure = values[2];
  return true;
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
bool sendSensorData(bool requireFresh)
{
  // if not connected
  if (!WiFi.isConnected())
  {
    Serial.println("[POST] Not connected, skip");
    return false;
  }

  bool hasPayload = loraHasNewPayload();
  if (requireFresh && !hasPayload)
  {
    Serial.println("[POST] No new LoRa payload, skip");
    return false;
  }

  if (hasPayload)
  {
    String payload = loraGetLatestPayload();
    if (!updateSensorReadingsFromLora(payload) && requireFresh)
    {
      return false;
    }
  }

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

    if (hasPayload)
    {
      loraMarkPayloadSent();
    }
  }
  else
  {
    Serial.print("[POST] Failed: ");
    Serial.println(http.errorToString(code).c_str());
    printWifiStatus();
  }
  http.end();
  return code > 0;
}

void fetchLatestData()
{
  if (!WiFi.isConnected())
  {
    Serial.println("[GET] Not connected, skip");
    return;
  }

  String url = "http://" + String(lAPTOP_IP) + ":" + String(FASTAPI_PORT) + "/api/web/data";
  Serial.print("[GET] URL: ");
  Serial.println(url);

  HTTPClient http;
  WiFiClient client;
  http.begin(client, url);
  int code = http.GET();

  if (code > 0)
  {
    Serial.print("[GET] HTTP code: ");
    Serial.println(code);
    String response = http.getString();
    Serial.print("[GET] Response (first 200 chars): ");
    if (response.length() > 200)
    {
      Serial.println(response.substring(0, 200));
    }
    else
    {
      Serial.println(response);
    }
  }
  else
  {
    Serial.print("[GET] Failed: ");
    Serial.println(http.errorToString(code).c_str());
  }
  http.end();
}

void setup()
{
  Serial.begin(9600); // initialize serial
  while (!Serial)
    ;

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

  if (!LoRa.begin(433E6))
  { // initialize ratio at 433 MHz
    Serial.println("LoRa init failed. Check your connections.");
    loraReady = false;
  }
  else
  {
    loraReady = true;
  }
  if (loraReady)
  {
    LoRa.onReceive(onStringReceive);
    LoRa.receive();
    Serial.println("LoRa init succeeded.");
  }

  pinMode(LED_BUILTIN, OUTPUT);
  setValveIndicator(false);
}

void loop()
{
  server.handleClient();

  if (loraReady && millis() - lastSendTime > interval)
  {
    String message = "Message from LoL1n esp8266!"; // send a message
    sendString(message, &msgCount, localAddress, destination);
    Serial.println("Sending " + message);
    lastSendTime = millis();        // timestamp the message
    interval = random(2000) + 1000; // 2-3 seconds
    LoRa.receive();                 // go back into receive mode
  }

  // Periodically send sensor data to FastAPI
  if (WiFi.isConnected() && loraHasNewPayload() && (millis() - last_post_time) >= POST_INTERVAL)
  {
    Serial.println("\n[LOOP] Sending sensor data...");
    sendSensorData(true);
    last_post_time = millis();
  }
}
