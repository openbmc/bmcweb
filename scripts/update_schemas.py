#!/usr/bin/env python3
import os
import shutil
import xml.etree.ElementTree as ET
import zipfile
from collections import OrderedDict, defaultdict
from io import BytesIO

import generate_schema_enums
import generate_meson
import requests

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
oem_schemas = [
    "OemAccountService",
    "OemComputerSystem",
    "OemManager",
    "OemSession",
    "OemVirtualMedia",
]

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
]
include_list.extend(oem_schemas)

json_only_schemas = [
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
        content = zip_ref.read(zip_file.filename)
        content = content.replace(b"\r\n", b"\n")
        with open(os.path.join(schema_path, os.path.basename(zip_file.filename)), "wb") as schema_out:
            schema_out.write(content)
        csdl_filenames.append(os.path.basename(zip_file.filename))

    elif zip_file.filename.startswith("json-schema/"):
        with open(os.path.join(json_schema_path, os.path.basename(zip_file.filename)), "wb") as schema_file:
            schema_file.write(zip_ref.read(zip_file.filename).replace(b"\r\n", b"\n"))
    elif zip_file.filename.startswith("openapi/"):
        pass
    elif zip_file.filename.startswith("dictionaries/"):
        pass

csdl_filenames.extend(
  [schema + "_v1.xml" for schema in oem_schemas]
)

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
    filepath = os.path.join(schema_path, filename)
    with open(filepath, "r") as schema_out:
        print(f"Reading {filepath}\n")
        content = schema_out.read()

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

zip_ref.close()

generate_schema_enums.main()
generate_meson.main(csdl_filenames, include_list, schema_versions)
