#!/usr/bin/env python3
import os
import shutil
import zipfile
from collections import OrderedDict, defaultdict
from io import BytesIO

import generate_schema_enums
import requests
from generate_schema_collections import generate_top_collections

VERSION = "DSP8010_2025.1"

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

proxies = {"https": os.environ.get("https_proxy", None)}

r = requests.get(
    "https://www.dmtf.org/sites/default/files/standards/documents/"
    + VERSION
    + ".zip",
    proxies=proxies,
)

r.raise_for_status()

redfish_core_path = os.path.join(SCRIPT_DIR, "..", "redfish-core")

cpp_path = os.path.realpath(os.path.join(redfish_core_path, "include"))

schema_path = os.path.join(redfish_core_path, "schema", "dmtf", "csdl")
json_schema_path = os.path.join(
    redfish_core_path, "schema", "dmtf", "json-schema"
)

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


shutil.rmtree(schema_path)
os.makedirs(schema_path)

shutil.rmtree(json_schema_path)
os.makedirs(json_schema_path)

csdl_filenames = []
json_schema_files = defaultdict(list)

for zip_file in zip_ref.infolist():
    if zip_file.is_dir():
        continue
    if zip_file.filename.startswith(VERSION + "/csdl/"):
        csdl_filenames.append(os.path.basename(zip_file.filename))
    elif zip_file.filename.startswith(VERSION + "/json-schema/"):
        filename = os.path.basename(zip_file.filename)
        filenamesplit = filename.split(".")
        json_schema_files[filenamesplit[0]].append(filename)
    elif zip_file.filename.startswith(VERSION + "/openapi/"):
        pass
    elif zip_file.filename.startswith(VERSION + "/dictionaries/"):
        pass

# sort the json files by version
for key, value in json_schema_files.items():
    value.sort(key=SchemaVersion, reverse=True)

# Create a dictionary ordered by schema name
json_schema_files = OrderedDict(
    sorted(json_schema_files.items(), key=lambda x: SchemaVersion(x[0]))
)


for csdl_file in csdl_filenames:
    with open(os.path.join(schema_path, csdl_file), "wb") as schema_out:
        content = zip_ref.read(os.path.join(VERSION + "/csdl", csdl_file))
        content = content.replace(b"\r\n", b"\n")
        schema_out.write(content)

for schema_filename, versions in json_schema_files.items():
    zip_filepath = os.path.join(VERSION + "/json-schema", versions[0])

    with open(
        os.path.join(json_schema_path, versions[0]), "wb"
    ) as schema_file:
        content = zip_ref.read(zip_filepath)
        content = content.replace(b"\r\n", b"\n")
        schema_file.write(content)

zip_ref.close()

generate_schema_enums.main()
generate_top_collections()
