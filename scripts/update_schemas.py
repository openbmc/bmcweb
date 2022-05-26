#!/usr/bin/env python3
import requests
import zipfile
from io import BytesIO
import os
from collections import OrderedDict, defaultdict
import shutil
import json

import xml.etree.ElementTree as ET

VERSION = "DSP8010_2021.4"

WARNING = '''/****************************************************************
 *                 READ THIS WARNING FIRST
 * This is an auto-generated header which contains definitions
 * for Redfish DMTF defined schemas.
 * DO NOT modify this registry outside of running the
 * update_schemas.py script.  The definitions contained within
 * this file are owned by DMTF.  Any modifications to these files
 * should be first pushed to the relevant registry in the DMTF
 * github organization.
 ***************************************************************/'''

# To use a new schema, add to list and rerun tool
include_list = [
    'AccountService',
    'ActionInfo',
    'Assembly',
    'AttributeRegistry',
    'Bios',
    'Cable',
    'CableCollection',
    'Certificate',
    'CertificateCollection',
    'CertificateLocations',
    'CertificateService',
    'Chassis',
    'ChassisCollection',
    'ComputerSystem',
    'ComputerSystemCollection',
    'Drive',
    'DriveCollection',
    'EthernetInterface',
    'EthernetInterfaceCollection',
    'Event',
    'EventDestination',
    'EventDestinationCollection',
    'EventService',
    'IPAddresses',
    'JsonSchemaFile',
    'JsonSchemaFileCollection',  # redfish/v1/JsonSchemas
    'LogEntry',
    'LogEntryCollection',
    'LogService',
    'LogServiceCollection',
    'Manager',
    'ManagerAccount',
    'ManagerAccountCollection',
    'ManagerCollection',
    'ManagerDiagnosticData',
    'ManagerNetworkProtocol',
    'Memory',
    'MemoryCollection',
    'Message',
    'MessageRegistry',
    'MessageRegistryCollection',
    'MessageRegistryFile',
    'MessageRegistryFileCollection',
    'MetricDefinition',
    'MetricDefinitionCollection',
    'MetricReport',
    'MetricReportCollection',
    'MetricReportDefinition',
    'MetricReportDefinitionCollection',
    'OperatingConfig',
    'OperatingConfigCollection',
    'PCIeDevice',
    'PCIeDeviceCollection',
    'PCIeFunction',
    'PCIeFunctionCollection',
    'PhysicalContext',
    'Power',
    'Privileges',  # Used in Role
    'Processor',
    'ProcessorCollection',
    'RedfishError',
    'RedfishExtensions',
    'Redundancy',
    'Resource',
    'Role',
    'RoleCollection',
    'Sensor',
    'SensorCollection',
    'ServiceRoot',
    'Session',
    'SessionCollection',
    'SessionService',
    'Settings',
    'SoftwareInventory',
    'SoftwareInventoryCollection',
    'Storage',
    'StorageCollection',
    'StorageController',
    'StorageControllerCollection',
    'Task',
    'TaskCollection',
    'TaskService',
    'TelemetryService',
    'Thermal',
    'Triggers',
    'TriggersCollection',
    'UpdateService',
    'VLanNetworkInterfaceCollection',
    'VLanNetworkInterface',
    'VirtualMedia',
    'VirtualMediaCollection',
    'odata',
    'odata-v4',
    'redfish-error',
    'redfish-payload-annotations',
    'redfish-schema',
    'redfish-schema-v1',
]

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

proxies = {
    'https': os.environ.get("https_proxy", None)
}

r = requests.get(
    'https://www.dmtf.org/sites/default/files/standards/documents/' +
    VERSION +
    '.zip',
    proxies=proxies)

r.raise_for_status()


static_path = os.path.realpath(os.path.join(SCRIPT_DIR, "..", "static",
                                            "redfish", "v1"))


cpp_path = os.path.realpath(os.path.join(SCRIPT_DIR, "..", "redfish-core",
                                         "include"))


schema_path = os.path.join(static_path, "schema")
json_schema_path = os.path.join(static_path, "JsonSchemas")

zipBytesIO = BytesIO(r.content)
zip_ref = zipfile.ZipFile(zipBytesIO)

# Remove the old files
skip_prefixes = ('Oem')
if os.path.exists(schema_path):
    files = [os.path.join(schema_path, f) for f in os.listdir(schema_path)
             if not f.startswith(skip_prefixes)]
    for f in files:
        os.remove(f)
if os.path.exists(json_schema_path):
    files = [os.path.join(json_schema_path, f) for f in
             os.listdir(json_schema_path) if not f.startswith(skip_prefixes)]
    for f in files:
        if (os.path.isfile(f)):
            os.remove(f)
        else:
            shutil.rmtree(f)

if not os.path.exists(schema_path):
    os.makedirs(schema_path)
if not os.path.exists(json_schema_path):
    os.makedirs(json_schema_path)

schema_versions = defaultdict(list)

schema_files = {}
for zip_filepath in zip_ref.namelist():
    if zip_filepath.startswith(os.path.join('json-schema/')):
        filename = os.path.basename(zip_filepath)
        filenamesplit = filename.split(".")

        # exclude schemas again to save flash space
        if filenamesplit[0] not in include_list:
            continue

        if len(filenamesplit) == 3:
            thisSchemaVersion = schema_files.get(filenamesplit[0], None)
            if thisSchemaVersion is None:
                schema_files[filenamesplit[0]] = filenamesplit[1]
            else:
                # need to see if we're a newer version.
                if list(map(int, filenamesplit[1][1:].split("_"))) > list(map(
                        int, thisSchemaVersion[1:].split("_"))):
                    schema_files[filenamesplit[0]] = filenamesplit[1]


for schema, version in schema_files.items():
    basename = schema + "." + version + ".json"
    zip_filepath = os.path.join("json-schema", basename)
    schemadir = os.path.join(json_schema_path, schema)
    os.makedirs(schemadir)

    with open(os.path.join(schemadir, schema + ".json"), 'wb') as schema_file:
        schema_file.write(zip_ref.read(zip_filepath).replace(b'\r\n', b'\n'))

with open(os.path.join(cpp_path, "schemas.hpp"), 'w') as hpp_file:
    hpp_file.write(
        "#pragma once\n"
        "{WARNING}\n"
        "#include <array>\n"
        "#include <schema_common.hpp>\n"
        "// clang-format off\n"
        "\n"
        "namespace redfish\n"
        "{{\n"
        .format(
            WARNING=WARNING))

    for schema_namespace, versions in schema_versions.items():
        varname = schema_namespace[0].lower() + schema_namespace[1:] + "Type"
        newest_version = versions[-1]

        hpp_file.write(
            "    constexpr SchemaVersion {}{{\n"
            "        \"{}\",\n"
            "        \"{}\"\n"
            "    }};\n\n".format(
                varname,
                schema_namespace,
                versions[-1],
                schema_namespace
            )
        )

    hpp_file.write(
        "    constexpr const std::array<const SchemaVersion*, {}> schemas {{\n".format(len(schema_versions))
    )

    for schema_namespace, versions in schema_versions.items():
        varname = schema_namespace[0].lower() + schema_namespace[1:] + "Type"
        hpp_file.write(
            "        &{},\n"
            .format(varname)
        )


    hpp_file.write(
        "    };"
        "\n"
        "\n"
    )



    hpp_file.write(
        "}\n"
    )

zip_ref.close()
