from __future__ import annotations

import asyncio
import logging
import time
from dataclasses import dataclass
from typing import Optional

import src.constants as const

logger = logging.getLogger(__name__)

try:
    from bleak import BleakClient, BleakScanner
except ImportError:  # pragma: no cover - exercised only when bleak is missing
    BleakClient = None
    BleakScanner = None


@dataclass(frozen=True)
class HapticCommand:
    intensity: int
    preset_id: int
    mood: str


def _clamp_unit(value: float) -> float:
    return max(0.0, min(1.0, float(value)))


def _clamp_intensity(value: float) -> int:
    return max(0, min(100, int(round(value))))


class NeuroHapticBridge:
    """Small synchronous facade that sends BLE haptic commands to the Nano 33 BLE.

    The main EEG loop is synchronous, so this class handles scanning and a single
    BLE write on demand. It intentionally rate-limits writes so we only resend when
    the resolved mood changes or the requested intensity changes meaningfully.
    """

    def __init__(self) -> None:
        self.enabled = bool(const.HAPTICS_ENABLE)
        self.device_name = const.HAPTICS_DEVICE_NAME
        self.characteristic_uuid = const.HAPTICS_CHARACTERISTIC_UUID
        self.scan_timeout_seconds = float(const.HAPTICS_SCAN_TIMEOUT_SECONDS)
        self.send_min_interval_seconds = float(const.HAPTICS_SEND_MIN_INTERVAL_SECONDS)
        self.min_intensity_delta = int(const.HAPTICS_MIN_INTENSITY_DELTA)
        self._last_command: Optional[HapticCommand] = None
        self._last_send_at: float = 0.0
        self._warned_missing_bleak = False
        self._preset_by_mood = {
            "calm": int(const.HAPTICS_PRESET_CALM),
            "focus": int(const.HAPTICS_PRESET_FOCUS),
            "deep_focus": int(const.HAPTICS_PRESET_DEEP_FOCUS),
            "hype": int(const.HAPTICS_PRESET_HYPE),
        }

    def update(self, *, mood: str, energy: float, focus: float) -> None:
        if not self.enabled:
            return

        if BleakClient is None or BleakScanner is None:
            if not self._warned_missing_bleak:
                logger.warning(
                    "Haptics enabled, but the bleak package is not installed. "
                    "Install requirements.txt to send BLE haptic commands."
                )
                self._warned_missing_bleak = True
            return

        command = self._build_command(mood=mood, energy=energy, focus=focus)
        if command is None or self._should_skip(command):
            return

        try:
            asyncio.run(self._send_command(command))
            self._last_command = command
            self._last_send_at = time.monotonic()
            logger.info(
                "Haptics -> mood=%s intensity=%d preset=%d",
                command.mood,
                command.intensity,
                command.preset_id,
            )
        except Exception as exc:
            logger.warning("Haptic BLE send failed: %s", exc)

    def _build_command(self, *, mood: str, energy: float, focus: float) -> Optional[HapticCommand]:
        preset_id = self._preset_by_mood.get(mood)
        if preset_id is None:
            return None

        energy_unit = _clamp_unit(energy)
        focus_unit = _clamp_unit(focus)

        # Keep intensity intuitive for demos: calm stays lighter, focus ramps up,
        # and hype lands in the middle preset unless you override the mapping.
        floor_by_mood = {
            "calm": 20,
            "focus": 55,
            "deep_focus": 80,
            "hype": 60,
        }
        ceiling_by_mood = {
            "calm": 55,
            "focus": 85,
            "deep_focus": 100,
            "hype": 95,
        }

        blended = _clamp_intensity((0.55 * energy_unit + 0.45 * focus_unit) * 100.0)
        intensity = max(floor_by_mood[mood], min(ceiling_by_mood[mood], blended))
        return HapticCommand(intensity=intensity, preset_id=preset_id, mood=mood)

    def _should_skip(self, command: HapticCommand) -> bool:
        if self._last_command is None:
            return False

        if command.preset_id != self._last_command.preset_id:
            return False

        if abs(command.intensity - self._last_command.intensity) >= self.min_intensity_delta:
            return False

        return (time.monotonic() - self._last_send_at) < self.send_min_interval_seconds

    async def _send_command(self, command: HapticCommand) -> None:
        devices = await BleakScanner.discover(timeout=self.scan_timeout_seconds)
        device = next((d for d in devices if d.name == self.device_name), None)
        if device is None:
            raise RuntimeError(
                f"Could not find a BLE device advertising as '{self.device_name}'."
            )

        async with BleakClient(device) as client:
            if not client.is_connected:
                raise RuntimeError(f"Connection attempt to '{self.device_name}' failed.")

            payload = bytearray([command.intensity, command.preset_id])
            await client.write_gatt_char(self.characteristic_uuid, payload, response=True)
