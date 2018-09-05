import json
import pprint
import sseclient
import requests
import sys
import http.client as http_client
import logging


http_client.HTTPConnection.debuglevel = 5
logging.basicConfig()
logging.getLogger().setLevel(logging.DEBUG)
requests_log = logging.getLogger("requests.packages.urllib3")
requests_log.setLevel(logging.DEBUG)
requests_log.propagate = True


url = 'https://10.243.48.93:18080/sse/'
response = requests.get(url, stream=True, verify=False,
                        auth=("root", "0penBmc"), headers={"Accept": "text/event-stream"})

#for line in response.iter_lines(decode_unicode=True):
#    if line:
#        print(line)


client = sseclient.SSEClient(response)

for event in client.events():
    print(event.data)
