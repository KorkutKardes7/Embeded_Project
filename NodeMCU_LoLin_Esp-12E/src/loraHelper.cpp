/*
* Defitions for loraHelper.h
*/

#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

byte deviceAddress;
static String latestPayload = "";
static bool hasNewPayload = false;
static unsigned long latestPayloadMillis = 0;

void setDeviceAddress(byte address){
  deviceAddress = address;
}

bool loraHasNewPayload() {
  return hasNewPayload;
}

String loraGetLatestPayload() {
  return latestPayload;
}

void loraMarkPayloadSent() {
  hasNewPayload = false;
}

unsigned long loraLatestPayloadTime() {
  return latestPayloadMillis;
}

void sendString(
    String outgoing,      /* outgoing message*/
    byte* msgCount,       /* count of outgoing messages */
    byte localAddress,    /* address of this device */
    byte destination      /* destination to send to */
) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(*msgCount);                // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  *(msgCount)++;                          // increment message ID
}

void onStringReceive(int packetSize) {
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
  if (recipient != deviceAddress && recipient != 0xFF) {
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
  //Serial.println("Snr: " + String(LoRa.packetSnr())); //causes a crash on esp32. 
  Serial.println();

  latestPayload = incoming;
  latestPayloadMillis = millis();
  hasNewPayload = true;
}