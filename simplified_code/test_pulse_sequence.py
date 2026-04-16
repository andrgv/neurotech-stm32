import argparse
import asyncio
import threading
import time

from bleak import BleakClient, BleakScanner

try:
    import serial
except ImportError:
    serial = None


DEVICE_NAME = "NeuroHaptic"
SERVICE_UUID = "19B10010-E8F2-537E-4F6C-D104768A1214".lower()
CHAR_UUID = "19B10011-E8F2-537E-4F6C-D104768A1214"

COMMANDS = {
    "low": 0x01,
    "medium": 0x02,
    "high": 0x03,
}

WAVEFORMS = {
    "low": 46,
    "medium": 47,
    "high": 48,
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Send a low/medium/high pulse sequence to the NeuroHaptic BLE device."
    )
    parser.add_argument(
        "--sequence",
        default="low,medium,high",
        help="Comma-separated pulse labels. Valid values: low, medium, high",
    )
    parser.add_argument(
        "--repeat",
        type=int,
        default=2,
        help="How many times to run the sequence",
    )
    parser.add_argument(
        "--delay",
        type=float,
        default=1.5,
        help="Seconds to wait between pulses",
    )
    parser.add_argument(
        "--scan-timeout",
        type=float,
        default=5.0,
        help="BLE scan timeout in seconds",
    )
    parser.add_argument(
        "--serial-port",
        default="",
        help="Optional serial port, for example COM5, to print live Arduino output",
    )
    parser.add_argument(
        "--baudrate",
        type=int,
        default=115200,
        help="Serial baud rate when --serial-port is used",
    )
    return parser.parse_args()


def normalize_sequence(raw_sequence: str) -> list[str]:
    sequence = [item.strip().lower() for item in raw_sequence.split(",") if item.strip()]
    invalid = [item for item in sequence if item not in COMMANDS]

    if invalid:
        raise ValueError(
            f"Unsupported pulse labels: {', '.join(invalid)}. Use only low, medium, high."
        )
    if not sequence:
        raise ValueError("Sequence cannot be empty.")

    return sequence


async def find_device_by_service(timeout: float):
    devices = await BleakScanner.discover(timeout=timeout, return_adv=True)

    for _, (device, adv) in devices.items():
        uuids = [u.lower() for u in (adv.service_uuids or [])]
        if SERVICE_UUID in uuids:
            return device

    return None


def theoretical_serial_output(label: str) -> list[str]:
    cmd = COMMANDS[label]
    waveform = WAVEFORMS[label]
    return [
        f"[expected-serial] Received command: {cmd}",
        f"[expected-serial] Buzz triggered -> {label.upper()} intensity using waveform {waveform}",
    ]


class SerialTail:
    def __init__(self, port: str, baudrate: int) -> None:
        if serial is None:
            raise RuntimeError(
                "pyserial is not installed. Run 'pip install pyserial' or omit --serial-port."
            )
        self._port = port
        self._baudrate = baudrate
        self._serial = None
        self._thread = None
        self._stop = threading.Event()

    def start(self) -> None:
        self._serial = serial.Serial(self._port, self._baudrate, timeout=0.2)
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def _run(self) -> None:
        while not self._stop.is_set():
            if self._serial is None:
                return
            line = self._serial.readline()
            if not line:
                continue
            decoded = line.decode("utf-8", errors="replace").rstrip()
            if decoded:
                print(f"[serial] {decoded}")

    def stop(self) -> None:
        self._stop.set()
        if self._thread is not None:
            self._thread.join(timeout=1.0)
        if self._serial is not None:
            self._serial.close()


async def run_sequence(
    sequence: list[str],
    repeat: int,
    delay: float,
    scan_timeout: float,
) -> None:
    device = await find_device_by_service(timeout=scan_timeout)

    if device is None:
        print("Device with matching service UUID not found.")
        return

    print(f"Found device: {device.name} ({device.address})")

    async with BleakClient(device) as client:
        print("Connected:", client.is_connected)

        for cycle in range(1, repeat + 1):
            print(f"\n=== Test cycle {cycle}/{repeat} ===")
            for label in sequence:
                cmd = COMMANDS[label]
                waveform = WAVEFORMS[label]
                print(
                    f"Sending {label.upper()} pulse -> command=0x{cmd:02X}, theoretical waveform={waveform}"
                )
                for line in theoretical_serial_output(label):
                    print(line)
                await client.write_gatt_char(CHAR_UUID, bytearray([cmd]))
                await asyncio.sleep(delay)


async def main() -> None:
    args = parse_args()
    sequence = normalize_sequence(args.sequence)
    tail = None

    if args.serial_port:
        tail = SerialTail(args.serial_port, args.baudrate)
        tail.start()
        print(f"Streaming live serial from {args.serial_port} @ {args.baudrate} baud")
        time.sleep(1.0)
    else:
        print("No serial port provided; printing theoretical Serial Monitor output only.")

    try:
        await run_sequence(
            sequence=sequence,
            repeat=max(1, args.repeat),
            delay=max(0.1, args.delay),
            scan_timeout=max(1.0, args.scan_timeout),
        )
    finally:
        if tail is not None:
            tail.stop()


if __name__ == "__main__":
    asyncio.run(main())