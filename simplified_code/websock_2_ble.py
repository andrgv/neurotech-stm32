import asyncio
import json
import time

from bleak import BleakScanner, BleakClient
import websockets

SERVICE_UUID = "19B10010-E8F2-537E-4F6C-D104768A1214".lower()
CHAR_UUID = "19B10011-E8F2-537E-4F6C-D104768A1214"

WS_URL = "ws://127.0.0.1:8733/ws"

THRESHOLD = 0.35
DURATION = 30.0


async def main():
    # --- Find BLE device ---
    print("Scanning for device...")
    devices = await BleakScanner.discover(timeout=8.0, return_adv=True)

    device = None
    for _, (d, adv) in devices.items():
        uuids = [u.lower() for u in (adv.service_uuids or [])]
        if SERVICE_UUID in uuids:
            device = d
            break

    if device is None:
        print("Device not found.")
        return

    print("Found:", device.name)

    # --- Connect BLE ---
    async with BleakClient(device) as client:
        print("BLE connected")

        # --- Connect to EEG feature stream ---
        async with websockets.connect(WS_URL) as ws:
            print("Connected to feature stream")

            low_start = None
            buzzed = False

            while True:
                msg = json.loads(await ws.recv())

                if msg["type"] != "features":
                    continue

                focus = msg["focus"]
                now = time.monotonic()

                print("focus:", round(focus, 3))

                if focus < THRESHOLD:
                    if low_start is None:
                        low_start = now

                    if (now - low_start) > DURATION and not buzzed:
                        print("LOW ATTENTION → BUZZ")
                        await client.write_gatt_char(CHAR_UUID, bytearray([0x01]))
                        buzzed = True
                else:
                    # reset when attention comes back
                    low_start = None
                    buzzed = False


if __name__ == "__main__":
    asyncio.run(main())