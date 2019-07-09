#!/usr/bin/python3
import requests
import zipfile
from io import BytesIO
import os
from collections import defaultdict
from collections import OrderedDict
from distutils.version import StrictVersion
import shutil
import json
import glob
import subprocess

import xml.etree.ElementTree as ET

REGISTRY_HEADER = '''/*
// Copyright (c) 2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
/****************************************************************
 * This is an auto-generated header which contains definitions
 * for Redfish DMTF defined messages.
 ***************************************************************/
#pragma once
#include <registries.hpp>

namespace redfish::message_registries::{}
{{
'''

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

include_path = os.path.realpath(os.path.join(SCRIPT_DIR, "..", "redfish-core", "include", "registries"))

proxies = {
    'https': os.environ.get("https_proxy", None)
}

base_file = requests.get('https://redfish.dmtf.org/registries/Base.1.4.0.json', proxies=proxies)
base_file.raise_for_status()
base_json = json.loads(base_file.text)
base_path = os.path.join(include_path, "base_message_registry.hpp")

files = [(base_path, base_json, "base")]

# Remove the old files
for file, json, namespace in files:
    try:
        os.remove(file)
    except:
        print("{} not found".format(file))

    with open(file, 'w') as registry:
        registry.write(REGISTRY_HEADER.format(namespace))
        # Parse the Registry header info
        registry.write("const Header header = {")
        registry.write("\"{}\",".format(json["@Redfish.Copyright"]))
        registry.write("\"{}\",".format(json["@odata.type"]))
        registry.write("\"{}\",".format(json["Id"]))
        registry.write(".\"{}\",".format(json["Name"]))
        registry.write("\"{}\",".format(json["Language"]))
        registry.write("\"{}\",".format(json["Description"]))
        registry.write(".\"{}\",".format(json["RegistryPrefix"]))
        registry.write("\"{}\",".format(json["RegistryVersion"]))
        registry.write("\"{}\",".format(json["OwningEntity"]))
        registry.write("};")

        # Parse each Message entry
        registry.write("const std::array registry = {")
        for messageId, message in sorted(json["Messages"].items()):
            registry.write("MessageEntry{")
            registry.write("\"{}\",".format(messageId))
            registry.write("{")
            registry.write("\"{}\",".format(message["Description"]))
            registry.write("\"{}\",".format(message["Message"]))
            registry.write("\"{}\",".format(message["Severity"]))
            registry.write("{},".format(message["NumberOfArgs"]))
            registry.write("{")
            paramTypes = message.get("ParamTypes")
            if paramTypes:
                for paramType in paramTypes:
                    registry.write("\"{}\",".format(paramType))
            registry.write("},")
            registry.write("\"{}\",".format(message["Resolution"]))
            registry.write("}},")
        registry.write("};}\n")
    subprocess.check_call(["clang-format-6.0", "-i", file])
