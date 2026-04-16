#include <Wire.h>
#include <ArduinoBLE.h>
#include <Adafruit_DRV2605.h>

Adafruit_DRV2605 drv;

// BLE service + one-byte writable characteristic
BLEService hapticService("19B10010-E8F2-537E-4F6C-D104768A1214");
BLEByteCharacteristic buzzCharacteristic(
  "19B10011-E8F2-537E-4F6C-D104768A1214",
  BLEWrite | BLEWriteWithoutResponse
);

// Fixed haptic effect
// From the DRV2605L library, 47 is a reasonable "Buzz 1" style effect.
const uint8_t BUZZ_WAVEFORM_ID = 47;

void haltWithError(const char *message) {
  Serial.println(message);
  while (1) {
    delay(10);
  }
}

void triggerBuzz() {
  drv.setWaveform(0, BUZZ_WAVEFORM_ID);
  drv.setWaveform(1, 0);   // end sequence
  drv.go();
  Serial.println("Buzz triggered");
}

void setup() {
  Serial.begin(115200);
  unsigned long start = millis();
  while (!Serial && millis() - start < 4000) {
    delay(10);
  }

  Serial.println("Starting BLE haptic receiver...");

  // I2C init
  Wire.begin();

  // DRV2605L init
  if (!drv.begin()) {
    haltWithError("ERROR: DRV2605L not found. Check SDA/SCL, power, and ground.");
  }

  drv.useERM();                       // use if you're driving an ERM motor
  drv.selectLibrary(1);
  drv.setMode(DRV2605_MODE_INTTRIG);

  Serial.println("DRV2605L initialized.");

  // BLE init
  if (!BLE.begin()) {
    haltWithError("ERROR: BLE failed to start.");
  }

  BLE.setLocalName("NeuroHaptic");
  BLE.setAdvertisedService(hapticService);

  hapticService.addCharacteristic(buzzCharacteristic);
  BLE.addService(hapticService);

  buzzCharacteristic.writeValue((byte)0x00);

  BLE.advertise();

  Serial.println("BLE ready. Device name: NeuroHaptic");
  Serial.println("Write 0x01 to the characteristic to trigger one buzz.");
}

void loop() {
  BLE.poll();

  if (buzzCharacteristic.written()) {
    byte cmd = buzzCharacteristic.value();

    Serial.print("Received command: ");
    Serial.println(cmd);

    if (cmd == 0x01) {
      triggerBuzz();
    }
  }
}