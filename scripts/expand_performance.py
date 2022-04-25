#!/usr/bin/env python3

from argparse import ArgumentParser
import requests
import json
import time


# Enumerate Chassis and then perform $expand at SensorsCollection
def multi_request(host, port):
    start = time.monotonic()
    url = f"http://{host}:{port}"
    chassis_collection = json.loads(
        requests.get(url + "/redfish/v1/Chassis").content)
    for chassis_url in chassis_collection["Members"]:
        chassis = json.loads(requests.get(
            url + chassis_url["@odata.id"]).content)
        sensors_collection_url = chassis["Sensors"]["@odata.id"]
        sensor_in_chassis = json.loads(
            requests.get(url + sensors_collection_url + "?$expand=.").content
        )
    end = time.monotonic()
    print("Enumerate sensors in multiple requests: " +
          str(end - start) + " seconds")


# $expand + $select at ServiceRoot
def single_request(host, port):
    start = time.monotonic()
    url = f"http://{host}:{port}"
    all_sensors = json.loads(
        requests.get(
            url +
            "/redfish/v1/?$expand=.($levels=4)&$select=Chassis,Members,Sensors"
        ).content
    )
    end = time.monotonic()
    print("Enumerate sensors in a single request: " +
          str(end - start) + " seconds")


def main(args):
    multi_request(args.host, args.port)
    single_request(args.host, args.port)


if __name__ == "__main__":
    parser = ArgumentParser(
        description="Test RTT of querying all sensors on a machine")
    parser.add_argument("--host", default="localhost",
                        help="host to send request")
    parser.add_argument(
        "--port", default=5123, type=int, help="the port host is listening to"
    )
    args = parser.parse_args()

    main(args)
