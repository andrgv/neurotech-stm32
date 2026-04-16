import asyncio
from bleak import BleakScanner, BleakClient

DEVICE_NAME = "NeuroHaptic"
CHAR_UUID = "19B10011-E8F2-537E-4F6C-D104768A1214"

async def find_device_by_name(name: str):
    devices = await BleakScanner.discover(timeout=5.0)
    for device in devices:
        if device.name == name:
            return device
    return None

async def send_buzz():
    device = await find_device_by_name(DEVICE_NAME)

    if device is None:
        print(f"Device '{DEVICE_NAME}' not found.")
        return

    print(f"Found device: {device.name} ({device.address})")

    async with BleakClient(device) as client:
        print("Connected:", client.is_connected)
        await client.write_gatt_char(CHAR_UUID, bytearray([0x01]))
        print("Buzz command sent.")

if __name__ == "__main__":
    asyncio.run(send_buzz())