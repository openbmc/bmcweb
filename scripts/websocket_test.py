import asyncio
import websockets
import ssl

ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
ssl_context.check_hostname = False
ssl_context.verify_mode = ssl.CERT_NONE


async def hello():
    async with websockets.connect(
            'wss://10.243.48.21:18080/asdjtag0', ssl=ssl_context, extra_headers={"Authorization": "Basic cm9vdDowcGVuQm1j"}) as websocket:
        greeting = await websocket.recv()
        print(greeting)
        await websocket.send("foobar")


asyncio.get_event_loop().run_until_complete(hello())
