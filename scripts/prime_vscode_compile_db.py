#! /usr/bin/python3

import json
import shlex
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
VS_CODE_FILE = os.path.join(SCRIPT_DIR, "../.vscode/c_cpp_properties.json")

def main():
    """ Main function"""
    if os.path.exists(VS_CODE_FILE):
        unique_includes = list(set(sys.argv[1:]))

        print("Adding {} includes to c_cpp_properties.json".format(len(unique_includes)))
        with open(VS_CODE_FILE) as vscodefile:
            vscode_data = json.load(vscodefile)

        for config in vscode_data["configurations"]:
            if config["name"] in "Linux":
                config["includePath"] = unique_includes


        with open(VS_CODE_FILE, 'w') as vscodefile:
            json.dump(vscode_data, vscodefile, sort_keys=True, indent=4)

if __name__ == "__main__":
    main()
