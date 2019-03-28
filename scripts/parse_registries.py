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

include_path = os.path.realpath(os.path.join(SCRIPT_DIR, "..", "redfish-core", "include"))

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
        registry.write(".copyright = \"{}\",".format(json["@Redfish.Copyright"]))
        registry.write(".type = \"{}\",".format(json["@odata.type"]))
        registry.write(".id = \"{}\",".format(json["Id"]))
        registry.write(".name = \"{}\",".format(json["Name"]))
        registry.write(".language = \"{}\",".format(json["Language"]))
        registry.write(".description = \"{}\",".format(json["Description"]))
        registry.write(".registryPrefix = \"{}\",".format(json["RegistryPrefix"]))
        registry.write(".registryVersion = \"{}\",".format(json["RegistryVersion"]))
        registry.write(".owningEntity = \"{}\",".format(json["OwningEntity"]))
        registry.write("};")

        # Parse each Message entry
        registry.write("const std::array registry = {")
        for messageId, message in sorted(json["Messages"].items()):
            registry.write("MessageEntry{")
            registry.write("\"{}\",".format(messageId))
            registry.write("{")
            registry.write(".description = \"{}\",".format(message["Description"]))
            registry.write(".message = \"{}\",".format(message["Message"]))
            registry.write(".severity = \"{}\",".format(message["Severity"]))
            registry.write(".numberOfArgs = {},".format(message["NumberOfArgs"]))
            registry.write(".paramTypes = {")
            paramTypes = message.get("ParamTypes")
            if paramTypes:
                for paramType in paramTypes:
                    registry.write("\"{}\",".format(paramType))
            registry.write("},")
            registry.write(".resolution = \"{}\",".format(message["Resolution"]))
            registry.write("}},")
        registry.write("};}\n")
    subprocess.check_call(["clang-format", "-i", file])
