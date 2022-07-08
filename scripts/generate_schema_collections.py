#!/usr/bin/python3

# Script to generate resource collection URIs
# Parses the Redfish schema to determine what are the resource collection URIs.
# These URIs are added to an unordered_set.  This set is used by Redfish
# Aggregation to determine if resource that exists on a satellite BMC but not
# the aggregating BMC needs to be added to the response when querying the
# service root.

import os
import xml.etree.ElementTree as ET
import shutil


SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
REDFISH_SCHEMA_DIR = os.path.realpath(
    os.path.join(SCRIPT_DIR, "..", "static", "redfish", "v1", "schema")
)
OUTFILE = os.path.realpath(
    os.path.join(SCRIPT_DIR, "..", "redfish-core", "include", "redfish_collections.hpp")
)

# Odata string types
EDMX = "{http://docs.oasis-open.org/odata/ns/edmx}"
EDM = "{http://docs.oasis-open.org/odata/ns/edm}"


def parse_file(filename):
    tree = ET.parse(filename)
    root = tree.getroot()
    results = []
    data_services = root.findall("DataServices")
    for ds in data_services:
        for element in ds:
            if element.tag == EDM + "Schema":
                results.extend(parse_schema(element, filename))
    return results

def generate_collections(flat_list):
    # Clear our old results if they exist
    if os.path.exists(OUTFILE):
        shutil.rmfile(OUTFILE)



def main():
    printf("Reading from {}".format(REDFISH_SCHEMA_DIR))
    dir_list = os.listdir(REDFISH_SCHEMA_DIR)

    filepaths = [
        os.path.join(REDFISH_SCHEMA_DIR, filename) for filename in dir_list
    ]

    collection_list = []

    for filepath in filepaths:
        out = parse_file(filepath)
        collection_list.extend(out)

    print("Parsing done")

    generate_collections(collection_list)


if __name__ == '__main__':
    main()
