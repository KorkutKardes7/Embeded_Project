#include <Arduino.h>

// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  int result = myFunction(2, 3);
}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;

void setup() {
  Serial.begin(9600);

  bool status = bme.begin(0x76);

  if (!status) {
    Serial.println("BME280 bulunamadi!");
    while (1);
  }
}

void loop() {

  Serial.print("Sicaklik: ");
  Serial.print(bme.readTemperature());
  Serial.println(" C");

  Serial.print("Nem: ");
  Serial.print(bme.readHumidity());
  Serial.println(" %");

  delay(2000);
}