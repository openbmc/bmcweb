#!/usr/bin/env python3

# Script to generate top level resource collection URIs
# Parses the Redfish schema to determine what are the top level collection URIs
# and writes them to a generated .hpp file as an unordered_set.  Also generates
# a map of URIs that contain a top level collection as part of their subtree.
# Those URIs as well as those of their immediate children as written as an
# unordered_map.  These URIs are need by Redfish Aggregation

import os
import xml.etree.ElementTree as ET

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
REDFISH_SCHEMA_DIR = os.path.realpath(
    os.path.join(SCRIPT_DIR, "..", "static", "redfish", "v1", "schema")
)
CPP_OUTFILE = os.path.realpath(
    os.path.join(SCRIPT_DIR, "..", "redfish-core", "include",
                 "aggregation_utils.hpp")
)


# Odata string types
EDMX = "{http://docs.oasis-open.org/odata/ns/edmx}"
EDM = "{http://docs.oasis-open.org/odata/ns/edm}"

seen_paths = set()

def parse_node(path, top_collections, found_top, xml_file, depth):
    filepath = os.path.join(REDFISH_SCHEMA_DIR, xml_file)

    # Bail if we are attempting to open a schema we haven't pulled in
    if (not os.path.exists(filepath)):
        return

    # No need to parse this node again
    if (path in seen_paths):
        return

    print("\nxml file is: ", xml_file)
    print("Path is: ", path)
    is_not_collection = not "Collection" in xml_file

    # We need to specially handle certain URIs since the Name attribute from the
    # schema is not used as part of the path
    if (path == "/redfish/v1/Tasks"):
        path = "/redfish/v1/TaskService"

    seen_paths.add(path)
    tree = ET.parse(filepath)
    root = tree.getroot()

    # Map xml URIs to their associated namespace
    xml_map = {}
    for ref in root.findall(EDMX + "Reference"):
        uri = ref.get("Uri")
        if uri is None:
            continue
        file = uri.split("/").pop()
        for inc in ref.findall(EDMX + "Include"):
            namespace = inc.get("Namespace")
            if namespace is None:
                continue
            xml_map[namespace] = file

    # I can't just parse all EntityTypes!  I need to find the ones corresponding
    # to what I'm currently looking at and then search their navigation
    # properties.  Make sure Name of each EntityType is a match.
    #
    # When I parse to a Type=Collection() I need to determine if it's locally
    # defined or defined in a separate file

    ds = root.find(EDMX + "DataServices")
    for schema in ds.findall(EDM + "Schema"):
        for entity_type in schema.findall(EDM + "EntityType"):
            for nav_prop in entity_type.findall(EDM + "NavigationProperty"):
                parse_navigation_property(nav_prop, path, top_collections,
                                          found_top, xml_map, depth,
                                          is_not_collection)

        # These ComplexType objects contain links to actual collections
        for complex_type in schema.findall(EDM + "ComplexType"):
            name = complex_type.get("Name")
            if ((name != "GenZ") and (name != "Boot") and (name != "Ethernet")):
                continue
            for nav_prop in complex_type.findall(EDM + "NavigationProperty"):
                parse_navigation_property(nav_prop, path, top_collections,
                                          found_top, xml_map, depth,
                                          is_not_collection)


# Helper function which expects a NavigationProperty to be passed in.  We need
# this because NavigationProperty appears under both EntityType and ComplexType
def parse_navigation_property(element, path, top_collections, found_top,
                              xml_map, depth, not_in_collection):
    if (element.tag != (EDM + "NavigationProperty")):
        return

    # We don't want to actually parse this property if it's just an excerpt
    for annotation in element.findall(EDM + "Annotation"):
        term = annotation.get("Term")
        if ((term == "Redfish.ExcerptCopy")):
            print("Not so fast my friend")
            return

    # We don't want to aggregate JsonSchemas as well as anything under
    # AccountService or SessionService
    nav_name = element.get("Name")
    if ((nav_name == "JsonSchemas") or
            (nav_name == "AccountService") or
            (nav_name == "SessionService")):
        found_top = True

    # ServiceRoot links to Sessions which we aren't going to aggregate
#    if (nav_name == "Sessions"):
#        found_top = True

    nav_type_split = element.get("Type").split(".")
    if (nav_type_split[0].endswith("Collection") or
            nav_type_split[0].startswith("Collection")):

        # We contain a collection array instead of link to a collection
        if (nav_type_split[0].startswith("Collection")):

            # If we contain a collection array then we don't want to keep
            # parsing unless we're actually a resource collection
            if (not_in_collection):
                #return
                # TODO: Instead we should add the array name to the path and then
                # proceed normally
                path += "/" + nav_name
                seen_paths.add(path)
                print("Intermediate path is: " + path)

            # nav_type should be of the form "Collection(<resource>.<resource>)"
            file_key = nav_type_split[0].split("(")[1]
            if (file_key != nav_type_split[1].split(")")[0]):
                return

            prev_count = path.count(file_key + "Id")
            if (prev_count):
                new_path = (path + "/{" + file_key + "Id" + str(prev_count+1)
                            + "}")
            else:
                new_path = path + "/{" + file_key + "Id}"
        else:
            # Escape if we've found a circular dependency like SubProcessors
            if (path.count(nav_name) >= 2):
                return

            file_key = nav_type_split[0]
            new_path = path + "/" + nav_name

        # Did we find the top level collection in the current path or did we
        # previously find it?
        if (not found_top):
            top_collections.add(new_path)
            found_top = True
#            print("Added top level collection: " + new_path)
    else:
        # Traverse the children until we hit top level collection or
        # run out of nodes in the path

        # Bail if we've found a circular dependency like MetricReport
        #if (path.count(nav_type_split[0])):
        if (path.count(nav_name)):
            return

        #new_path = path + "/" + nav_type_split[0]
        new_path = path + "/" + nav_name
        file_key = nav_type_split[0]

    # Stop walking the tree if we've hit a circular dependency
    if (file_key == "Resource"):
        return
    if ((file_key == "Resource") and path.endswith("Resource")):
        print("Don't go in circles!!!!!!!!")
        return

    # A sensor will only appear under a Sensors collection
    if ((file_key == "Sensor") and (not path.endswith("Sensors"))):
        print("Skip this sensor!")
        return

    # A control will only appear under a Controls collection
    if ((file_key == "Control") and (not path.endswith("Controls"))):
        print("Skip this control!")
        return

    # We need to specially handle certain URIs since the Name attribute from the
    # schema is not used as part of the path
    if (new_path == "/redfish/v1/Tasks"):
        new_path = "/redfish/v1/TaskService"

    seg_len = 0 - (len(new_path) - len(path) - 1)
    new_seg = new_path[seg_len:]
    print("new_seg is: ", new_seg)

    # Keep walking the Redfish tree
    parse_node(new_path, top_collections, found_top, xml_map[file_key],
               depth+1)


def main():
    # We need to separately track top level resources as well as all URIs that
    # are upstream from a top level resource.  We shouldn't combine these into
    # a single structure because:
    #
    # 1) We want direct lookup of top level collections for prefix handling
    # purposes.
    #
    # 2) A top level collection will not always be one level below the service
    # root.  For example, we need to aggregate
    # /redfish/v1/CompositionService/ActivePool and we do not currently support
    # CompositionService.  If a satellite BMC implements it then we would need
    # to display a link to CompositionService under /redfish/v1 even though
    # CompositionService is not a top level collection.

    # Contains URIs for all top level collections
    top_collections = set()

    # Begin parsing from the Service Root
    curr_path = "/redfish/v1"
    parse_node(curr_path, top_collections, False, "ServiceRoot_v1.xml", 2)

    print("Finished traversal!")

    with open(CPP_OUTFILE, "w") as hpp_file:
        hpp_file.write(
            "#pragma once\n"
            "{WARNING}\n"
            "// clang-format off\n"
            "#include <array>\n"
            "\n"
            "namespace redfish\n"
            "{{\n"
            "constexpr std::array "
            "topCollections{{\n".format(WARNING=WARNING)
        )

        for collection in sorted(top_collections):
            hpp_file.write('    "{}",\n'.format(collection))

        hpp_file.write("};\n" "} // namespace redfish\n")


if __name__ == '__main__':
    main()
