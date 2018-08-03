import json
import ssl
import websocket

websocket.enableTrace(True)

ws = websocket.create_connection('wss://10.243.48.93:18080/subscribe',
                       sslopt={"cert_reqs": ssl.CERT_NONE},
                       cookie="XSRF-TOKEN=m0KhYNbxFmUEI4Sr1I22; SESSION=0mdwzoQy3gggQxW3vrEw")
request = json.dumps({
    "paths": ["/xyz/openbmc_project/logging", "/xyz/openbmc_project/sensors"],
    "interfaces": ["xyz.openbmc_project.Logging.Entry", "xyz.openbmc_project.Sensor.Value"]
})

ws.send(request)
print("Sent")
print("Receiving...")
while True:
    result = ws.recv()
    print("Received '%s'" % result)
ws.close()
