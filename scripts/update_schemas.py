#!/usr/bin/env python3
import json
import os
import shutil
import xml.etree.ElementTree as ET
import zipfile
from collections import OrderedDict, defaultdict
from io import BytesIO

import generate_schema_enums
import requests
from generate_schema_collections import generate_registries

VERSION = "DSP8010_2022.2"

WARNING = """/****************************************************************
 *                 READ THIS WARNING FIRST
 * This is an auto-generated header which contains definitions
 * for Redfish DMTF defined schemas.
 * DO NOT modify this registry outside of running the
 * update_schemas.py script.  The definitions contained within
 * this file are owned by DMTF.  Any modifications to these files
 * should be first pushed to the relevant registry in the DMTF
 * github organization.
 ***************************************************************/"""

# To use a new schema, add to list and rerun tool
include_list = [
    "AccountService",
    "ActionInfo",
    "Assembly",
    "AttributeRegistry",
    "Bios",
    "Cable",
    "CableCollection",
    "Certificate",
    "CertificateCollection",
    "CertificateLocations",
    "CertificateService",
    "Chassis",
    "ChassisCollection",
    "ComputerSystem",
    "ComputerSystemCollection",
    "Drive",
    "DriveCollection",
    "EnvironmentMetrics",
    "EthernetInterface",
    "EthernetInterfaceCollection",
    "Event",
    "EventDestination",
    "EventDestinationCollection",
    "EventService",
    "Fan",
    "FanCollection",
    "IPAddresses",
    "JsonSchemaFile",
    "JsonSchemaFileCollection",  # redfish/v1/JsonSchemas
    "LogEntry",
    "LogEntryCollection",
    "LogService",
    "LogServiceCollection",
    "Manager",
    "ManagerAccount",
    "ManagerAccountCollection",
    "ManagerCollection",
    "ManagerDiagnosticData",
    "ManagerNetworkProtocol",
    "Memory",
    "MemoryCollection",
    "Message",
    "MessageRegistry",
    "MessageRegistryCollection",
    "MessageRegistryFile",
    "MessageRegistryFileCollection",
    "MetricDefinition",
    "MetricDefinitionCollection",
    "MetricReport",
    "MetricReportCollection",
    "MetricReportDefinition",
    "MetricReportDefinitionCollection",
    "OperatingConfig",
    "OperatingConfigCollection",
    "PCIeDevice",
    "PCIeDeviceCollection",
    "PCIeFunction",
    "PCIeFunctionCollection",
    "PhysicalContext",
    "PCIeSlots",
    "Power",
    "PowerSubsystem",
    "PowerSupply",
    "PowerSupplyCollection",
    "Privileges",  # Used in Role
    "Processor",
    "ProcessorCollection",
    "RedfishError",
    "RedfishExtensions",
    "Redundancy",
    "Resource",
    "Role",
    "RoleCollection",
    "Sensor",
    "SensorCollection",
    "ServiceRoot",
    "Session",
    "SessionCollection",
    "SessionService",
    "Settings",
    "SoftwareInventory",
    "SoftwareInventoryCollection",
    "Storage",
    "StorageCollection",
    "StorageController",
    "StorageControllerCollection",
    "Task",
    "TaskCollection",
    "TaskService",
    "TelemetryService",
    "Thermal",
    "ThermalMetrics",
    "ThermalSubsystem",
    "Triggers",
    "TriggersCollection",
    "UpdateService",
    "VLanNetworkInterfaceCollection",
    "VLanNetworkInterface",
    "VirtualMedia",
    "VirtualMediaCollection",
    "odata",
    "odata-v4",
    "redfish-error",
    "redfish-payload-annotations",
    "redfish-schema",
    "redfish-schema-v1",
]

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

proxies = {"https": os.environ.get("https_proxy", None)}

r = requests.get(
    "https://www.dmtf.org/sites/default/files/standards/documents/"
    + VERSION
    + ".zip",
    proxies=proxies,
)

r.raise_for_status()


static_path = os.path.realpath(
    os.path.join(SCRIPT_DIR, "..", "static", "redfish", "v1")
)


cpp_path = os.path.realpath(
    os.path.join(SCRIPT_DIR, "..", "redfish-core", "include")
)


schema_path = os.path.join(static_path, "schema")
privilege_map_registry_path = os.path.join(
    cpp_path, "registries", "privilege_map_registry.hpp"
)
json_schema_path = os.path.join(static_path, "JsonSchemas")
metadata_index_path = os.path.join(static_path, "$metadata", "index.xml")

zipBytesIO = BytesIO(r.content)
zip_ref = zipfile.ZipFile(zipBytesIO)


class SchemaVersion:
    """
    A Python class for sorting Redfish schema versions.  Allows sorting Redfish
    versions in the way humans expect, by comparing version strings as lists
    (ie 0_2_0 comes before 0_10_0) in the way humans expect.  It does case
    insensitive schema name comparisons
    """

    def __init__(self, key):
        key = str.casefold(key)

        split_tup = key.split(".")
        self.version_pieces = [split_tup[0]]
        if len(split_tup) < 2:
            return
        version = split_tup[1]

        if version.startswith("v"):
            version = version[1:]
        if any(char.isdigit() for char in version):
            self.version_pieces.extend([int(x) for x in version.split("_")])

    def __lt__(self, other):
        return self.version_pieces < other.version_pieces


# Remove the old files
skip_prefixes = "Oem"
if os.path.exists(schema_path):
    files = [
        os.path.join(schema_path, f)
        for f in os.listdir(schema_path)
        if not f.startswith(skip_prefixes)
    ]
    for f in files:
        os.remove(f)
if os.path.exists(json_schema_path):
    files = [
        os.path.join(json_schema_path, f)
        for f in os.listdir(json_schema_path)
        if not f.startswith(skip_prefixes)
    ]
    for f in files:
        if os.path.isfile(f):
            os.remove(f)
        else:
            shutil.rmtree(f)
try:
    os.remove(metadata_index_path)
except FileNotFoundError:
    pass

if not os.path.exists(schema_path):
    os.makedirs(schema_path)
if not os.path.exists(json_schema_path):
    os.makedirs(json_schema_path)

csdl_filenames = []
json_schema_files = defaultdict(list)
schema_versions = defaultdict(list)


for zip_file in zip_ref.infolist():
    if zip_file.is_dir():
        continue
    if zip_file.filename.startswith("csdl/"):
        csdl_filenames.append(os.path.basename(zip_file.filename))
    elif zip_file.filename.startswith("json-schema/"):
        filename = os.path.basename(zip_file.filename)
        filenamesplit = filename.split(".")
        # exclude schemas again to save flash space
        if filenamesplit[0] not in include_list:
            continue
        json_schema_files[filenamesplit[0]].append(filename)
    elif zip_file.filename.startswith("openapi/"):
        pass
    elif zip_file.filename.startswith("dictionaries/"):
        pass

# sort the json files by version
for key, value in json_schema_files.items():
    value.sort(key=SchemaVersion, reverse=True)

# Create a dictionary ordered by schema name
json_schema_files = OrderedDict(
    sorted(json_schema_files.items(), key=lambda x: SchemaVersion(x[0]))
)

csdl_filenames.sort(key=SchemaVersion)
for filename in csdl_filenames:
    # filename looks like Zone_v1.xml
    with open(os.path.join(schema_path, filename), "wb") as schema_out:
        content = zip_ref.read(os.path.join("csdl", filename))
        content = content.replace(b"\r\n", b"\n")

        schema_out.write(content)

        filenamesplit = filename.split("_")
        xml_root = ET.fromstring(content)
        edmx = "{http://docs.oasis-open.org/odata/ns/edmx}"
        edm = "{http://docs.oasis-open.org/odata/ns/edm}"
        for edmx_child in xml_root:
            if edmx_child.tag == edmx + "DataServices":
                for data_child in edmx_child:
                    if data_child.tag == edm + "Schema":
                        namespace = data_child.attrib["Namespace"]

                        subname = namespace.split(".", 1)
                        version = "".join(subname[1:])

                        schema_versions[subname[0]].append(version)


for schema, version in json_schema_files.items():
    zip_filepath = os.path.join("json-schema", version[0])
    schemadir = os.path.join(json_schema_path, schema)
    os.makedirs(schemadir)

    with open(os.path.join(schemadir, schema + ".json"), "wb") as schema_file:
        schema_file.write(zip_ref.read(zip_filepath).replace(b"\r\n", b"\n"))

with open(os.path.join(cpp_path, "schemas.hpp"), "w") as hpp_file:
    hpp_file.write(
        "#pragma once\n"
        f"{WARNING}\n"
        "// clang-format off\n"
        '#include "schema_common.hpp"\n'
        "\n"
        "#include <array>\n"
        "\n"
        "namespace redfish::schemas\n"
        "{\n"
    )

    for schema_namespace, versions in schema_versions.items():
        varname = schema_namespace[0].lower() + schema_namespace[1:]

        include_in_metadata = varname in include_list

        # switch is a keyword
        if varname == "switch":
            varname = "switchSchema"
        newest_version = versions[-1][1:].split("_")
        if newest_version[0] == "":
            newest_version = [0, 0, 0]

        hpp_file.write(
            f"    constexpr SchemaVersion {varname}{{\n"
            f'        "{schema_namespace}",\n'
            f"        {newest_version[0]},\n"
            f"        {newest_version[1]},\n"
            f"        {newest_version[2]},\n"
            f"        {str(include_in_metadata).lower()}\n"
            "    };\n\n"
        )

    hpp_file.write(
        "    constexpr const std::array<"
        f"const SchemaVersion, {len(schema_versions)}> schemas {{\n"
    )
    for schema_namespace, versions in schema_versions.items():
        varname = schema_namespace[0].lower() + schema_namespace[1:]
        # switch is a keyword
        if varname == "switch":
            varname = "switchSchema"
        hpp_file.write(f"        {varname},\n")

    hpp_file.write("    };\n}\n")


# Script to add Privilege Registry Map

operation_list = ["GET", "HEAD", "PATCH", "POST", "PUT", "DELETE"]

url = (
    "https://redfish.dmtf.org/registries/Redfish_1.3.0_PrivilegeRegistry.json"
)
dmtf = requests.get(url, proxies=proxies)
dmtf.raise_for_status()
base_privilege_registry = json.loads(dmtf.text)

base_privilege_registry_entities = set()

for mapping in base_privilege_registry["Mappings"]:
    base_privilege_registry_entities.add(mapping["Entity"])

with open(privilege_map_registry_path, "w") as registry:
    registry.write(
        "#pragma once\n"
        f"{WARNING}\n"
        "// clang-format off\n"
        '#include "privilege_registry.hpp"\n'
        '#include "privileges.hpp"\n'
        '#include "verb.hpp"\n'
        "\n"
        "#include <array>\n"
        "#include <span>\n"
        "\n"
        "namespace redfish::privileges\n"
        "{\n"
    )
    registry.write("using OperationMap = ")
    registry.write("std::array<const std::span<const Privileges>, 6>;\n")
    registry.write(
        "constexpr const static std::array<OperationMap, {}>"
        " privilegeSetMap".format(len(schema_versions.items()))
    )
    registry.write("{{\n")
    for schema_namespace, versions in schema_versions.items():
        if schema_namespace not in base_privilege_registry_entities:
            registry.write("    NULL,\n")
            continue
        registry.write("    {{\n")
        for operation in operation_list:
            # TODO ManagerDiagnostic has no DELETE privilege in registry
            if operation not in mapping["OperationMap"]:
                continue
            operation = operation.lower()
            registry.write("      {}{}".format(operation, schema_namespace))
            registry.write(",\n")
        registry.write("    }},\n")
    registry.write("}};\n")
    registry.write("} // namespace redfish::privileges\n// clang-format on\n")

zip_ref.close()

generate_schema_enums.main()
generate_registries(include_list)

# Now delete the xml schema files we aren't supporting
if os.path.exists(schema_path):
    files = [
        os.path.join(schema_path, f)
        for f in os.listdir(schema_path)
        if not f.startswith(skip_prefixes)
    ]
    for filename in files:
        # filename will include the absolute path
        filenamesplit = filename.split("/")
        name = filenamesplit.pop()
        namesplit = name.split("_")
        if namesplit[0] not in include_list:
            print("excluding schema: " + filename)
            os.remove(filename)
