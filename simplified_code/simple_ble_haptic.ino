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

// Command bytes for quick bench testing over BLE.
const uint8_t CMD_LOW = 0x01;
const uint8_t CMD_MEDIUM = 0x02;
const uint8_t CMD_HIGH = 0x03;

// Simple intensity tiers. These are still canned DRV2605L effects, not true
// analog amplitude control, but they are useful for motor bring-up.
const uint8_t BUZZ_WAVEFORM_LOW = 46;
const uint8_t BUZZ_WAVEFORM_MEDIUM = 47;
const uint8_t BUZZ_WAVEFORM_HIGH = 48;

void haltWithError(const char *message) {
  Serial.println(message);
  while (1) {
    delay(10);
  }
}

const char *commandLabel(byte cmd) {
  switch (cmd) {
    case CMD_LOW:
      return "LOW";
    case CMD_MEDIUM:
      return "MEDIUM";
    case CMD_HIGH:
      return "HIGH";
    default:
      return "UNKNOWN";
  }
}

uint8_t waveformForCommand(byte cmd) {
  switch (cmd) {
    case CMD_LOW:
      return BUZZ_WAVEFORM_LOW;
    case CMD_MEDIUM:
      return BUZZ_WAVEFORM_MEDIUM;
    case CMD_HIGH:
      return BUZZ_WAVEFORM_HIGH;
    default:
      return 0;
  }
}

void triggerBuzz(byte cmd) {
  uint8_t waveformId = waveformForCommand(cmd);
  if (waveformId == 0) {
    Serial.println("Unknown command. Use 0x01 (low), 0x02 (medium), or 0x03 (high).");
    return;
  }

  drv.setWaveform(0, waveformId);
  drv.setWaveform(1, 0);   // end sequence
  drv.go();

  Serial.print("Buzz triggered -> ");
  Serial.print(commandLabel(cmd));
  Serial.print(" intensity using waveform ");
  Serial.println(waveformId);
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
  Serial.println("Write 0x01/0x02/0x03 for low/medium/high pulses.");
}

void loop() {
  BLE.poll();

  if (buzzCharacteristic.written()) {
    byte cmd = buzzCharacteristic.value();

    Serial.print("Received command: ");
    Serial.println(cmd);

    triggerBuzz(cmd);
  }
}
