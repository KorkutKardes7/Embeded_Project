#include <Arduino.h>

#include <SPI.h>
#include <LoRa.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>

#include "loraHelper.h"



#define NSS  D8
#define RST  D3
#define DIO0 D2


String outgoing;              // outgoing message
byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0xBB;     // address of this device
byte destination = 0xFF;      // destination to send to
long lastSendTime = 0;        // last send time
int interval = 2000;          // interval between sends

// ESP acts as access point. Connect laptop to this SSID.
const char* AP_SSID = "NodeMCU_AP";
const char* AP_PASSWORD = "12345678";

// If laptop is connected to ESP AP, laptop is commonly 192.168.4.2
// and ESP gateway/AP is 192.168.4.1.
const char* API_URL = "http://192.168.4.2:5000/api/esp/telemetry";

unsigned long lastPostTime = 0;
const unsigned long postIntervalMs = 5000;

void setupAccessPoint() {
  IPAddress localIp(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(localIp, gateway, subnet);
  bool started = WiFi.softAP(AP_SSID, AP_PASSWORD);

  Serial.print("AP start: ");
  Serial.println(started ? "OK" : "FAIL");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void postTelemetry(float airTemperature, float soilTemperature, float soilMoisture) {
  WiFiClient wifiClient;
  HTTPClient http;

  if (!http.begin(wifiClient, API_URL)) {
    Serial.println("HTTP begin failed");
    return;
  }

  http.addHeader("Content-Type", "application/json");

  String payload = "{";
  payload += "\"air_temperature\":" + String(airTemperature, 2) + ",";
  payload += "\"soil_temperature\":" + String(soilTemperature, 2) + ",";
  payload += "\"soil_moisture\":" + String(soilMoisture, 2);
  payload += "}";

  int httpCode = http.POST(payload);
  String response = http.getString();

  Serial.print("HTTP code: ");
  Serial.println(httpCode);
  Serial.print("HTTP response: ");
  Serial.println(response);

  http.end();
}


void setup() {
  Serial.begin(9600);                   // initialize serial
  while (!Serial);

  Serial.println("LoRa Duplex with callback");

  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(NSS, RST, DIO0);// set CS, reset, IRQ pin

  LoRa.setSPIFrequency(8E6);

  setDeviceAddress(localAddress);


  if (!LoRa.begin(433E6)) {             // initialize ratio at 433 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }

  LoRa.onReceive(onStringReceive);
  LoRa.receive();
  Serial.println("LoRa init succeeded.");

  setupAccessPoint();
}

void loop() {
  
  if (millis() - lastSendTime > interval) {
    String message = "Message from LoL1n esp8266!";   // send a message
    sendString(message, &msgCount, localAddress, destination);
    Serial.println("Sending " + message);
    lastSendTime = millis();            // timestamp the message
    interval = random(2000) + 1000;     // 2-3 seconds
    LoRa.receive();                     // go back into receive mode
  }
  

  if (millis() - lastPostTime > postIntervalMs) {
    lastPostTime = millis();

    // Placeholder values; replace with real sensor reads when available.
    float airTemperature = 18.0 + ((millis() / 1000) % 40) * 0.1;
    float soilTemperature = 12.0 + ((millis() / 1200) % 30) * 0.1;
    float soilMoisture = 40.0 + ((millis() / 1500) % 20) * 0.5;

    postTelemetry(airTemperature, soilTemperature, soilMoisture);
  }
}
