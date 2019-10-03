#!/usr/bin/env python3

# Example of streaming sensor values from openbmc using the /subscribe api
# requires websockets package to be installed

import asyncio
import ssl
import websockets
import base64
import json
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--host", help="Host to connect to", required=True)
parser.add_argument(
    "--username", help="Username to connect with", default="root")
parser.add_argument("--password", help="Password to use", default="0penBmc")

args = parser.parse_args()

sensor_type_map = {
    "voltage": "Volts",
    "power": "Watts",
    "fan": "RPM",
    "fan_tach": "RPM",
    "temperature": "Degrees C",
    "altitude": "Meters",
    "current": "Amps",
    "energy": "Joules",
    "cfm": "CFM"
}


async def hello():
    uri = 'wss://{}/subscribe'.format(args.host)
    ssl_context = ssl.SSLContext()
    authbytes = "{}:{}".format(args.username, args.password).encode('ascii')
    auth = "Basic {}".format(base64.b64encode(authbytes).decode('ascii'))
    headers = {"Authorization": auth}
    async with websockets.connect(uri, ssl=ssl_context, extra_headers=headers) as websocket:
        request = json.dumps({
            "paths": ["/xyz/openbmc_project/sensors"],
            "interfaces": ["xyz.openbmc_project.Sensor.Value"]
        })
        await websocket.send(request)

        while True:
            payload = await websocket.recv()
            j = json.loads(payload)
            path = j.get("path", "unknown/unknown")
            name = path.split("/")[-1]
            sensor_type = path.split("/")[-2]
            units = sensor_type_map.get(sensor_type, "")
            properties = j.get("properties", [])
            value = properties.get("Value", None)
            if value is None:
                continue
            print(f"{name:<20} {value:4.02f} {units}")

asyncio.get_event_loop().run_until_complete(hello())
