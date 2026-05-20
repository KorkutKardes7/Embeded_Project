#pragma once
/*
* Helper functions for LoRa functionality.
*/

#include <Arduino.h>


void setDeviceAddress(byte address);

bool loraHasNewPayload();
String loraGetLatestPayload();
void loraMarkPayloadSent();
unsigned long loraLatestPayloadTime();

void sendString(
    String outgoing,      /* outgoing message*/
    byte* msgCount,        /* count of outgoing messages */
    byte localAddress,    /* address of this device */
    byte destination      /* destination to send to */
);

void onStringReceive(int packetSize);