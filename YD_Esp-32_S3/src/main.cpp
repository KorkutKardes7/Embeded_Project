#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SPI.h>
#include <LoRa.h>

#include "loraHelper.h"

#define NSS  10
#define RST  4
#define DIO0 5

Adafruit_BME280 bme;

String outgoing;              // outgoing message
byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0xBA;     // address of this device
byte destination = 0xFF;      // destination to send to
long lastSendTime = 0;        // last send time
int interval = 2000;          // interval between sends

void setup() {
  Serial.begin(115200);
  delay(1000);

  bool status = bme.begin(0x76);

  if (!status) {
    Serial.println("BME280 not found!");
    while (1);
  }

  Serial.println("LoRa Duplex with callback");

  // override the default CS, reset, and IRQ pins
  LoRa.setPins(NSS, RST, DIO0);// set CS, reset, IRQ pin

  if (!LoRa.begin(433E6)) {             // initialize ratio at 433 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }

  setDeviceAddress(localAddress);

  LoRa.onReceive(onStringReceive);
  LoRa.receive();
  Serial.println("LoRa init succeeded.");
}

void loop() {
  if (millis() - lastSendTime > interval) {
    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100.0F; // hPa

    String message = "T:" + String(temperature, 1) + "C "
                     + "H:" + String(humidity, 1) + "% "
                     + "P:" + String(pressure, 1) + "hPa";

    sendString(message, &msgCount, localAddress, destination);
    Serial.println("Sending " + message);
    lastSendTime = millis();            // timestamp the message
    interval = random(2000) + 1000;     // 2-3 seconds
    LoRa.receive();                     // go back into receive mode
  }
}