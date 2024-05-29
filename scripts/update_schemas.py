#!/usr/bin/env python3
import os
import shutil
import zipfile
from io import BytesIO

import generate_schema_enums
import requests
from generate_schema_collections import generate_top_collections

VERSION = "DSP8010_2024.1"

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

shutil.rmtree(schema_path)
os.makedirs(schema_path)

shutil.rmtree(json_schema_path)
os.makedirs(json_schema_path)

csdl_filenames = []
json_schema_files = []

for zip_file in zip_ref.infolist():
    if zip_file.is_dir():
        continue
    if zip_file.filename.startswith("csdl/"):
        csdl_filenames.append(os.path.basename(zip_file.filename))
    elif zip_file.filename.startswith("json-schema/"):
        json_schema_files.append(os.path.basename(zip_file.filename))
    elif zip_file.filename.startswith("openapi/"):
        pass
    elif zip_file.filename.startswith("dictionaries/"):
        pass

for csdl_file in csdl_filenames:
    with open(os.path.join(schema_path, csdl_file), "wb") as schema_out:
        content = zip_ref.read(os.path.join("csdl", csdl_file))
        content = content.replace(b"\r\n", b"\n")
        schema_out.write(content)

for schema_file in json_schema_files:
    zip_filepath = os.path.join("json-schema", schema_file)

    with open(
        os.path.join(json_schema_path, schema_file), "wb"
    ) as schema_file:
        content = zip_ref.read(zip_filepath)
        content = content.replace(b"\r\n", b"\n")
        schema_file.write(content)

zip_ref.close()

generate_schema_enums.main()
generate_top_collections()
