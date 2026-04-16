import asyncio
from bleak import BleakScanner, BleakClient

SERVICE_UUID = "19B10010-E8F2-537E-4F6C-D104768A1214".lower()
CHAR_UUID = "19B10011-E8F2-537E-4F6C-D104768A1214"

async def find_device():
    devices = await BleakScanner.discover(timeout=8.0, return_adv=True)

    for _, (device, adv) in devices.items():
        uuids = [u.lower() for u in (adv.service_uuids or [])]
        if SERVICE_UUID in uuids:
            return device

    return None

async def send_buzz():
    device = await find_device()

    if device is None:
        print("Device with matching service UUID not found.")
        return

    print(f"Found device: name={device.name}, address={device.address}")

    async with BleakClient(device) as client:
        await client.write_gatt_char(CHAR_UUID, bytearray([0x01]))
        print("Buzz command sent.")

if __name__ == "__main__":
    asyncio.run(send_buzz())