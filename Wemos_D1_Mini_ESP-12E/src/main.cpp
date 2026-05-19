#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

#include "loraHelper.h"


#define NSS  D3
#define RST  D4
#define DIO0 D8


String outgoing;              // outgoing message
byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0xBC;     // address of this device
byte destination = 0xFF;      // destination to send to
long lastSendTime = 0;        // last send time
int interval = 2000;          // interval between sends


void setup() {
  Serial.begin(9600);                   // initialize serial
  
  while (!Serial);

  Serial.println("LoRa Duplex with callback");


  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(NSS, RST, DIO0);// set CS, reset, IRQ pin

  if (!LoRa.begin(433E6)) {             // initialize ratio at 433 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true){delay(10);}                       // if failed, do nothing
  }

  setDeviceAddress(localAddress);

  LoRa.onReceive(onStringReceive);
  LoRa.receive();
  Serial.println("LoRa init succeeded.");
}

void loop() {
  if (millis() - lastSendTime > interval) {
    String message = "Message from Wemos D1 Mini!";   // send a message
    sendString(message, &msgCount, localAddress, destination);
    Serial.println("Sending " + message);
    lastSendTime = millis();            // timestamp the message
    interval = random(2000) + 1000;     // 
    LoRa.receive();                     // go back into receive mode
  }
}
