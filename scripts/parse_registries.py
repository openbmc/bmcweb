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

REGISTRY_HEADER = '''
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

include_path = os.path.realpath(
    os.path.join(
        SCRIPT_DIR,
        "..",
        "redfish-core",
        "include",
        "registries"))

proxies = {
    'https': os.environ.get("https_proxy", None)
}


def make_getter(dmtf_name, header_name, type_name):
    url = 'https://redfish.dmtf.org/registries/{}'.format(dmtf_name)
    dmtf = requests.get(url, proxies=proxies)
    dmtf.raise_for_status()
    json_file = json.loads(dmtf.text)
    path = os.path.join(include_path, header_name)
    return (path, json_file, type_name, url)


def clang_format(filename):
    subprocess.check_call(["clang-format-12", "-i", filename])


files = []
files.append(make_getter('Base.1.11.0.json',
                         'base_message_registry.hpp',
                         'base'))
files.append(make_getter('TaskEvent.1.0.3.json',
                         'task_event_message_registry.hpp',
                         'task_event'))
files.append(make_getter('ResourceEvent.1.0.3.json',
                         'resource_event_message_registry.hpp',
                         'resource_event'))

# Remove the old files
for file, json_dict, namespace, url in files:
    try:
        os.remove(file)
    except BaseException:
        print("{} not found".format(file))

    with open(file, 'w') as registry:
        registry.write(REGISTRY_HEADER.format(namespace))
        # Parse the Registry header info
        registry.write("const Header header = {")
        registry.write("\"{}\",".format(json_dict["@Redfish.Copyright"]))
        registry.write("\"{}\",".format(json_dict["@odata.type"]))
        registry.write("\"{}\",".format(json_dict["Id"]))
        registry.write("\"{}\",".format(json_dict["Name"]))
        registry.write("\"{}\",".format(json_dict["Language"]))
        registry.write("\"{}\",".format(json_dict["Description"]))
        registry.write("\"{}\",".format(json_dict["RegistryPrefix"]))
        registry.write("\"{}\",".format(json_dict["RegistryVersion"]))
        registry.write("\"{}\",".format(json_dict["OwningEntity"]))
        registry.write("};")

        registry.write('constexpr const char * url = "{}";\n\n'.format(url))
        # Parse each Message entry
        registry.write(
            "constexpr std::array<MessageEntry, {}> registry = {{".format(
                len(json_dict["Messages"])))
        for messageId, message in sorted(json_dict["Messages"].items()):
            registry.write("MessageEntry{")
            registry.write("\"{}\",".format(messageId))
            registry.write("{")
            registry.write("\"{}\",".format(message["Description"]))
            registry.write("\"{}\",".format(message["Message"]))
            registry.write("\"{}\",".format(message["Severity"]))
            registry.write("\"{}\",".format(message["MessageSeverity"]))
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
    clang_format(file)


def get_privilege_string_from_list(privilege_list):
    privilege_string = "{{\n"
    for privilege_json in privilege_list:
        privileges = privilege_json["Privilege"]
        privilege_string += "    {"
        for privilege in privileges:
            if privilege == "NoAuth":
                continue
            privilege_string += "\""
            privilege_string += privilege
            privilege_string += "\",\n"
        if privilege != "NoAuth":
            privilege_string = privilege_string[:-2]
        privilege_string += "}"
        privilege_string += ",\n"
    privilege_string = privilege_string[:-2]
    privilege_string += "\n}}"
    return privilege_string


def get_variable_name_for_privilege_set(privilege_list):
    names = []
    for privilege_json in privilege_list:
        privileges = privilege_json["Privilege"]
        names.append("And".join(privileges))
    return "Or".join(names)


def make_privilege_registry():
    path, json_file, type_name, url = \
        make_getter('Redfish_1.2.0_PrivilegeRegistry.json',
                    'privilege_registry.hpp', 'privilege')
    with open(path, 'w') as registry:
        registry.write("#pragma once\n")
        registry.write(
            "//{} is generated.  Do not edit directly\n".format(
                os.path.basename(path)))

        registry.write("#include <privileges.hpp>\n\n")
        registry.write("namespace redfish::privileges{\n")

        privilege_dict = {}
        for mapping in json_file["Mappings"]:
            # first pass, identify all the unique privilege sets
            for operation, privilege_list in mapping["OperationMap"].items():
                privilege_dict[get_privilege_string_from_list(
                    privilege_list)] = (privilege_list, )
        registry.write("// clang-format off\n")
        for index, key in enumerate(privilege_dict):
            (privilege_list, ) = privilege_dict[key]
            name = get_variable_name_for_privilege_set(privilege_list)
            registry.write(
                "const std::array<Privileges, {}> ".format(
                    len(privilege_list)))
            registry.write(
                "privilegeSet{} = {};\n".format(name, key))
            privilege_dict[key] = (privilege_list, name)
        registry.write("// clang-format on\n\n")

        for mapping in json_file["Mappings"]:
            entity = mapping["Entity"]
            registry.write("//{}\n".format(entity))
            for operation, privilege_list in mapping["OperationMap"].items():
                privilege_string = get_privilege_string_from_list(
                    privilege_list)
                operation = operation.lower()

                registry.write(
                    "const static auto& {}{} = privilegeSet{};\n".format(
                        operation,
                        entity,
                        privilege_dict[privilege_string][1]))
            registry.write("\n")
        registry.write("} // namespace redfish::privileges\n")
    clang_format(path)


make_privilege_registry()
