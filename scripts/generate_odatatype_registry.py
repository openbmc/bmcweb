#!/usr/bin/env python3

import json
import os
import re

import requests

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

include_path = os.path.realpath(
    os.path.join(SCRIPT_DIR, "..", "redfish-core", "include", "registries")
)

read_path = os.path.realpath(
    os.path.join(SCRIPT_DIR, "..", "redfish-core", "lib")
)

proxies = {"https": os.environ.get("https_proxy", None)}

REGEX_MATCH = r'BMCWEB_ROUTE\(app,("[^}.]*?")\).privileges\(redfish::privileges::(get|head|patch|post|put|delete)(.*?)\)'


PRAGMA_ONCE = """#pragma once
"""

WARNING = """/****************************************************************
 *                 READ THIS WARNING FIRST
 * This is an auto-generated header which contains definitions
 * for Redfish DMTF defined messages.
 * DO NOT modify this registry outside of running the
 * generate_odatatype_registry script.  The definitions contained within
 * this file are owned by DMTF.  Any modifications to these files
 * should be first pushed to the relevant registry in the DMTF
 * github organization.
 ***************************************************************/"""

HEADER = (
    PRAGMA_ONCE
    + WARNING
    + """

#include <array>
#include <string_view>

// clang-format off

namespace redfish
{
"""
)

URI_GETTER = """
inline std::span<const std::string_view> getUriFromEntityTag(redfish::privileges::EntityTag entity)
{
    size_t entityIndex = static_cast<size_t>(entity);
    return entityTypeToUriMap[entityIndex];
}"""


def make_getter(dmtf_name, header_name, type_name):
    url = "https://redfish.dmtf.org/registries/{}".format(dmtf_name)
    dmtf = requests.get(url, proxies=proxies)
    dmtf.raise_for_status()
    json_file = json.loads(dmtf.text)
    path = os.path.join(include_path, header_name)
    return (path, json_file, type_name, url)


def main():
    path, json_file, type_name, url = make_getter(
        "Redfish_1.3.0_PrivilegeRegistry.json",
        "entity_type_registry.hpp",
        "registry",
    )

    entity_tag_list = []
    for i in range(len(json_file["Mappings"])):
        mapping = json_file["Mappings"][i]
        entity_tag_list.append(mapping["Entity"])

    entity_tag_to_uri = {}

    for entity in entity_tag_list:
        entity_tag_to_uri[entity] = set()

    for file in os.listdir(read_path):
        with open(read_path + "/" + file, "r") as f:
            matches = re.findall(
                REGEX_MATCH, f.read().replace(" ", "").replace("\n", "")
            )
            for match in matches:
                if match[0] in entity_tag_to_uri[match[2]]:
                    continue

                entity_tag_to_uri[match[2]].add(match[0])

    with open(path, "w") as registry:
        registry.write(HEADER)

        for entity in entity_tag_list:
            uri_set = entity_tag_to_uri[entity]
            registry.write(
                "const std::array<std::string_view, {}> {}Uris = ".format(
                    len(uri_set), entity
                )
            )
            registry.write("{{\n")

            for uri in uri_set:
                registry.write("    {},\n".format(uri))

            registry.write("}};\n\n")

        registry.write(
            "const static std::array<const std::span<const std::string_view>,"
            " {}> entityTypeToUriMap".format(len(entity_tag_list))
        )

        registry.write("{{\n")
        for entity in entity_tag_list:
            registry.write("    {}Uris,\n".format(entity))
        registry.write("}};\n")
        registry.write(URI_GETTER)
        registry.write(
            "} // namespace redfish::privileges\n// clang-format on\n"
        )


if __name__ == "__main__":
    main()
