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

struct HapticPattern {
  const char *label;
  const uint8_t *sequence;
  uint8_t length;
  const char *summary;
};

// Use clearly different patterns instead of neighboring waveform IDs so the
// coin motor feels more distinct during bench testing.
const uint8_t LOW_SEQUENCE[] = { 7, 0 };                 // one soft bump
const uint8_t MEDIUM_SEQUENCE[] = { 47, 0, 47, 0 };      // two medium buzzes
const uint8_t HIGH_SEQUENCE[] = { 1, 47, 1, 47, 0 };     // repeated strong click + buzz

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

const HapticPattern *patternForCommand(byte cmd) {
  static const HapticPattern low = { "LOW", LOW_SEQUENCE, 2, "single soft bump" };
  static const HapticPattern medium = { "MEDIUM", MEDIUM_SEQUENCE, 4, "double medium buzz" };
  static const HapticPattern high = { "HIGH", HIGH_SEQUENCE, 5, "strong repeated click-buzz" };

  switch (cmd) {
    case CMD_LOW:
      return &low;
    case CMD_MEDIUM:
      return &medium;
    case CMD_HIGH:
      return &high;
    default:
      return nullptr;
  }
}

void triggerBuzz(byte cmd) {
  const HapticPattern *pattern = patternForCommand(cmd);
  if (pattern == nullptr) {
    Serial.println("Unknown command. Use 0x01 (low), 0x02 (medium), or 0x03 (high).");
    return;
  }

  for (uint8_t i = 0; i < pattern->length; ++i) {
    drv.setWaveform(i, pattern->sequence[i]);
  }
  for (uint8_t i = pattern->length; i < 8; ++i) {
    drv.setWaveform(i, 0);
  }
  drv.go();

  Serial.print("Buzz triggered -> ");
  Serial.print(pattern->label);
  Serial.print(" intensity using pattern: ");
  Serial.println(pattern->summary);
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
