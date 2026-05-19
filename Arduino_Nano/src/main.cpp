#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SPI.h>
#include <LoRa.h>

#define NSS  10
#define RST  9
#define DIO0 2


String outgoing;              // outgoing message
byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0xBC;     // address of this device
byte destination = 0xFF;      // destination to send to
long lastSendTime = 0;        // last send time
int interval = 2000;          // interval between sends


void sendMessage(String outgoing) {
  if(!LoRa.beginPacket()){                   // start packet
    Serial.println("error beginning packet!");
  }
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  if(!LoRa.endPacket()){                     // finish packet and send it
    Serial.println("error ending packet!");
  }
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
  //bool status = bme.begin(0x76);

  /*
  if (!status) {
    Serial.println("BME280 bulunamadi!");
    while (1);
  }
  */
  while (!Serial);

  Serial.println("LoRa Duplex with callback");

  //LoRa.setSPIFrequency(1E3); //set the spi bus speed to 1KHz. The logic level converter sucks.

  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(NSS, RST, DIO0);// set CS, reset, IRQ pin

  if (!LoRa.begin(433E6)) {             // initialize ratio at 433 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }

  LoRa.onReceive(onReceive);
  LoRa.receive();
  Serial.println("LoRa init succeeded.");
}

void loop() {
  if (millis() - lastSendTime > interval) {
    String message = "Message from arduino Nano!";   // send a message
    sendMessage(message);
    Serial.println("Sending " + message);
    lastSendTime = millis();            // timestamp the message
    interval = random(2000) + 3000;     // 
    LoRa.receive();                     // go back into receive mode
  }
}
