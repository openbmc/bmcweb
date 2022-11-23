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
    os.path.join(
        SCRIPT_DIR, "..", "redfish-core", "include", "aggregation_utils.hpp"
    )
)


# Odata string types
EDMX = "{http://docs.oasis-open.org/odata/ns/edmx}"
EDM = "{http://docs.oasis-open.org/odata/ns/edm}"

seen_paths = set()


def parse_node(target_entitytype, path, top_collections, found_top, xml_file):
    filepath = os.path.join(REDFISH_SCHEMA_DIR, xml_file)
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

    parse_root(
        root, target_entitytype, path, top_collections, found_top, xml_map
    )


# Given a root node we want to parse the tree to find all instances of a
# specific EntityType.  This is a separate routine so that we can rewalk the
# current tree when a NavigationProperty Type links to the current file.
def parse_root(
    root, target_entitytype, path, top_collections, found_top, xml_map
):
    ds = root.find(EDMX + "DataServices")
    for schema in ds.findall(EDM + "Schema"):
        for entity_type in schema.findall(EDM + "EntityType"):
            name = entity_type.get("Name")
            if name != target_entitytype:
                continue
            for nav_prop in entity_type.findall(EDM + "NavigationProperty"):
                parse_navigation_property(
                    root,
                    name,
                    nav_prop,
                    path,
                    top_collections,
                    found_top,
                    xml_map,
                )

        # These ComplexType objects contain links to actual resources or
        # resource collections
        for complex_type in schema.findall(EDM + "ComplexType"):
            name = complex_type.get("Name")
            if name != target_entitytype:
                continue
            for nav_prop in complex_type.findall(EDM + "NavigationProperty"):
                parse_navigation_property(
                    root,
                    name,
                    nav_prop,
                    path,
                    top_collections,
                    found_top,
                    xml_map,
                )


# Helper function which expects a NavigationProperty to be passed in.  We need
# this because NavigationProperty appears under both EntityType and ComplexType
def parse_navigation_property(
    root, curr_entitytype, element, path, top_collections, found_top, xml_map
):
    if element.tag != (EDM + "NavigationProperty"):
        return

    # We don't want to actually parse this property if it's just an excerpt
    for annotation in element.findall(EDM + "Annotation"):
        term = annotation.get("Term")
        if term == "Redfish.ExcerptCopy":
            return

    # We don't want to aggregate JsonSchemas as well as anything under
    # AccountService or SessionService
    nav_name = element.get("Name")
    if nav_name in ["JsonSchemas", "AccountService", "SessionService"]:
        return

    nav_type = element.get("Type")
    if "Collection" in nav_type:
        # Type is either Collection(<Namespace>.<TypeName>) or
        # Collection(<NamespaceName>.<NamespaceVersion>.<TypeName>)
        if nav_type.startswith("Collection"):
            qualified_name = nav_type.split("(")[1].split(")")[0]
            # Do we need to parse this file or another file?
            qualified_name_split = qualified_name.split(".")
            if len(qualified_name_split) == 3:
                typename = qualified_name_split[2]
            else:
                typename = qualified_name_split[1]
            file_key = qualified_name_split[0]

            # If we contain a collection array then we don't want to add the
            # name to the path if we're a collection schema
            if nav_name != "Members":
                path += "/" + nav_name
                if path in seen_paths:
                    return
                seen_paths.add(path)

                # Did we find the top level collection in the current path or
                # did we previously find it?
                if not found_top:
                    top_collections.add(path)
                    found_top = True

            member_id = typename + "Id"
            prev_count = path.count(member_id)
            if prev_count:
                new_path = path + "/{" + member_id + str(prev_count + 1) + "}"
            else:
                new_path = path + "/{" + member_id + "}"

        # type is "<Namespace>.<TypeName>", both should end with "Collection"
        else:
            # Escape if we've found a circular dependency like SubProcessors
            if path.count(nav_name) >= 2:
                return

            nav_type_split = nav_type.split(".")
            if (len(nav_type_split) != 2) and (
                nav_type_split[0] != nav_type_split[1]
            ):
                # We ended up with something like Resource.ResourceCollection
                return
            file_key = nav_type_split[0]
            typename = nav_type_split[1]
            new_path = path + "/" + nav_name

        # Did we find the top level collection in the current path or did we
        # previously find it?
        if not found_top:
            top_collections.add(new_path)
            found_top = True

    # NavigationProperty is not for a collection
    else:
        # Bail if we've found a circular dependency like MetricReport
        if path.count(nav_name):
            return

        new_path = path + "/" + nav_name
        nav_type_split = nav_type.split(".")
        file_key = nav_type_split[0]
        typename = nav_type_split[1]

    # We need to specially handle certain URIs since the Name attribute from the
    # schema is not used as part of the path
    # TODO: Expand this section to add special handling across the entirety of
    # the Redfish tree
    new_path2 = ""
    if new_path == "/redfish/v1/Tasks":
        new_path2 = "/redfish/v1/TaskService"

    # If we had to apply special handling then we need to remove the inital
    # version of the URI if it was previously added
    if new_path2 != "":
        if new_path in seen_paths:
            seen_paths.remove(new_path)
        new_path = new_path2

    # No need to parse the new URI if we've already done so
    if new_path in seen_paths:
        return
    seen_paths.add(new_path)

    # We can stop parsing if we've found a top level collection
    # TODO: Don't return here when we want to walk the entire tree instead
    if found_top:
        return

    # If the namespace of the NavigationProperty's Type is not in our xml map
    # then that means it inherits from elsewhere in the current file
    if file_key in xml_map:
        parse_node(
            typename, new_path, top_collections, found_top, xml_map[file_key]
        )
    else:
        parse_root(
            root, typename, new_path, top_collections, found_top, xml_map
        )


def generate_top_collections():
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
    seen_paths.add(curr_path)
    parse_node(
        "ServiceRoot", curr_path, top_collections, False, "ServiceRoot_v1.xml"
    )

    print("Finished traversal!")

    TOTAL = len(top_collections)
    with open(CPP_OUTFILE, "w") as hpp_file:
        hpp_file.write(
            "#pragma once\n"
            "{WARNING}\n"
            "// clang-format off\n"
            "#include <array>\n"
            "#include <string_view>\n"
            "\n"
            "namespace redfish\n"
            "{{\n"
            '// Note that each URI actually begins with "/redfish/v1"\n'
            "// They've been omitted to save space and reduce search time\n"
            "constexpr std::array<std::string_view, {TOTAL}> "
            "topCollections{{\n".format(WARNING=WARNING, TOTAL=TOTAL)
        )

        for collection in sorted(top_collections):
            # All URIs start with "/redfish/v1".  We can omit that portion to
            # save memory and reduce lookup time
            hpp_file.write(
                '    "{}",\n'.format(collection.split("/redfish/v1")[1])
            )

        hpp_file.write("};\n} // namespace redfish\n")
