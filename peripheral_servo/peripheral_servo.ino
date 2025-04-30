#include "defines.h"
#include <ArduinoBLE.h>

#include <Adafruit_PWMServoDriver.h>
#include <Wire.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
BLEService servoService(SERVO_BLE_ID); // Bluetooth® Low Energy LED Service

BLEByteCharacteristic servoCharacteristic(SERVO_BLE_CHAR, BLERead | BLEWrite);

#define SERVOMIN 150
#define SERVOREST 300
#define SERVOMAX 600
#define SERVO_FREQ 50
uint8_t servonum = 0;
uint8_t sequence_num = 0;

void setup()
{
    Serial.begin(9600);
    delay(250);
    // while (!Serial)
    //     ;

    pwm.begin();

    pwm.setOscillatorFrequency(27000000);
    pwm.setPWMFreq(SERVO_FREQ); // Analog servos run at ~50 Hz updates

    for (servonum; servonum < 6; servonum++) {
        pwm.setPWM(servonum, 0, SERVOREST + 50);
        delay(250);
        pwm.setPWM(servonum, 0, SERVOREST - 50);
        delay(250);
        pwm.setPWM(servonum, 0, SERVOREST);
        delay(250);
        pwm.setPWMOff(servonum);
        Serial.println(servonum);
    }
    servonum = 0;
    pwm.sleep();

    if (!BLE.begin()) {
        while (1)
            ;
    }

    BLE.setAdvertisedService(servoService);

    servoService.addCharacteristic(servoCharacteristic);
    servoCharacteristic.writeValue(0);

    BLE.addService(servoService);

    BLE.advertise();
    Serial.println("BLE SERVO Peripheral");
}

void open_next_servo()
{
    if (sequence_num >= 12) {
        Serial.println("Not opening any more bins");

        return;
    }

    int servo_use = sequence_num / 2;
    int bin_use = sequence_num % 2;

    Serial.print("Opening servo# ");
    Serial.print(sequence_num);
    Serial.print(",");
    Serial.print(servo_use);
    Serial.print(",");
    Serial.println(bin_use);

    pwm.wakeup();
    delay(250);
    if (bin_use == 1) {

      if ((servo_use == 0)
            || (servo_use == 5)) {
            pwm.setPWM(servo_use, 0, 450);
        } else {
            pwm.setPWM(servo_use, 0, 500);
        }


        
    } else {
        if ((servo_use == 0)
            || (servo_use == 5)) {
            pwm.setPWM(servo_use, 0, 150);
        } else {
            pwm.setPWM(servo_use, 0, 100);
        }
    }

    delay(500);
    pwm.setPWM(servo_use, 0, SERVOREST);
    delay(50);
    pwm.setPWMOff(servo_use);
    delay(10);
    pwm.sleep();

    sequence_num += 1;
}
void loop()
{
    BLEDevice central = BLE.central();
    if (central) {
        Serial.println("CONNECTED TO CENTRAL");
        while (central.connected()) {
            if (servoCharacteristic.written()) {
                if (servoCharacteristic.value()) { // any value other than 0
                    Serial.println("OPENING BIN");
                    open_next_servo();
                }
                Serial.println("DISCONNECTED FROM CENTRAL");
            }
        }
    }
}
