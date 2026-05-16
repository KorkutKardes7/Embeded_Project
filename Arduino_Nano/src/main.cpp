#include <Arduino.h>

#include <SPI.h>
#include <LoRa.h>

#define NSS  10
#define RST  9
#define DIO0 2

int counter = 0;

void onTxDone() {
  Serial.println("TxDone");
}

boolean runEvery(unsigned long interval)
{
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    return true;
  }
  return false;
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("LoRa Sender non-blocking Callback");

  LoRa.setPins(NSS, RST, DIO0);
  LoRa.setSPIFrequency(1E3); //set the spi bus speed to 1KHz. The logic level converter sucks.

  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  LoRa.onTxDone(onTxDone);
}

void loop() {
  if (runEvery(5000)) { // repeat every 5000 millis

    Serial.print("Sending packet non-blocking: ");
    Serial.println(counter);

    // send in async / non-blocking mode
    LoRa.beginPacket();
    LoRa.print("hello ");
    LoRa.print(counter);
    LoRa.endPacket(true); // true = async / non-blocking mode

    counter++;
  }
}

