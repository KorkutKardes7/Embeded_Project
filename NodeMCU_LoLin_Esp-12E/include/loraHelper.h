#pragma once
/*
* Helper functions for LoRa functionality.
*/


void setDeviceAddress(byte address);

void sendString(
    String outgoing,      /* outgoing message*/
    byte* msgCount,        /* count of outgoing messages */
    byte localAddress,    /* address of this device */
    byte destination      /* destination to send to */
);

void onStringReceive(int packetSize);