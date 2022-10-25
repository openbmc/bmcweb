#!/usr/bin/env python3
from atexit import register
from email.mime import base
import json
import os

import requests
import argparse

PRAGMA_ONCE = """#pragma once
"""

WARNING = """/****************************************************************
 *                 READ THIS WARNING FIRST
 * This is an auto-generated header which contains definitions
 * for Redfish DMTF defined messages.
 * DO NOT modify this registry outside of running the
 * parse_registries.py script.  The definitions contained within
 * this file are owned by DMTF.  Any modifications to these files
 * should be first pushed to the relevant registry in the DMTF
 * github organization.
 ***************************************************************/"""

REGISTRY_HEADER = (
    PRAGMA_ONCE
    + WARNING
    + """
#include "registries.hpp"

#include <array>

// clang-format off

namespace redfish::registries::{}
{{
"""
)

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

include_path = os.path.realpath(
    os.path.join(SCRIPT_DIR, "..", "redfish-core", "include", "registries")
)

privilege_test_file = os.path.join(
    os.path.realpath(os.path.join(SCRIPT_DIR, "..",
                     "test", "redfish-core", "include")),
    "base_privilege.hpp",
)

proxies = {"https": os.environ.get("https_proxy", None)}


def make_getter(dmtf_name, header_name, type_name):
    url = "https://redfish.dmtf.org/registries/{}".format(dmtf_name)
    dmtf = requests.get(url, proxies=proxies)
    dmtf.raise_for_status()
    json_file = json.loads(dmtf.text)
    path = os.path.join(include_path, header_name)
    return (path, json_file, type_name, url)


def update_registries(files):
    # Remove the old files
    for file, json_dict, namespace, url in files:
        try:
            os.remove(file)
        except BaseException:
            print("{} not found".format(file))

        with open(file, "w") as registry:
            registry.write(REGISTRY_HEADER.format(namespace))
            # Parse the Registry header info
            registry.write(
                "const Header header = {{\n"
                '    "{json_dict[@Redfish.Copyright]}",\n'
                '    "{json_dict[@odata.type]}",\n'
                '    "{json_dict[Id]}",\n'
                '    "{json_dict[Name]}",\n'
                '    "{json_dict[Language]}",\n'
                '    "{json_dict[Description]}",\n'
                '    "{json_dict[RegistryPrefix]}",\n'
                '    "{json_dict[RegistryVersion]}",\n'
                '    "{json_dict[OwningEntity]}",\n'
                "}};\n"
                "constexpr const char* url =\n"
                '    "{url}";\n'
                "\n"
                "constexpr std::array registry =\n"
                "{{\n".format(
                    json_dict=json_dict,
                    url=url,
                )
            )

            messages_sorted = sorted(json_dict["Messages"].items())
            for messageId, message in messages_sorted:
                registry.write(
                    "    MessageEntry{{\n"
                    '        "{messageId}",\n'
                    "        {{\n"
                    '            "{message[Description]}",\n'
                    '            "{message[Message]}",\n'
                    '            "{message[MessageSeverity]}",\n'
                    "            {message[NumberOfArgs]},\n"
                    "            {{".format(
                        messageId=messageId, message=message)
                )
                paramTypes = message.get("ParamTypes")
                if paramTypes:
                    for paramType in paramTypes:
                        registry.write(
                            "\n" '                "{}",'.format(paramType))
                    registry.write("\n            },\n")
                else:
                    registry.write("},\n")
                registry.write(
                    '            "{message[Resolution]}",\n'
                    "        }}}},\n".format(message=message)
                )

            registry.write("\n};\n" "\n" "enum class Index\n" "{\n")
            for index, (messageId, message) in enumerate(messages_sorted):
                messageId = messageId[0].lower() + messageId[1:]
                registry.write("    {} = {},\n".format(messageId, index))
            registry.write(
                "}};\n" "}} // namespace redfish::registries::{}\n".format(
                    namespace)
            )


PRIVILEGE_HEADER = (
    PRAGMA_ONCE
    + WARNING
    + """
#include <array>
#include <cstdint>
#include <string_view>

namespace redfish::privileges
{
"""
)

PRIVILEGE_TAILER = "} // namespace redfish::privileges"

PRIVILEGE_TEST_HEADER = (
    PRAGMA_ONCE
    + WARNING
    + """
#include <string_view>

namespace redfish::privileges
{
"""
)

PRIVILEGE_TEST_TAILER = "} // namespace redfish::privileges"

# Convert a Python array to C++ initialization list


def array_to_cpp_init_list_str(array):
    res = '{"'
    res += '", "'.join(array)
    res += '"}'
    return res


def array_to_cpp_init_list_int(array):
    res = "{"
    res += ", ".join(array)
    res += "}"
    return res


# Returns a bitmap or privileges AND
# e.g., assume basePrivileges = [
# "Login", "ConfigureManager", "ConfigureUsers", "ConfigureComponents",
# "ConfigureSelf"];
# If a client needs both Login and ConfigureManager privilege to access a
# resource, then the encoding is 0b00011
def encode(base_privileges, privileges):
    privilege_set = set(privileges)
    encoding = 0b0
    for i, privilege in enumerate(base_privileges):
        if privilege in privilege_set:
            encoding |= 1 << i
    return "0b" + bin(encoding)[2:].zfill(len(base_privileges))


def make_privilege_registry():
    path, json_file, type_name, url = make_getter(
        "Redfish_1.3.0_PrivilegeRegistry.json",
        "privilege_registry.hpp", "privilege"
    )

    with open(privilege_test_file, "w") as registry:
        registry.write(PRIVILEGE_TEST_HEADER)
        registry.write(
            'constexpr std::string_view basePrivilegeStr = R"({})";\n'.format(
                json.dumps(json_file, indent=2)
            )
        )
        registry.write(PRIVILEGE_TEST_TAILER)
        os.system(f"clang-format -i --style=file {privilege_test_file}")

    max_privileges_cnt = 64
    with open(path, "w") as registry:
        registry.write(PRIVILEGE_HEADER)

        registry.write(
            f"constexpr const size_t maxPrivilegeCount={max_privileges_cnt};\n"
        )

        base_privileges = json_file["PrivilegesUsed"]

        if len(base_privileges) > max_privileges_cnt:
            raise Exception("Too many base privileges")

        base_privileges_str = array_to_cpp_init_list_str(base_privileges)

        registry.write(
            "constexpr std::array<std::string_view, {length}> basePrivileges ="
            "\n"
            "{array};\n".format(length=len(base_privileges),
                                array=base_privileges_str)
        )

        # Generate all entities
        entities = []
        for mapping in json_file["Mappings"]:
            entities.append(mapping["Entity"])

        entities_str = array_to_cpp_init_list_str(entities)

        # Add a Tag at the begin to prevent from conflicting with reversed
        # keywords, e.g., switch
        entity_tags = ["tag" + entity for entity in entities]

        registry.write("enum class EntityTag : int {\n")

        for i in range(len(entity_tags)):
            registry.write(entity_tags[i] + f" = {i},\n")
        registry.write("none = {},\n".format(len(entity_tags)))
        registry.write("};\n")

        registry.write(
            "constexpr std::array<std::string_view, {length}> entities ="
            "{array};\n".format(length=len(entities), array=entities_str)
        )

        # key: operation, GET, POST, etc
        # value: an 1d array representing an array of bitmaps for all the
        # entities in sequence;
        # The 1d array is actually a sequential version of a 2d array. The
        # length of each sub-array is stored in operation_entity_bitmap_length
        operation_entity_bitmaps = {
            "GET": [],
            "HEAD": [],
            "PATCH": [],
            "PUT": [],
            "DELETE": [],
            "POST": [],
        }

        # key: operation, GET, POST, etc
        # value: an 1d array, value[i] represents the length of the bitmaps of
        # an entity
        operation_entity_bitmap_length = {
            "GET": [],
            "HEAD": [],
            "PATCH": [],
            "PUT": [],
            "DELETE": [],
            "POST": [],
        }

        for index, _ in enumerate(entities):
            for method, bitmaps in operation_entity_bitmaps.items():
                # TODO: ManagerDiagnosticData doesn't specify DELETE privileges
                # in Redfish_1.3.0_PrivilegeRegistry.json
                # Fixed in https://github.com/DMTF/Redfish/issues/5296
                # Remove this when 1.3.1 is available
                if method not in json_file["Mappings"][index]["OperationMap"]:
                    bitmaps.append(
                        encode(base_privileges, ["ConfigureManager"]))
                    operation_entity_bitmap_length[method].append("1")
                    continue

                privileges_arr = \
                    json_file["Mappings"][index]["OperationMap"][method]

                for privileges in privileges_arr:
                    bitmaps.append(
                        encode(base_privileges, privileges["Privilege"]))

                operation_entity_bitmap_length[method].append(
                    str(len(privileges_arr)))

        for method, bitmaps_array in operation_entity_bitmaps.items():

            registry.write(
                "constexpr std::array<uint64_t, {length}> "
                "{method}BasePrivilegesBitmaps = {array};".format(
                    length=len(bitmaps_array),
                    method=method.lower(),
                    array=array_to_cpp_init_list_int(bitmaps_array),
                )
            )

            registry.write(
                "constexpr std::array<uint64_t, {length}> "
                "{method}BasePrivilegesLength = {array};".format(
                    length=len(operation_entity_bitmap_length[method]),
                    method=method.lower(),
                    array=array_to_cpp_init_list_int(
                        operation_entity_bitmap_length[method]
                    ),
                )
            )

        registry.write(PRIVILEGE_TAILER)

    os.system("clang-format -i --style=file {}".format(path))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--registries",
        type=str,
        default="base,task_event,resource_event,privilege",
        help="Comma delimited list of registries to update",
    )

    args = parser.parse_args()

    registries = set(args.registries.split(","))
    files = []

    if "base" in registries:
        files.append(
            make_getter("Base.1.13.0.json",
                        "base_message_registry.hpp", "base")
        )
        registries.remove("base")

    if "task_event" in registries:
        files.append(
            make_getter(
                "TaskEvent.1.0.3.json",
                "task_event_message_registry.hpp", "task_event"
            )
        )
        registries.remove("task_event")

    if "resource_event" in registries:
        files.append(
            make_getter(
                "ResourceEvent.1.0.3.json",
                "resource_event_message_registry.hpp",
                "resource_event",
            )
        )
        registries.remove("resource_event")

    update_registries(files)

    if "privilege" in registries:
        make_privilege_registry()
        registries.remove("privilege")

    if len(registries) != 0:
        raise Exception(",".join(registries) + " is not support!")


if __name__ == "__main__":
    main()
