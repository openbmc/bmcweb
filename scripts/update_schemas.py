#!/usr/bin/env python3
import os
import shutil
import xml.etree.ElementTree as ET
import zipfile
from collections import OrderedDict, defaultdict
from io import BytesIO

import generate_schema_enums
import requests
from generate_schema_collections import generate_top_collections

VERSION = "DSP8010_2023.3"

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
    "AggregationService",
    "AggregationSource",
    "AggregationSourceCollection",
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
    "ComponentIntegrity",
    "ComponentIntegrityCollection",
    "Drive",
    "DriveCollection",
    "EnvironmentMetrics",
    "EthernetInterface",
    "EthernetInterfaceCollection",
    "Event",
    "EventDestination",
    "EventDestinationCollection",
    "EventService",
    "FabricAdapter",
    "FabricAdapterCollection",
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
    "Port",
    "PortCollection",
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
    "VirtualMedia",
    "VirtualMediaCollection",
    "odata",
    "odata-v4",
    "redfish-error",
    "redfish-payload-annotations",
    "redfish-schema",
    "redfish-schema-v1",
]

# OEM schemas
oem_schema_names = [
    "OemManager",
    "OemManagerAccount",
    "OemComputerSystem",
    "OemVirtualMedia",
    "OpenBMCAccountService",
    "OemUpdateService",
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
skip_prefixes = ["Oem", "OpenBMC"]
if os.path.exists(schema_path):
    files = [
        os.path.join(schema_path, f)
        for f in os.listdir(schema_path)
        if not any([f.startswith(prefix) for prefix in skip_prefixes])
    ]
    for f in files:
        os.remove(f)
if os.path.exists(json_schema_path):
    files = [
        os.path.join(json_schema_path, f)
        for f in os.listdir(json_schema_path)
        if not any([f.startswith(prefix) for prefix in skip_prefixes])
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

# Create oem filenames - from oem json names
oem_csdl_filenames = []
for filename in oem_schema_names:
    oem_csdl_filenames.append(filename + "_v1.xml")

# Append Oem csdl files
csdl_filenames += oem_csdl_filenames

with open(metadata_index_path, "w") as metadata_index:
    metadata_index.write('<?xml version="1.0" encoding="UTF-8"?>\n')
    metadata_index.write(
        "<edmx:Edmx xmlns:edmx="
        '"http://docs.oasis-open.org/odata/ns/edmx"'
        ' Version="4.0">\n'
    )

    for filename in csdl_filenames:
        # filename looks like Zone_v1.xml
        if filename in oem_csdl_filenames:
            with open(
                os.path.join(schema_path, filename), "rb"
            ) as oem_csdl_in:
                content = oem_csdl_in.read()
                content = content.replace(b"\r\n", b"\n")
        else:
            with open(os.path.join(schema_path, filename), "wb") as schema_out:
                content = zip_ref.read(os.path.join("csdl", filename))
                content = content.replace(b"\r\n", b"\n")
                schema_out.write(content)
                filenamesplit = filename.split("_")
                if filenamesplit[0] not in include_list:
                    continue

        metadata_index.write(
            '    <edmx:Reference Uri="/redfish/v1/schema/' + filename + '">\n'
        )

        xml_root = ET.fromstring(content)
        edmx = "{http://docs.oasis-open.org/odata/ns/edmx}"
        edm = "{http://docs.oasis-open.org/odata/ns/edm}"
        for edmx_child in xml_root:
            if edmx_child.tag == edmx + "DataServices":
                for data_child in edmx_child:
                    if data_child.tag == edm + "Schema":
                        namespace = data_child.attrib["Namespace"]
                        if namespace.startswith("RedfishExtensions"):
                            metadata_index.write(
                                '        <edmx:Include Namespace="'
                                + namespace
                                + '"  Alias="Redfish"/>\n'
                            )

                        else:
                            metadata_index.write(
                                '        <edmx:Include Namespace="'
                                + namespace
                                + '"/>\n'
                            )
        metadata_index.write("    </edmx:Reference>\n")

    metadata_index.write(
        "    <edmx:DataServices>\n"
        "        <Schema "
        'xmlns="http://docs.oasis-open.org/odata/ns/edm" '
        'Namespace="Service">\n'
        '            <EntityContainer Name="Service" '
        'Extends="ServiceRoot.v1_0_0.ServiceContainer"/>\n'
        "        </Schema>\n"
        "    </edmx:DataServices>\n"
    )
    metadata_index.write("</edmx:Edmx>\n")


for schema, version in json_schema_files.items():
    zip_filepath = os.path.join("json-schema", version[0])
    schemadir = os.path.join(json_schema_path, schema)
    os.makedirs(schemadir)

    with open(os.path.join(schemadir, schema + ".json"), "wb") as schema_file:
        schema_file.write(zip_ref.read(zip_filepath).replace(b"\r\n", b"\n"))

with open(os.path.join(cpp_path, "schemas.hpp"), "w") as hpp_file:
    hpp_file.write(
        "#pragma once\n"
        "{WARNING}\n"
        "// clang-format off\n"
        "#include <array>\n"
        "\n"
        "namespace redfish\n"
        "{{\n"
        "    constexpr std::array schemas {{\n".format(WARNING=WARNING)
    )
    for schema_file in json_schema_files:
        hpp_file.write('        "{}",\n'.format(schema_file))

    for schema_file in oem_schema_names:
        hpp_file.write('        "{}",\n'.format(schema_file))

    hpp_file.write("    };\n}\n")

zip_ref.close()

generate_schema_enums.main()
generate_top_collections()

# Now delete the xml schema files we aren't supporting
if os.path.exists(schema_path):
    files = [
        os.path.join(schema_path, f)
        for f in os.listdir(schema_path)
        if not any([f.startswith(prefix) for prefix in skip_prefixes])
    ]
    for filename in files:
        # filename will include the absolute path
        filenamesplit = filename.split("/")
        name = filenamesplit.pop()
        namesplit = name.split("_")
        if namesplit[0] not in include_list:
            print("excluding schema: " + filename)
            os.remove(filename)
