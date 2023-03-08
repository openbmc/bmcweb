#!/usr/bin/env python3
import requests
import zipfile
from io import BytesIO
from packaging.version import Version, parse

import os
from collections import OrderedDict, defaultdict
import shutil
import json

import xml.etree.ElementTree as ET

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
    'EnvironmentMetrics',
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
json_schema_path = os.path.join(static_path, "JsonSchemas")
metadata_index_path = os.path.join(static_path, "$metadata", "index.xml")

zipBytesIO = BytesIO(r.content)
zip_ref = zipfile.ZipFile(zipBytesIO)


def version_sort_key(key):
    """
    Method that computes a sort key that zero pads all numbers, such that
    version sorting like
    0_2_0
    0_10_0
    sorts in the way humans expect.
    it also does case insensitive comparisons.
    """
    key = str.casefold(key)

    # Decription of this class calls it "Version numbering for anarchists and
    # software realists.".  That seems like exactly what we need here.

    if not any(char.isdigit() for char in key):
        split_tup = os.path.splitext(key)
        key = split_tup[0] + ".v0_0_0" + split_tup[1]

    # special case some files that don't seem to follow the naming convention,
    # and cause sort problems.  These need brought up with DMTF TODO(Ed)
    if key == "odata.4.0.0.json":
        key = "odata.v4_0_0.json"
    if key == "redfish-schema.1.0.0.json":
        key = "redfish-schema.v1_0_0.json"

    return parse(key)


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

for zip_filepath in zip_ref.namelist():
    if zip_filepath.startswith("csdl/") and (zip_filepath != "csdl/"):
        csdl_filenames.append(os.path.basename(zip_filepath))
    elif zip_filepath.startswith("json-schema/"):
        filename = os.path.basename(zip_filepath)
        filenamesplit = filename.split(".")
        # exclude schemas again to save flash space
        if filenamesplit[0] not in include_list:
            continue
        json_schema_files[filenamesplit[0]].append(filename)
    elif zip_filepath.startswith("openapi/"):
        pass
    elif zip_filepath.startswith("dictionaries/"):
        pass

# sort the json files by version
for key, value in json_schema_files.items():
    value.sort(key=version_sort_key, reverse=True)

# Create a dictionary ordered by schema name
json_schema_files = OrderedDict(
    sorted(json_schema_files.items(), key=lambda x: version_sort_key(x[0]))
)

csdl_filenames.sort(key=version_sort_key)
with open(metadata_index_path, "w") as metadata_index:

    metadata_index.write('<?xml version="1.0" encoding="UTF-8"?>\n')
    metadata_index.write(
        "<edmx:Edmx xmlns:edmx="
        '"http://docs.oasis-open.org/odata/ns/edmx"'
        ' Version="4.0">\n'
    )

    for filename in csdl_filenames:
        # filename looks like Zone_v1.xml
        filenamesplit = filename.split("_")
        if filenamesplit[0] not in include_list:
            print("excluding schema: " + filename)
            continue

        with open(os.path.join(schema_path, filename), "wb") as schema_out:

            metadata_index.write(
                '    <edmx:Reference Uri="/redfish/v1/schema/'
                + filename
                + '">\n'
            )

            content = zip_ref.read(os.path.join("csdl", filename))
            content = content.replace(b"\r\n", b"\n")
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
                                    "        "
                                    '<edmx:Include Namespace="'
                                    + namespace
                                    + '"  Alias="Redfish"/>\n'
                                )

                            else:
                                metadata_index.write(
                                    "        "
                                    '<edmx:Include Namespace="'
                                    + namespace
                                    + '"/>\n'
                                )
            schema_out.write(content)
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
    # TODO:Issue#32 There's a bug in the script that currently deletes this
    # schema (because it's an OEM schema). Because it's the only six, and we
    # don't update schemas very often, we just manually fix it. Need a
    # permanent fix to the script.
    metadata_index.write(
        '    <edmx:Reference Uri="/redfish/v1/schema/OemManager_v1.xml">\n'
    )
    metadata_index.write('        <edmx:Include Namespace="OemManager"/>\n')
    metadata_index.write("    </edmx:Reference>\n")

    metadata_index.write(
        '    <edmx:Reference Uri="'
        '/redfish/v1/schema/OemComputerSystem_v1.xml">\n'
    )
    metadata_index.write(
        '        <edmx:Include Namespace="OemComputerSystem"/>\n'
    )
    metadata_index.write("    </edmx:Reference>\n")

    metadata_index.write(
        '    <edmx:Reference Uri="'
        '/redfish/v1/schema/OemVirtualMedia_v1.xml">\n'
    )
    metadata_index.write(
        '        <edmx:Include Namespace="OemVirtualMedia"/>\n'
    )
    metadata_index.write(
        '        <edmx:Include Namespace="OemVirtualMedia.v1_0_0"/>\n'
    )
    metadata_index.write("    </edmx:Reference>\n")

    metadata_index.write(
        '    <edmx:Reference Uri="'
        '/redfish/v1/schema/OemAccountService_v1.xml">\n'
    )
    metadata_index.write(
        '        <edmx:Include Namespace="OemAccountService"/>\n'
    )
    metadata_index.write(
        '        <edmx:Include Namespace="OemAccountService.v1_0_0"/>\n'
    )
    metadata_index.write("    </edmx:Reference>\n")

    metadata_index.write(
        '    <edmx:Reference Uri="/redfish/v1/schema/OemSession_v1.xml">\n'
    )
    metadata_index.write('        <edmx:Include Namespace="OemSession"/>\n')
    metadata_index.write(
        '        <edmx:Include Namespace="OemSession.v1_0_0"/>\n'
    )
    metadata_index.write("    </edmx:Reference>\n")

    metadata_index.write(
        '    <edmx:Reference Uri="/redfish/v1/schema/OemRouteMetric_v1.xml">\n'
    )
    metadata_index.write('        <edmx:Include Namespace="OemRouteMetric"/>\n')
    metadata_index.write(
        '        <edmx:Include Namespace="OemRouteMetric.v1_0_0"/>\n'
    )
    metadata_index.write("    </edmx:Reference>\n")

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

    hpp_file.write("    };\n" "}\n")

zip_ref.close()
