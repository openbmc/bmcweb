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

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

proxies = {
    'https': os.environ.get("https_proxy", None)
}

r = requests.get('https://www.dmtf.org/sites/default/files/standards/documents/'
                 'DSP8010_2018.1.zip', proxies=proxies)

r.raise_for_status()

static_path = os.path.realpath(os.path.join(SCRIPT_DIR, "..", "static",
                                            "redfish", "v1"))

schema_path = os.path.join(static_path, "schema")
json_schema_path = os.path.join(static_path, "JsonSchemas")

zipBytesIO = BytesIO(r.content)
zip_ref = zipfile.ZipFile(zipBytesIO)

# Remove the old files
if os.path.exists(schema_path):
    shutil.rmtree(schema_path)
if os.path.exists(json_schema_path):
    shutil.rmtree(json_schema_path)
os.makedirs(schema_path)
os.makedirs(json_schema_path)

for zip_filepath in zip_ref.namelist():

    if zip_filepath.startswith('metadata/'):
        filename = os.path.basename(zip_filepath)
        with open(os.path.join(schema_path, filename), 'wb') as schema_file:
            schema_file.write(zip_ref.read(zip_filepath))

schema_files = {}
for zip_filepath in zip_ref.namelist():
    if zip_filepath.startswith('json-schema/'):
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
    zip_filepath = os.path.join("json-schema", basename)
    schemadir = os.path.join(json_schema_path, schema)
    os.makedirs(schemadir)
    with open(os.path.join(schemadir, "index.json"), 'wb') as schema_file:
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
