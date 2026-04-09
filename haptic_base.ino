/*
  haptic_base.ino

  BLE-controlled DRV2605L demo for an Arduino Nano 33 BLE.
  The board talks to the DRV2605L over I2C and listens for
  BLE commands that tell it which haptic preset to play.

  Wiring for the DRV2605L breakout:
  - DRV2605L SDA -> Arduino A4
  - DRV2605L SCL -> Arduino A5
  - DRV2605L VIN -> Arduino 3.3V
  - DRV2605L GND -> Arduino GND
*/

#include <Wire.h>
#include <ArduinoBLE.h>
#include <Adafruit_DRV2605.h>

// Create the haptic driver object from the Adafruit library.
Adafruit_DRV2605 drv;

// This table maps simple BLE preset IDs (1-6) to DRV2605L waveform IDs.
// The first three entries match the EEG states we discussed earlier.
struct HapticPreset {
  uint8_t presetId;
  const char *stateName;
  const char *waveformName;
  uint8_t waveformId;
};

const HapticPreset presets[] = {
    {1, "relaxed", "Pulsing Strong 1", 52},
    {2, "transitioning", "Buzz 1", 47},
    {3, "focused", "Strong Buzz", 14},
    {4, "spare", "Soft Bump - 100%", 7},
    {5, "spare", "Strong Click - 100%", 1},
    {6, "spare", "750 ms Alert 100%", 15}
};

const size_t presetCount = sizeof(presets) / sizeof(presets[0]);

// Custom BLE service and characteristic UUIDs for the NeuroHaptic device.
BLEService hapticService("19B10010-E8F2-537E-4F6C-D104768A1214");
BLECharacteristic hapticCommandCharacteristic(
    "19B10011-E8F2-537E-4F6C-D104768A1214", BLEWrite | BLEWriteWithoutResponse, 2);

void haltWithError(const char *message);
const HapticPreset *findPreset(uint8_t presetId);
void handleBleCommand();
void triggerEffect(int effectID);

void setup() {
  // Start USB Serial so we can watch status messages in the Serial Monitor.
  Serial.begin(115200);

  // Give the Serial Monitor a moment to connect on boards that enumerate over USB.
  delay(1000);
  Serial.println("Starting NeuroHaptic BLE haptic demo...");

  // Start the I2C bus. The Nano 33 BLE uses the Wire library for I2C.
  // This project is wired with SDA on A4 and SCL on A5, powered from 3.3V.
  Wire.begin();

  // Initialize the DRV2605L and stop here if the device is not detected.
  if (!drv.begin()) {
    haltWithError("ERROR: DRV2605L initialization failed over I2C. Check SDA/A4, SCL/A5, 3.3V, and GND.");
  }

  // Configure the DRV2605L for a standard ERM motor and use waveform library 1.
  drv.useERM();
  drv.selectLibrary(1);
  drv.setMode(DRV2605_MODE_INTTRIG);

  Serial.println("DRV2605L initialized in ERM mode with library 1.");

  // Start BLE and advertise a writable command endpoint named NeuroHaptic.
  if (!BLE.begin()) {
    haltWithError("ERROR: BLE initialization failed. Reset the board and try again.");
  }

  BLE.setLocalName("NeuroHaptic");
  BLE.setDeviceName("NeuroHaptic");
  BLE.setAdvertisedService(hapticService);
  hapticService.addCharacteristic(hapticCommandCharacteristic);
  BLE.addService(hapticService);

  // Initialize the 2-byte characteristic to zeros before advertising.
  uint8_t initialCommand[2] = {0, 0};
  hapticCommandCharacteristic.writeValue(initialCommand, sizeof(initialCommand));

  BLE.advertise();

  Serial.println("BLE ready. Advertising as 'NeuroHaptic'.");
  Serial.println("Write 2 bytes to the command characteristic: [intensity 0-100, preset ID 1-6].");
}

void loop() {
  // Keep the BLE stack running and check for new writes from the central device.
  BLE.poll();
  handleBleCommand();
}

void haltWithError(const char *message) {
  Serial.println(message);

  // Halt so the board stays in a known state until power is cycled or reprogrammed.
  while (true) {
    delay(10);
  }
}

const HapticPreset *findPreset(uint8_t presetId) {
  for (size_t i = 0; i < presetCount; ++i) {
    if (presets[i].presetId == presetId) {
      return &presets[i];
    }
  }

  return nullptr;
}

void handleBleCommand() {
  if (!hapticCommandCharacteristic.written()) {
    return;
  }

  uint8_t command[2] = {0, 0};
  const int bytesRead = hapticCommandCharacteristic.readValue(command, sizeof(command));

  if (bytesRead != 2) {
    Serial.print("ERROR: Expected 2 BLE command bytes, received ");
    Serial.println(bytesRead);
    return;
  }

  const uint8_t intensity = command[0];
  const uint8_t presetId = command[1];

  Serial.print("Received BLE command -> intensity: ");
  Serial.print(intensity);
  Serial.print(", preset ID: ");
  Serial.println(presetId);

  if (intensity > 100) {
    Serial.println("WARNING: Intensity should be in the range 0-100. Value received will be ignored for now.");
  }

  triggerEffect(presetId);
}

void triggerEffect(int effectID) {
  // This helper now accepts a simple BLE preset ID (1-6) instead of a raw DRV2605L waveform ID.
  const HapticPreset *preset = findPreset(static_cast<uint8_t>(effectID));

  if (preset == nullptr) {
    Serial.print("ERROR: Unknown preset ID ");
    Serial.print(effectID);
    Serial.println(". Valid preset IDs are 1 through 6.");
    return;
  }

  Serial.print("Playing preset ");
  Serial.print(preset->presetId);
  Serial.print(" -> ");
  Serial.print(preset->stateName);
  Serial.print(" / ");
  Serial.print(preset->waveformName);
  Serial.print(" (DRV2605L waveform ID ");
  Serial.print(preset->waveformId);
  Serial.println(")");

  // Slot 0 is the first waveform slot. Slot 1 is set to 0 to mark the end.
  drv.setWaveform(0, preset->waveformId);
  drv.setWaveform(1, 0);

  // Trigger the effect immediately using internal trigger mode.
  drv.go();
}
