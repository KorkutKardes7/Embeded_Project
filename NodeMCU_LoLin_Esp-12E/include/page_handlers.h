#ifndef PAGE_HANDLERS_H
#define PAGE_HANDLERS_H

#include <Arduino.h>
#include <ESP8266WebServer.h>

extern ESP8266WebServer server;

extern const char *ESP_SSID;
extern const char *ESP_PASSWORD;
extern const char *lAPTOP_IP;
extern const char *FASTAPI_PORT;

extern String valve_status;
extern float temperature;
extern float humidity;
extern float pressure;

// lora helpers used by some handlers
extern bool loraHasNewPayload(); // lora helper flag
extern String loraGetLatestPayload();
extern void loraMarkPayloadSent();

// functions from mainn.cpp used by handlers
extern bool sendSensorData(bool requireFresh);
extern void fetchLatestData();
extern void setValveIndicator(bool opened);

// page handler declarations
void handleRoot();
void handleTestStatus();
void handleConnect();
void handleTestConnect();
void handleTestPost();
void handleTestGet();

#endif // PAGE_HANDLERS_H
