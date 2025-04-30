#include <PDM.h>
#include "defines.h"
#include <Arduino_APDS9960.h>  //Modify the official Arduino_APDS9960 library to remove all "private:" in Arduino_APDS9960.h
#include "Arduino_BMI270_BMM150.h"
#include <Arduino_HS300x.h>
#include <Arduino_LPS22HB.h>
#include <ArduinoBLE.h>

#define SHOW_SENSOR_PRINT false
float a_x, a_y, a_z;
bool scan_for_servo_ble{ false }; 
bool scan_for_weight_ble{ false };
float temperature, humidity, pressure;

short sampleBuffer[256];
volatile int samplesRead = 0;
float volume = 0;
unsigned long last_color = 0;

int r, g, b;


void onPDMdata() {
  int bytesAvailable = PDM.available();
  PDM.read(sampleBuffer, bytesAvailable);
  samplesRead = bytesAvailable / 2;  // 16-bit samples
}


void setup() {
  Serial.begin(115200);
  while (!Serial) {};

  last_color = millis();


  Serial.println("L:STARTED_SERIAL");

  if (!HS300x.begin()) {
    Serial.println("E:HUMIDITY_TEMPERATURE_FAILED_TO_START");
  }

  if (!BARO.begin()) {
    Serial.println("E:BAROMETER_FAILED_TO_START");
  }


  if (!IMU.begin()) {
    Serial.println("E:IMU_FAILED_TO_BEGIN");
  }
  IMU.accelerationSampleRate();

  if (!APDS.begin()) {
    Serial.println("E:APDS_FAILED_TO_START");
  }
  APDS.setCONTROL(0x03);

  PDM.onReceive(onPDMdata);
  if (!PDM.begin(1, 16000)) {  // mono, 16kHz sample rate
    Serial.println("E:PDM_FAILED_TO_START");
    while (1)
      ;
  }

  PDM.setGain(80);  // Adjust as needed (0-80)

  BLE.begin();

  Serial.println("L:FINISHED_SETUP_STARTING_LOOP");
  // APDS.enableGesture();
  // APDS.enableColor();
}


bool ble_servo_manage(BLEDevice peripheral) {

  Serial.println("L:STARTING_SERVO_MANAGEMENT");
  if (peripheral.connect()) {
    Serial.println("L:SERVO_CONNECTED");
  } else {
    Serial.println("L:SERVO_FAIL_TO_CONNECT");
    return false;
  }


  if (peripheral.discoverAttributes()) {
    Serial.println("L:SERVO_DISCOVER_ATTRIBUTES");
  } else {
    Serial.println("L:SERVO_FAIL_TO_DISCOVER_ATTRIBUTES");
    peripheral.disconnect();
    return false;
  }

  BLECharacteristic servoCharac = peripheral.characteristic(SERVO_BLE_CHAR);

  if (!servoCharac) {
    Serial.println("L:SERVO_NOT_HAVE_CHARAC");
    peripheral.disconnect();
    return false;
  } else if (!servoCharac.canWrite()) {
    Serial.println("L:SERVO_CANNOT_WRITE");
    peripheral.disconnect();
    return false;
  }

  while (peripheral.connected()) {
    servoCharac.writeValue((byte)0x01);
    Serial.println("L:SERVO_WROTE_VALUE");
    peripheral.disconnect();
    Serial.println("L:SERVO_DISCONNECTED");
  }
  return true;
}




bool ble_scale_manage(BLEDevice peripheral) {

  Serial.println("L:STARTING_SCALE_MANAGEMENT");
  if (peripheral.connect()) {
    Serial.println("L:SCALE_CONNECTED");
  } else {
    Serial.println("L:SCALE_FAIL_TO_CONNECT");
    return false;
  }


  if (peripheral.discoverAttributes()) {
    Serial.println("L:SCALE_DISCOVER_ATTRIBUTES");
  } else {
    Serial.println("L:SCALE_FAIL_TO_DISCOVER_ATTRIBUTES");
    peripheral.disconnect();
    return false;
  }


  BLECharacteristic scaleCharac = peripheral.characteristic(SCALE_BLE_CHAR);

  if (!scaleCharac) {
    Serial.println("L:SCALE_NOT_HAVE_CHARAC");
    peripheral.disconnect();
    return false;
  }

  while (peripheral.connected()) {
    Serial.print("Scale:s_v:");
    byte floatBytes[4];
    scaleCharac.readValue(floatBytes, 4);
    float value;
    memcpy(&value, floatBytes, 4);
    Serial.println(value);
    peripheral.disconnect();
    Serial.println("L:SCALE_DISCONNECTED");
  }
  return true;
}

void loop() {

  if (Serial.available() > 0) {
    int read_byte = Serial.read();
    scan_for_servo_ble = false;
    scan_for_weight_ble = false;
    if (read_byte == 's') {
      Serial.println("L:Starting scan for Servo BLE");
      BLE.scanForUuid(SERVO_BLE_ID);
      scan_for_servo_ble = true;
    } else if (read_byte == 'w') {
      Serial.println("L:Starting scan for Weight BLE");
      BLE.scanForUuid(SCALE_BLE_ID);
      scan_for_weight_ble = true;
    }
  }
  if (scan_for_servo_ble) {
    Serial.println("L:Looking for Servo BLE");
    BLEDevice peripheral = BLE.available();

    if (peripheral) {
      // discovered a peripheral, print out address, local name, and advertised service
      Serial.print("L:FOUND UUID:");
      Serial.println(peripheral.advertisedServiceUuid());

      if (peripheral.advertisedServiceUuid() == SERVO_BLE_ID) {
        Serial.println("L:FOUND SERVO");
        scan_for_servo_ble = false;
        BLE.stopScan();
        ble_servo_manage(peripheral);
      }
      
    }
  }

  if (scan_for_weight_ble) {
    Serial.println("L:Looking for Weight BLE");
    BLEDevice peripheral = BLE.available();

    if (peripheral) {
      // discovered a peripheral, print out address, local name, and advertised service
      Serial.print("L:FOUND UUID:");
      Serial.println(peripheral.advertisedServiceUuid());


      if (peripheral.advertisedServiceUuid() == SCALE_BLE_ID) {
        Serial.println("L:FOUND WEIGHER");
        scan_for_weight_ble = false;
        BLE.stopScan();
        ble_scale_manage(peripheral);
      }
      
    }
  }

  if (samplesRead) {
    float rms = 0;
    for (int i = 0; i < samplesRead; i++) {
      rms += sampleBuffer[i] * sampleBuffer[i];
    }
    rms = sqrt(rms / samplesRead);
    if (SHOW_SENSOR_PRINT) {
      Serial.print("VOL:v:");
      Serial.println(rms);
    }

    samplesRead = 0;
  }

  if (IMU.accelerationAvailable()) {

    IMU.readAcceleration(a_x, a_y, a_z);

    if (SHOW_SENSOR_PRINT) {
      Serial.print("Accel:a_x:");
      Serial.println(a_x);

      Serial.print("Accel:a_y:");
      Serial.println(a_y);

      Serial.print("Accel:a_z:");
      Serial.println(a_z);
    }
  }

  if ((millis() - last_color) > 10000) {
    temperature = HS300x.readTemperature();
    humidity = HS300x.readHumidity();
    pressure = BARO.readPressure();
    if (SHOW_SENSOR_PRINT) {
      Serial.print("Env:e_T:");
      Serial.println(temperature);

      Serial.print("Env:e_H:");
      Serial.println(humidity);

      Serial.print("Env:e_P:");
      Serial.println(pressure);
    }




    APDS.disableGesture();
    APDS.enableColor();
    while (!APDS.colorAvailable()) {
      delay(5);
    }
    APDS.readColor(r, g, b);

    if (SHOW_SENSOR_PRINT) {
      Serial.print("RGB:rx:");
      Serial.println(r);

      Serial.print("RGB:gx:");
      Serial.println(g);

      Serial.print("RGB:bx:");
      Serial.println(b);
    }
    APDS.disableColor();
    APDS.enableGesture();
    last_color = millis();
  }

  if (APDS.gestureFIFOAvailable()) {
    uint8_t fifo_data[128];
    int available = APDS.gestureFIFOAvailable();
    if (available > 0) {
      uint8_t bytes_read = APDS.readGFIFO_U(fifo_data, available * 4);
      if (bytes_read > 0) {
        for (int i = 0; i + 3 < bytes_read; i += 4) {
          uint8_t u, d, l, r;
          u = fifo_data[i];
          d = fifo_data[i + 1];
          l = fifo_data[i + 2];
          r = fifo_data[i + 3];

          if (SHOW_SENSOR_PRINT) {
            Serial.print("IR:u:");
            Serial.println(u);

            Serial.print("IR:d:");
            Serial.println(d);

            Serial.print("IR:l:");
            Serial.println(l);

            Serial.print("IR:r:");
            Serial.println(r);
          }
        }
      }
    }
  }
}