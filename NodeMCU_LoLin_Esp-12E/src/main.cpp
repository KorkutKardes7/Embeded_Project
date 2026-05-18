#include <Arduino.h>

#include <SPI.h>
#include <LoRa.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>


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


void sendMessage(String outgoing) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}


void onReceive(int packetSize) {
  Serial.println("?");
  if (packetSize == 0) return;          // if there's no packet, return
  Serial.println("!");

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";                 // payload of packet

  while (LoRa.available()) {            // can't use readString() in callback, so
    incoming += (char)LoRa.read();      // add bytes one by one
  }

  if (incomingLength != incoming.length()) {   // check length for error
    Serial.println("error: message length does not match length");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF) {
    Serial.println("This message is not for me.");
    return;                             // skip rest of function
  }

  // if message is for this device, or broadcast, print details:
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();
}



void setup() {
  Serial.begin(9600);                   // initialize serial
  while (!Serial);

  Serial.println("LoRa Duplex with callback");

  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(NSS, RST, DIO0);// set CS, reset, IRQ pin

  LoRa.setSPIFrequency(8E6);

  if (!LoRa.begin(433E6)) {             // initialize ratio at 433 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }

  LoRa.onReceive(onReceive);
  LoRa.receive();
  Serial.println("LoRa init succeeded.");

  setupAccessPoint();
}

void loop() {
  if (millis() - lastSendTime > interval) {
    String message = "HeLoRa World!";   // send a message
    sendMessage(message);
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
