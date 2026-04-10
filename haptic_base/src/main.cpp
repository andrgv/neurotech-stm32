/*
  main.cpp

  BLE-controlled DRV2605L demo for an Arduino Nano 33 BLE.
  The board talks to the DRV2605L over I2C and listens for
  BLE commands that tell it which haptic preset to play.

  Wiring for the DRV2605L breakout:
  - DRV2605L SDA -> Arduino SDA
  - DRV2605L SCL -> Arduino SCL
  - DRV2605L VIN -> Arduino 3.3V
  - DRV2605L GND -> Arduino GND
*/

#include <Arduino.h>
#include <ArduinoBLE.h>
#include <Adafruit_DRV2605.h>
#include <Wire.h>

Adafruit_DRV2605 drv;

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
    {6, "spare", "750 ms Alert 100%", 15},
};

const size_t presetCount = sizeof(presets) / sizeof(presets[0]);

BLEService hapticService("19B10010-E8F2-537E-4F6C-D104768A1214");
BLECharacteristic hapticCommandCharacteristic(
    "19B10011-E8F2-537E-4F6C-D104768A1214", BLEWrite | BLEWriteWithoutResponse, 2);

void haltWithError(const char *message);
const HapticPreset *findPreset(uint8_t presetId);
void handleBleCommand();
void triggerEffect(int effectID);

void setup() {
  Serial.begin(115200);

  // Give USB Serial a short window to attach so startup logs are easier to catch.
  const unsigned long serialStart = millis();
  while (!Serial && millis() - serialStart < 4000) {
    delay(10);
  }
  delay(250);

  Serial.println("Starting NeuroHaptic BLE haptic demo...");

  Wire.begin();

  if (!drv.begin()) {
    haltWithError("ERROR: DRV2605L initialization failed over I2C. Check SDA/SCL, 3.3V, and GND.");
  }

  drv.useERM();
  drv.selectLibrary(1);
  drv.setMode(DRV2605_MODE_INTTRIG);

  Serial.println("DRV2605L initialized in ERM mode with library 1.");

  if (!BLE.begin()) {
    haltWithError("ERROR: BLE initialization failed. Reset the board and try again.");
  }

  BLE.setLocalName("NeuroHaptic");
  BLE.setDeviceName("NeuroHaptic");
  BLE.setAdvertisedService(hapticService);
  hapticService.addCharacteristic(hapticCommandCharacteristic);
  BLE.addService(hapticService);

  uint8_t initialCommand[2] = {0, 0};
  hapticCommandCharacteristic.writeValue(initialCommand, sizeof(initialCommand));

  BLE.advertise();

  Serial.println("BLE ready. Advertising as 'NeuroHaptic'.");
  Serial.println("Write 2 bytes to the command characteristic: [intensity 0-100, preset ID 1-6].");
}

void loop() {
  BLE.poll();
  handleBleCommand();
}

void haltWithError(const char *message) {
  Serial.println(message);

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

  drv.setWaveform(0, preset->waveformId);
  drv.setWaveform(1, 0);
  drv.go();
}
