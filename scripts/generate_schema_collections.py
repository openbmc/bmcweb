#!/usr/bin/python3

# Script to generate resource collection URIs
# Parses the Redfish schema to determine what are the resource collection URIs.
# These URIs are added to an unordered_set.  This set is used by Redfish
# Aggregation to determine if resource that exists on a satellite BMC but not
# the aggregating BMC needs to be added to the response when querying the
# service root.

import os
import xml.etree.ElementTree as ET


SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
REDFISH_SCHEMA_DIR = os.path.realpath(
    os.path.join(SCRIPT_DIR, "..", "static", "redfish", "v1", "schema")
)
OUTFILE = os.path.realpath(
    os.path.join(SCRIPT_DIR, "..", "redfish-core", "include",
                 "redfish_collections.hpp")
)

# Odata string types
EDMX = "{http://docs.oasis-open.org/odata/ns/edmx}"
EDM = "{http://docs.oasis-open.org/odata/ns/edm}"


def parse_node(path, collection_parents, top_collections, xml_file):
    collection_in_subtree = False
    filepath = os.path.join(REDFISH_SCHEMA_DIR, xml_file)
    print("filepath is: " + filepath)
    tree = ET.parse(filepath)
    root = tree.getroot()

    # Map xml URIs to their associated namespace
    xml_map = {}
    for ref in root.findall(EDMX + "Reference"):
        uri = ref.get("Uri")
        if uri is None:
            continue
        file = uri.split("/").pop()
        for ref_child in ref.iter(EDMX + "Include"):
            ns = ref_child.get("Namespace")
            if ns is None:
                continue
            xml_map[ns] = file

    ds = root.find(EDMX + "DataServices")
    contain_collections = set()
    local_col = set()
    for ds_child in ds.iter(EDM + "Schema"):
        for navprop in ds_child.iter(EDM + "NavigationProperty"):
            nav_name = navprop.get("Name")
            nav_type_split = navprop.get("Type").split(".")
            new_path = path + "/" + nav_name
            if (nav_type_split[0].endswith("Collection") or
                nav_type_split[0].startswith("Collection")):
                top_collections.add(new_path)
                local_col.add(new_path)
                collection_in_subtree = True
                print("Added collection: " + new_path)
            else:
                # Traverse the children until we hit top level collection
                print("We gotta go deeper for: " + new_path + ", type: " + navprop.get("Type"))
                if (parse_node(new_path, collection_parents, top_collections, xml_map[nav_type_split[0]])):
                    collection_in_subtree = True
                    local_col.add(new_path)

    if len(local_col) > 0:
        collection_parents[path] = local_col

    return collection_in_subtree


def main():
    print("Reading from {}".format(REDFISH_SCHEMA_DIR))
    dir_list = os.listdir(REDFISH_SCHEMA_DIR)

    filepaths = [
        os.path.join(REDFISH_SCHEMA_DIR, filename) for filename in dir_list
    ]

    # Should I make a map of <key, set>?
    # key = URI containing collections
    # set = collection URIs
    master_map = {}


    # In the end we'll need to know top level collection URIs for prefix
    # matching purposes.  Maybe we can construct this at the end by iterating
    # over the values in the master_map
    #
    # NO! We can't because a top level collection won't always be one level
    # deep.  As an example, we need to aggregate
    # /redfish/v1/UpdateService/FirmwareInventory as a special case.  In that
    # instance we should make sure that /redfish/v1 can include a link to
    # /redfish/v1/UpdateService even though UpdateService is not a collection.

    top_collections = set()

    # Begin parsing from the Service Root
    curr_path = "/redfish/v1"
    parse_node(curr_path, master_map, top_collections, "ServiceRoot_v1.xml")

    print("Finished traversal!")

    keys = []
    for key in master_map:
        keys.append(key)

    for key in sorted(keys):
        print(key)
        for val in sorted(master_map[key]):
            print("    ", val)
        print("\n")

    # TODOME: Generate cpp file(s) containing the unordered set and map

if __name__ == '__main__':
    main()
