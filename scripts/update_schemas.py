#!/usr/bin/env python3
import os
import shutil
import subprocess
import tempfile
from collections import OrderedDict, defaultdict

import generate_schema_enums
from generate_schema_collections import generate_top_collections

REPO_URL = "https://github.com/DMTF/Redfish-Publications.git"
TAG = "2025.4"

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

redfish_core_path = os.path.join(SCRIPT_DIR, "..", "redfish-core")

schema_path = os.path.join(redfish_core_path, "schema", "dmtf", "csdl")
json_schema_path = os.path.join(
    redfish_core_path, "schema", "dmtf", "json-schema"
)

schema_installed_path = os.path.join(schema_path, "..", "installed")
json_schema_installed_path = json_schema_path + "-installed"


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


with tempfile.TemporaryDirectory() as repo_dir:
    subprocess.run(
        [
            "git",
            "clone",
            "--depth=1",
            "--branch",
            TAG,
            REPO_URL,
            repo_dir,
        ],
        check=True,
    )

    repo_csdl_dir = os.path.join(repo_dir, "csdl")
    repo_json_dir = os.path.join(repo_dir, "json-schema")

    shutil.rmtree(schema_path)
    os.makedirs(schema_path)

    shutil.rmtree(json_schema_path)
    os.makedirs(json_schema_path)

    csdl_filenames = sorted(os.listdir(repo_csdl_dir))

    json_schema_files = defaultdict(list)
    for filename in os.listdir(repo_json_dir):
        json_schema_files[filename.split(".")[0]].append(filename)

    for value in json_schema_files.values():
        value.sort(key=SchemaVersion, reverse=True)

    json_schema_files = OrderedDict(
        sorted(json_schema_files.items(), key=lambda x: SchemaVersion(x[0]))
    )

    csdl_installed_symlinks = set(os.listdir(schema_installed_path))
    shutil.rmtree(schema_installed_path)
    os.makedirs(schema_installed_path)

    for csdl_file in csdl_filenames:
        shutil.copy(
            os.path.join(repo_csdl_dir, csdl_file),
            os.path.join(schema_path, csdl_file),
        )

        if csdl_file in csdl_installed_symlinks:
            os.symlink(
                os.path.join("..", "csdl", csdl_file),
                os.path.join(schema_installed_path, csdl_file),
            )

    json_base_symlinks = defaultdict(list)
    for json_symlink in os.listdir(json_schema_installed_path):
        base_json_name = json_symlink.split(".")[0]
        json_base_symlinks[base_json_name].append(json_symlink)

    shutil.rmtree(json_schema_installed_path)
    os.makedirs(json_schema_installed_path)

    for schema_filename, versions in json_schema_files.items():
        shutil.copy(
            os.path.join(repo_json_dir, versions[0]),
            os.path.join(json_schema_path, versions[0]),
        )

        if schema_filename in json_base_symlinks:
            os.symlink(
                os.path.join("..", "json-schema", versions[0]),
                os.path.join(json_schema_installed_path, versions[0]),
            )

generate_schema_enums.main()
generate_top_collections()
