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

import xml.etree.ElementTree as ET

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

proxies = {
    'https': os.environ.get("https_proxy", None)
}

r = requests.get('https://www.dmtf.org/sites/default/files/standards/documents/'
                 'DSP8010_2018.3.zip', proxies=proxies)

r.raise_for_status()

static_path = os.path.realpath(os.path.join(SCRIPT_DIR, "..", "static",
                                            "redfish", "v1"))

schema_path = os.path.join(static_path, "schema")
json_schema_path = os.path.join(static_path, "JsonSchemas")
metadata_index_path = os.path.join(static_path, "$metadata", "index.xml")

zipBytesIO = BytesIO(r.content)
zip_ref = zipfile.ZipFile(zipBytesIO)

# Remove the old files
if os.path.exists(schema_path):
    files = glob.glob(os.path.join(schema_path, '[!Oem]*'))
    for f in files:
        os.remove(f)
if os.path.exists(json_schema_path):
    files = glob.glob(os.path.join(json_schema_path, '[!Oem]*'))
    for f in files:
        if (os.path.isfile(f)):
            os.remove(f)
        else:
            shutil.rmtree(f)
os.remove(metadata_index_path)

if not os.path.exists(schema_path):
    os.makedirs(schema_path)
if not os.path.exists(json_schema_path):
    os.makedirs(json_schema_path)

with open(metadata_index_path, 'w') as metadata_index:

    metadata_index.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
    metadata_index.write(
        "<edmx:Edmx xmlns:edmx=\"http://docs.oasis-open.org/odata/ns/edmx\" Version=\"4.0\">\n")

    for zip_filepath in zip_ref.namelist():
        if zip_filepath.startswith('DSP8010_2018.3/csdl/') & (zip_filepath != "DSP8010_2018.3/csdl/"):
            filename = os.path.basename(zip_filepath)
            with open(os.path.join(schema_path, filename), 'wb') as schema_file:

                metadata_index.write(
                    "    <edmx:Reference Uri=\"/redfish/v1/schema/" + filename + "\">\n")

                content = zip_ref.read(zip_filepath)
                xml_root = ET.fromstring(content)

                for edmx_child in xml_root:

                    if edmx_child.tag == "{http://docs.oasis-open.org/odata/ns/edmx}DataServices":
                        for data_child in edmx_child:
                            if data_child.tag == "{http://docs.oasis-open.org/odata/ns/edm}Schema":
                                namespace = data_child.attrib["Namespace"]
                                if namespace.startswith("RedfishExtensions"):
                                    metadata_index.write(
                                        "        <edmx:Include Namespace=\"" + namespace + "\"  Alias=\"Redfish\"/>\n")

                                else:
                                    metadata_index.write(
                                        "        <edmx:Include Namespace=\"" + namespace + "\"/>\n")
                schema_file.write(content)
                metadata_index.write("    </edmx:Reference>\n")

    metadata_index.write("""    <edmx:DataServices>
        <Schema xmlns="http://docs.oasis-open.org/odata/ns/edm" Namespace="Service">
            <EntityContainer Name="Service" Extends="ServiceRoot.v1_0_0.ServiceContainer"/>
        </Schema>
    </edmx:DataServices>
""")
    #TODO:Issue#32 There's a bug in the script that currently deletes this
    #schema (because it's an OEM schema). Because it's the only one, and we
    #don't update schemas very often, we just manually fix it. Need a
    #permanent fix to the script.
    metadata_index.write("    <edmx:Reference Uri=\"/redfish/v1/schema/OemManager_v1.xml\"/>\n")
    metadata_index.write("        <edmx:Include Namespace=\"OemManager\"/>\n")
    metadata_index.write("    </edmx:Reference>\n")
    metadata_index.write("</edmx:Edmx>\n")

schema_files = {}
for zip_filepath in zip_ref.namelist():
    if zip_filepath.startswith('DSP8010_2018.3/json-schema/'):
        filename = os.path.basename(zip_filepath)
        filenamesplit = filename.split(".")
        if len(filenamesplit) == 3:
            thisSchemaVersion = schema_files.get(filenamesplit[0], None)
            if thisSchemaVersion == None:
                schema_files[filenamesplit[0]] = filenamesplit[1]
            else:
                # need to see if we're a newer version.
                if list(map(int, filenamesplit[1][1:].split("_"))) > list(map(
                        int, thisSchemaVersion[1:].split("_"))):
                    schema_files[filenamesplit[0]] = filenamesplit[1]


for schema, version in schema_files.items():
    basename = schema + "." + version + ".json"
    zip_filepath = os.path.join("DSP8010_2018.3/json-schema", basename)
    schemadir = os.path.join(json_schema_path, schema)
    os.makedirs(schemadir)
    location_json = OrderedDict()
    location_json["Language"] = "en"
    location_json["PublicationUri"] = (
        "http://redfish.dmtf.org/schemas/v1/" + schema + ".json")
    location_json["Uri"] = (
        "/redfish/v1/JsonSchemas/" + schema + "/" + schema + ".json")

    index_json = OrderedDict()
    index_json["@odata.context"] = "/redfish/v1/$metadata#JsonSchemaFile.JsonSchemaFile"
    index_json["@odata.id"] = "/redfish/v1/JsonSchemas/" + schema
    index_json["@odata.type"] = "#JsonSchemaFile.v1_0_2.JsonSchemaFile"
    index_json["Name"] = schema + " Schema File"
    index_json["Schema"] = "#" + schema + "." + schema
    index_json["Description"] =  schema + " Schema File Location"
    index_json["Id"] = schema
    index_json["Languages"] = [ "en" ]
    index_json["Languages@odata.count"] = 1
    index_json["Location"] = [ location_json ]
    index_json["Location@odata.count"] = 1

    with open(os.path.join(schemadir, "index.json"), 'w') as schema_file:
        json.dump(index_json, schema_file, indent=4)
    with open(os.path.join(schemadir, schema + ".json"), 'wb') as schema_file:
        schema_file.write(zip_ref.read(zip_filepath))

with open(os.path.join(json_schema_path, "index.json"), 'w') as index_file:
    members = [{"@odata.id": "/redfish/v1/JsonSchemas/" + schema + "/"}
               for schema in schema_files]

    members.sort(key=lambda x: x["@odata.id"])

    indexData = OrderedDict()

    indexData["@odata.id"] = "/redfish/v1/JsonSchemas"
    indexData["@odata.context"] = ("/redfish/v1/$metadata"
                                   "#JsonSchemaFileCollection."
                                   "JsonSchemaFileCollection")
    indexData["@odata.type"] = ("#JsonSchemaFileCollection."
                                "JsonSchemaFileCollection")
    indexData["Name"] = "JsonSchemaFile Collection"
    indexData["Description"] = "Collection of JsonSchemaFiles"
    indexData["Members@odata.count"] = len(schema_files)
    indexData["Members"] = members

    json.dump(indexData, index_file, indent=2)

zip_ref.close()
