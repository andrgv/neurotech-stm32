import asyncio
from bleak import BleakScanner

async def main():
    devices = await BleakScanner.discover(timeout=5.0)
    for d in devices:
        print(f"name={d.name}, address={d.address}")

if __name__ == "__main__":
    asyncio.run(main())