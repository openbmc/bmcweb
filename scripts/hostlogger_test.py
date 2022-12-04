#!/usr/bin/env python3

# Simple script to expose host serial console logs
# Search and get the log via redfish in every 5 seconds

import argparse
import json
import logging
import time
import traceback

import requests

parser = argparse.ArgumentParser()
parser.add_argument("--host", help="Host to connect to", required=True)
parser.add_argument("--cert", help="File path to cert", required=True)
parser.add_argument(
    "--username", help="Username to connect with", default="root"
)
parser.add_argument("--password", help="Password to use", default="0penBmc")

args = parser.parse_args()


def requests_get(url):
    try:
        resp = requests.get(
            url=url,
            cert=args.cert,
            verify=False,
            headers={"Cache-Control": "no-cache"},
            timeout=5,
        )
        data = resp.json()

        if resp.status_code != requests.codes.ok:
            print(
                "There occurs error when get request, status_code = "
                + str(resp.status_code)
                + "\n"
            )
            print(json.dumps(data, indent=4, sort_keys=True))
            pass

        return data

    except Exception:
        traceback.print_exc()
        pass


def label_parser(url, label):
    data = requests_get(url)
    for key in sorted(data.keys()):
        if key == label:
            content = data[key]
            break
    return content


def main():
    logging.captureWarnings(True)
    totalEntryUri = (
        f"https://{args.host}/redfish/v1/Systems/system/"
        + "LogServices/HostLogger/Entries/"
    )
    id = 0
    entryCount = 0
    message = ""

    while 1:
        entryCount = label_parser(totalEntryUri, "Members@odata.count")
        if id == entryCount:
            # entryCount equals to ID means there has no change during the
            # interval, sleep 5 seconds for next search.
            time.sleep(5)
        elif id < entryCount:
            # print new entries which created in this interval.
            for i in range(id + 1, entryCount):
                singleEntryUri = (
                    "https://{}/redfish/v1/Systems/system/LogServices/"
                    "HostLogger/Entries/{}".format(args.host, i)
                )
                message = label_parser(singleEntryUri, "Message")
                # need to present all special characters, so use "repr"
                # function
                print(repr(message))
            id = entryCount


main()
