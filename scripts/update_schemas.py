#!/usr/bin/env python3
import os
import shutil
import zipfile
from collections import OrderedDict, defaultdict
from io import BytesIO

import generate_schema_enums
import requests
from generate_schema_collections import generate_top_collections

VERSION = "DSP8010_2024.1"

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


schema_path = os.path.join(
    SCRIPT_DIR, "..", "redfish-core", "schema", "dmtf", "csdl"
)
json_schema_path = os.path.join(
    SCRIPT_DIR, "..", "redfish-core", "schema", "dmtf", "json-schema"
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
    if zip_file.filename.startswith("csdl/"):
        csdl_filenames.append(os.path.basename(zip_file.filename))
    elif zip_file.filename.startswith("json-schema/"):
        filename = os.path.basename(zip_file.filename)
        filenamesplit = filename.split(".")
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
for csdl_file in csdl_filenames:
    with open(os.path.join(schema_path, csdl_file), "wb") as schema_out:
        content = zip_ref.read(os.path.join("csdl", csdl_file))
        content = content.replace(b"\r\n", b"\n")
        schema_out.write(content)

for schema, versions in json_schema_files.items():
    for version in versions:
        zip_filepath = os.path.join("json-schema", version)

        with open(
            os.path.join(json_schema_path, version), "wb"
        ) as schema_file:
            schema_file.write(
                zip_ref.read(zip_filepath).replace(b"\r\n", b"\n")
            )

with open(os.path.join(json_schema_path, "meson.build"), "w") as meson_file:
    for schema, version in json_schema_files.items():
        meson_file.write(
            f"install_data(\n"
            f"    '{version[0]}',\n"
            f"    install_dir: 'share/www/redfish/v1/JsonSchema/{schema}',\n"
            ")\n"
        )

with open(os.path.join(cpp_path, "schemas.hpp"), "w") as hpp_file:
    schemas = []
    for root, dirs, files in os.walk(
        os.path.join(
            SCRIPT_DIR, "..", "redfish-core", "schema", "dmtf", "installed"
        )
    ):
        for csdl_file in sorted(files, key=SchemaVersion):
            if csdl_file.endswith("_v1.xml"):
                schemas.append(csdl_file.replace("_v1.xml", ""))
    hpp_file.write(
        "#pragma once\n"
        "{WARNING}\n"
        "// clang-format off\n"
        "#include <array>\n"
        "#include <string_view>\n"
        "\n"
        "namespace redfish\n"
        "{{\n"
        "    constexpr std::array<std::string_view,{SIZE}> schemas {{\n".format(
            WARNING=WARNING,
            SIZE=len(schemas),
        )
    )
    for schema in schemas:
        hpp_file.write('        "{}",\n'.format(schema))

    hpp_file.write("    };\n}\n")

zip_ref.close()

generate_schema_enums.main()
generate_top_collections()
