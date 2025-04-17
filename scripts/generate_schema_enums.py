#!/usr/bin/python3
import os
import re
import shutil
import xml.etree.ElementTree as ET
from collections import defaultdict

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
REDFISH_SCHEMA_DIR = os.path.realpath(
    os.path.join(SCRIPT_DIR, "..", "redfish-core", "schema")
)

OUTFOLDER = os.path.realpath(
    os.path.join(
        SCRIPT_DIR, "..", "redfish-core", "include", "generated", "enums"
    )
)

# Odata string types
EDMX = "{http://docs.oasis-open.org/odata/ns/edmx}"
EDM = "{http://docs.oasis-open.org/odata/ns/edm}"


class Enum:
    def __init__(self, name, values, namespace, from_file):
        self.name = name
        self.values = values
        self.namespace = namespace
        self.from_file = from_file


def parse_schema(element, filename):
    EntityTypes = []
    namespace = element.attrib["Namespace"]
    for schema_element in element:
        name = schema_element.attrib.get("Name", None)
        if name is None:
            continue
        if schema_element.tag == EDM + "EnumType":
            enums = []
            for member in schema_element.findall(EDM + "Member"):
                enums.append(member.attrib["Name"])
            EntityTypes.append(Enum(name, enums, namespace, filename))
        if schema_element.tag == EDM + "TypeDefinition":
            enums = []
            for annotation in schema_element:
                for collection in annotation:
                    for record in collection.findall(EDM + "Record"):
                        for member in record.findall(EDM + "PropertyValue"):
                            enums.append(member.attrib["String"])
            EntityTypes.append(Enum(name, enums, namespace, filename))
    return EntityTypes


def parse_file(filename):
    print(f"Parsing {filename}")
    tree = ET.parse(filename)
    root = tree.getroot()
    results = []
    data_services = root.findall(EDMX + "DataServices")
    for ds in data_services:
        for element in ds:
            if element.tag == EDM + "Schema":
                results.extend(parse_schema(element, filename))

    return results


def camel_to_snake(name):
    # snake casing PCIeDevice and PCIeFunction results in mediocre results
    # given that the standard didn't camel case those in a way that the
    # algorithm expects, so change the casing explicitly to generate sane
    # snake case results.
    name = name.replace("PCIe", "Pcie")
    name = re.sub("(.)([A-Z][a-z]+)", r"\1_\2", name)
    return re.sub("([a-z0-9])([A-Z])", r"\1_\2", name).lower()


def write_enum_list(redfish_defs_file, enum_list, snake_case_namespace):
    redfish_defs_file.write(
        "// SPDX-License-Identifier: Apache-2.0\n"
        "// SPDX-FileCopyrightText: Copyright OpenBMC Authors\n"
        "#pragma once\n"
        "#include <nlohmann/json.hpp>\n\n"
        "namespace {}\n"
        "{{\n"
        "// clang-format off\n\n".format(snake_case_namespace)
    )

    for element in enum_list:
        redfish_defs_file.write("enum class {}{{\n".format(element.name))
        values = element.values
        if "Invalid" not in values:
            values.insert(0, "Invalid")

        for value in values:
            # If the value is numeric, prefix it with the enum name
            if value.isdigit():
                enum_value = f"{element.name}{value}"
            else:
                enum_value = re.sub(r"[^0-9_a-zA-Z]", "", value)

            redfish_defs_file.write("    {},\n".format(enum_value))
        redfish_defs_file.write("};\n\n")

    for element in enum_list:
        values = element.values
        if "Invalid" not in values:
            values.insert(0, "Invalid")
        # nlohmann::json apparently uses c style arrays in their enum
        # implementation, and clang-tidy isn't smart enough to figure out that
        # the C arrays are in their code not bmcwebs, so we have to explicitly
        # ignore the error.
        redfish_defs_file.write(
            "NLOHMANN_JSON_SERIALIZE_ENUM({}, {{\n".format(element.name)
        )
        for value in values:
            # If the value is numeric, prefix it with the enum name
            if value.isdigit():
                enum_value = f"{element.name}{value}"
            else:
                enum_value = re.sub(r"[^0-9_a-zA-Z]", "", value)
            redfish_defs_file.write(
                '    {{{}::{}, "{}"}},\n'.format(
                    element.name, enum_value, value
                )
            )

        redfish_defs_file.write("});\n\n")

        print(element.name)

    redfish_defs_file.write("}\n// clang-format on\n")


def generate_enums(flat_list):
    # clear out the old results if they exist
    if os.path.exists(OUTFOLDER):
        shutil.rmtree(OUTFOLDER)
    os.makedirs(OUTFOLDER)

    enum_by_namespace = defaultdict(list)

    for element in flat_list:
        if isinstance(element, Enum):
            namespace_split = element.namespace.split(".")[0]
            enum_by_namespace[namespace_split].append(element)

    for namespace, enum_list in enum_by_namespace.items():
        snake_case_namespace = camel_to_snake(namespace)
        outfile = os.path.join(
            OUTFOLDER, "{}.hpp".format(snake_case_namespace)
        )

        with open(outfile, "w") as redfish_defs:
            write_enum_list(redfish_defs, enum_list, snake_case_namespace)


def main():
    print("Reading from {}".format(REDFISH_SCHEMA_DIR))

    filepaths = []
    for root, dirs, files in os.walk(REDFISH_SCHEMA_DIR):
        for csdl_file in files:
            filepath = os.path.join(root, csdl_file)
            if os.path.islink(filepath):
                continue
            if csdl_file.endswith(".xml"):
                filepaths.append(filepath)
    print(filepaths)
    enum_list = []
    filepaths.sort()
    for filepath in filepaths:
        out = parse_file(filepath)
        enum_list.extend(out)

    print("Parsing done")

    generate_enums(enum_list)


if __name__ == "__main__":
    main()
