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

  Wiring for the 10 mm ERM vibration motor disk:
  - Motor lead 1 -> DRV2605L OUT+
  - Motor lead 2 -> DRV2605L OUT-

  Breadboard note:
  - Put the DRV2605L breakout across the breadboard center gap if possible.
  - Run Arduino 3.3V and GND to the breadboard power rails.
  - Run SDA/SCL from the Arduino to the DRV2605L breakout pins.
  - Connect the motor disk directly to the DRV2605L output pads/pins, not to
    the Arduino GPIO pins.
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
  uint8_t waveformSoft;
  uint8_t waveformMedium;
  uint8_t waveformStrong;
};

const HapticPreset presets[] = {
    {1, "relaxed", "Soft / Medium / Strong pulse", 47, 51, 52},
    {2, "transitioning", "Soft / Medium / Strong buzz", 46, 47, 48},
    {3, "focused", "Soft / Medium / Strong buzz", 12, 14, 16},
    {4, "spare", "Soft / Medium / Strong bump", 7, 10, 13},
    {5, "spare", "Soft / Medium / Strong click", 1, 4, 6},
    {6, "spare", "Soft / Medium / Strong alert", 15, 16, 17}
};

const size_t presetCount = sizeof(presets) / sizeof(presets[0]);

// Custom BLE service and characteristic UUIDs for the NeuroHaptic device.
BLEService hapticService("19B10010-E8F2-537E-4F6C-D104768A1214");
BLECharacteristic hapticCommandCharacteristic(
    "19B10011-E8F2-537E-4F6C-D104768A1214", BLEWrite | BLEWriteWithoutResponse, 2);

void haltWithError(const char *message);
const HapticPreset *findPreset(uint8_t presetId);
void handleBleCommand();
uint8_t resolveWaveformId(const HapticPreset *preset, uint8_t intensity);
void triggerEffect(int effectID, uint8_t intensity);

void setup() {
  // Old Serial Monitor debug path kept for reference:
  // Serial.begin(115200);
  // unsigned long serialStart = millis();
  // while (!Serial && millis() - serialStart < 4000) {
  //   delay(10);
  // }
  // delay(250);

  // Serial.println("Starting NeuroHaptic BLE haptic demo...");

  // Start the I2C bus. The Nano 33 BLE uses the Wire library for I2C.
  // This project is wired with SDA and SCL, powered from 3.3V.
  Wire.begin();

  // Initialize the DRV2605L and stop here if the device is not detected.
  if (!drv.begin()) {
    haltWithError("ERROR: DRV2605L initialization failed over I2C. Check SDA/SCL, 3.3V, and GND.");
  }

  // Configure the DRV2605L for a standard ERM motor and use waveform library 1.
  drv.useERM();
  drv.selectLibrary(1);
  drv.setMode(DRV2605_MODE_INTTRIG);

  // Serial.println("DRV2605L initialized in ERM mode with library 1.");

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

  // Serial.println("BLE ready. Advertising as 'NeuroHaptic'.");
  // Serial.println("Write 2 bytes to the command characteristic: [intensity 0-100, preset ID 1-6].");
}

void loop() {
  // Keep the BLE stack running and check for new writes from the central device.
  BLE.poll();
  handleBleCommand();
}

void haltWithError(const char *message) {
  // Old debug output kept for reference:
  // Serial.println(message);

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
    // Serial.print("ERROR: Expected 2 BLE command bytes, received ");
    // Serial.println(bytesRead);
    return;
  }

  uint8_t intensity = command[0];
  const uint8_t presetId = command[1];

  // Serial.print("Received BLE command -> intensity: ");
  // Serial.print(intensity);
  // Serial.print(", preset ID: ");
  // Serial.println(presetId);

  if (intensity > 100) {
    // Serial.println("WARNING: Intensity above 100 was clamped.");
    intensity = 100;
  }

  triggerEffect(presetId, intensity);
}

uint8_t resolveWaveformId(const HapticPreset *preset, uint8_t intensity) {
  if (intensity < 34) {
    return preset->waveformSoft;
  }
  if (intensity < 67) {
    return preset->waveformMedium;
  }
  return preset->waveformStrong;
}

void triggerEffect(int effectID, uint8_t intensity) {
  // This helper now accepts a BLE preset ID plus intensity and resolves it to
  // a motor waveform for the DRV2605L.
  const HapticPreset *preset = findPreset(static_cast<uint8_t>(effectID));

  if (preset == nullptr) {
    // Serial.print("ERROR: Unknown preset ID ");
    // Serial.print(effectID);
    // Serial.println(". Valid preset IDs are 1 through 6.");
    return;
  }

  const uint8_t waveformId = resolveWaveformId(preset, intensity);

  // Serial.print("Playing preset ");
  // Serial.print(preset->presetId);
  // Serial.print(" -> ");
  // Serial.print(preset->stateName);
  // Serial.print(" / ");
  // Serial.print(preset->waveformName);
  // Serial.print(" (DRV2605L waveform ID ");
  // Serial.print(waveformId);
  // Serial.println(")");

  // Slot 0 is the first waveform slot. Slot 1 is set to 0 to mark the end.
  drv.setWaveform(0, waveformId);
  drv.setWaveform(1, 0);

  // Trigger the effect immediately using internal trigger mode.
  drv.go();
}
