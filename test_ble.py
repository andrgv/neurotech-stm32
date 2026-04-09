import asyncio
import sys

from bleak import BleakClient, BleakScanner


DEVICE_NAME = "NeuroHaptic"
COMMAND_CHARACTERISTIC_UUID = "19B10011-E8F2-537E-4F6C-D104768A1214"
SCAN_TIMEOUT_SECONDS = 10.0
PACKET_GAP_SECONDS = 3.0

TEST_PACKETS = [
    ([50, 1], "relaxed, medium intensity"),
    ([75, 2], "transitioning, high intensity"),
    ([100, 3], "focused, full intensity"),
    ([25, 1], "relaxed, low intensity"),
    ([0, 1], "off"),
]


async def find_device_by_name(name: str):
    print(f"Scanning for BLE device '{name}'...")
    devices = await BleakScanner.discover(timeout=SCAN_TIMEOUT_SECONDS)

    for device in devices:
        if device.name == name:
            print(f"Found '{name}' at address {device.address}")
            return device

    return None


async def send_test_packets():
    device = await find_device_by_name(DEVICE_NAME)
    if device is None:
        raise RuntimeError(
            f"Could not find a BLE device advertising as '{DEVICE_NAME}'. "
            "Make sure the Arduino is powered on and advertising."
        )

    async with BleakClient(device) as client:
        if not client.is_connected:
            raise RuntimeError(f"Connection attempt to '{DEVICE_NAME}' failed.")

        print(f"Connected to '{DEVICE_NAME}'.")

        for index, (packet, label) in enumerate(TEST_PACKETS, start=1):
            payload = bytearray(packet)
            await client.write_gatt_char(COMMAND_CHARACTERISTIC_UUID, payload)
            print(f"Sent packet {index}/5: {packet} -> {label}")

            if index < len(TEST_PACKETS):
                await asyncio.sleep(PACKET_GAP_SECONDS)

        print("All test packets sent.")


async def main():
    try:
        await send_test_packets()
    except Exception as exc:
        print(f"BLE test failed: {exc}", file=sys.stderr)
        raise


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except Exception:
        sys.exit(1)
