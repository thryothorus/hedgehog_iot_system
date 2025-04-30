#include <ArduinoBLE.h>
#include "defines.h"
#include "HX711.h"
uint8_t dataPin = 5;
uint8_t clockPin = 4;
HX711 scale;

BLEService scaleService(SCALE_BLE_ID);  // Bluetooth® Low Energy LED Service

BLEFloatCharacteristic scaleCharacteristic(SCALE_BLE_CHAR, BLERead);


void setup() {
  // Serial.begin(9600);
  // while (!Serial)
  //   ;
  if (!BLE.begin()) {
    while (1)
      ;
  }
  scale.begin(dataPin, clockPin);
  BLE.setAdvertisedService(scaleService);


  scaleService.addCharacteristic(scaleCharacteristic);
  scaleCharacteristic.writeValue(154.12345f);
  BLE.addService(scaleService);
  BLE.advertise();
  // Serial.println("BLE SCALE Peripheral");
}

void loop() {
  
  
  BLEDevice central = BLE.central();
  if (central) {
    scaleCharacteristic.writeValue((float) scale.get_value());
    // scaleCharacteristic.writeValue((float) millis());
    // Serial.println("CONNECTED TO CENTRAL");
    while (central.connected()) {
    }
    // Serial.println("DISCONNECTED FROM CENTRAL");
  }
}
