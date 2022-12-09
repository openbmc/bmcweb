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


def parse_node(
    target_entitytype,
    path,
    top_collections,
    uri_to_entity_map,
    found_top,
    xml_file,
):
    filename = xml_file.split("_")[0]

    filepath = os.path.join(REDFISH_SCHEMA_DIR, xml_file)

    # Bail if we are attempting to open a schema we haven't pulled in.  This
    # catches Swordfish schema files
    if not os.path.exists(filepath):
        return

    uri_to_entity_map[path] = filename

    tree = ET.parse(filepath)
    root = tree.getroot()

    # Map xml URIs to their associated namespace
    xml_map = {}
    for ref in root:
        if ref.tag == EDMX + "Reference":
            uri = ref.get("Uri")
            if uri is None:
                continue
            file = uri.split("/").pop()
            for inc in ref:
                if inc.tag == EDMX + "Include":
                    namespace = inc.get("Namespace")
                    if namespace is None:
                        continue
                    xml_map[namespace] = file

    parse_root(
        root,
        target_entitytype,
        path,
        top_collections,
        uri_to_entity_map,
        found_top,
        xml_map,
    )


# Given a root node we want to parse the tree to find all instances of a
# specific EntityType.  This is a separate routine so that we can rewalk the
# current tree when a NavigationProperty Type links to the current file.
def parse_root(
    root,
    target_entitytype,
    path,
    top_collections,
    uri_to_entity_map,
    found_top,
    xml_map,
):
    ds = root.find(EDMX + "DataServices")
    for schema in ds:
        if schema.tag == EDM + "Schema":
            for element in schema:
                name = element.get("Name")
                if element.tag == EDM + "EntityType":
                    if name != target_entitytype:
                        continue
                    for nav_prop in element:
                        if nav_prop.tag == EDM + "NavigationProperty":
                            parse_navigation_property(
                                root,
                                name,
                                nav_prop,
                                path,
                                top_collections,
                                uri_to_entity_map,
                                found_top,
                                xml_map,
                            )

                elif element.tag == EDM + "Action":
                    # We have to generate some uris based off of Actions and Action Info
                    schema_namespace = schema.get("Namespace")

                    # ActionInfo's naming convention is
                    # path + {Action Name} + "ActionInfo"
                    action_info_path = path + "/" + name + "ActionInfo"
                    uri_to_entity_map[action_info_path] = "ActionInfo"

                    # Action's naming convention is
                    # path + {Schema Namespace} + "." + {Action name}
                    action_path = (
                        path + "/Actions/" + schema_namespace + "." + name
                    )
                    uri_to_entity_map[action_path] = schema_namespace

                # These ComplexType objects contain links to actual resources or
                # resource collections
                elif element.tag == EDM + "ComplexType":
                    if name != target_entitytype:
                        continue
                    for nav_prop in element:
                        if nav_prop.tag == EDM + "NavigationProperty":
                            parse_navigation_property(
                                root,
                                name,
                                nav_prop,
                                path,
                                top_collections,
                                uri_to_entity_map,
                                found_top,
                                xml_map,
                            )


# Helper function which expects a NavigationProperty to be passed in.  We need
# this because NavigationProperty appears under both EntityType and ComplexType
def parse_navigation_property(
    root,
    curr_entitytype,
    element,
    path,
    top_collections,
    uri_to_entity_map,
    found_top,
    xml_map,
):
    # We don't want to actually parse this property if it's just an excerpt
    for annotation in element:
        if annotation.tag == EDM + "Annotation":
            term = annotation.get("Term")
            if term == "Redfish.ExcerptCopy":
                return

    # We don't want to aggregate JsonSchemas as well as anything under
    # AccountService or SessionService
    nav_name = element.get("Name")
    if nav_name in ["JsonSchemas", "AccountService", "SessionService"]:
        found_top = True

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

            new_path = path + "/[*]"

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
    seg_len = -(len(new_path) - len(path) - 1)
    new_seg = new_path[seg_len:]

    # These are "Property" instead of "NavigationProperty", but should still
    # appear in the path
    if new_seg == "AccountService" or new_seg == "RemoteAccountService":
        print(f"nav_name{nav_name}")
        for node_name in ("ActiveDirectory", "LDAP"):
            parse_node(
                "ExternalAccountProvider",
                new_path + "/" + node_name,
                top_collections,
                uri_to_entity_map,
                found_top,
                xml_map[file_key],
            )

    # Other Properties treated this way are:
    if new_path == "/Tasks":
        new_path = "/TaskService"

    new_path2 = ""
    if "ComputerSystems" in new_path:
        new_path2 = new_path.replace("ComputerSystems", "Systems")
    # For Metrics sometimes the TypeName from the Type attribute is used
    # instead of the Name attribute
    if new_seg == "Metrics":
        if curr_entitytype in ["Processor", "Memory", "Switch"]:
            new_path2 = path + "/" + nav_type_split[1]

    elif new_seg == "RequestorVCAT":
        new_path2 = path + "/" + "REQ-VCAT"

    elif new_seg == "ResponderVCAT":
        new_path2 = path + "/" + "RSP-VCAT"

    elif new_seg == "AdditionalExternalAccountProviders":
        new_path2 = path + "/" + "ExternalAccountProviders"

    elif (new_seg == "Log") and (curr_entitytype == "Memory"):
        new_path = path + "/" + "DeviceLog"

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

    # If the namespace of the NavigationProperty's Type is not in our xml map
    # then that means it inherits from elsewhere in the current file
    if file_key in xml_map:
        parse_node(
            typename,
            new_path,
            top_collections,
            uri_to_entity_map,
            found_top,
            xml_map[file_key],
        )
    else:
        parse_root(
            root,
            typename,
            new_path,
            top_collections,
            uri_to_entity_map,
            found_top,
            xml_map,
        )


def generate_top_collections(uri_to_entity_map, top_collections):
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
    with open(CPP_OUTFILE, "w") as registry:
        registry.write(
            "#pragma once\n"
            "{}\n"
            "// clang-format off\n"
            "#include <array>\n"
            "#include <utility>\n"
            "#include <string_view>\n"
            "\n"
            "namespace redfish\n"
            "{{\n"
            "\n"
            "struct Path{{\n"
            "    std::string_view path;\n"
            "    std::string_view type;\n"
            "    bool isTop;\n"
            "}};\n\n"
            "constexpr std::array<Path, {}> topCollections".format(
                WARNING, len(uri_to_entity_map)
            )
        )
        registry.write("{{\n")
        for uri, typename in sorted(uri_to_entity_map.items()):
            is_top = str(uri in top_collections).lower()
            registry.write("  {")
            registry.write(f'"{uri}", "{typename}", {is_top}')
            registry.write("},\n")
        registry.write("}};\n")

        registry.write("\n} // namespace redfish\n")


def generate_registries():
    top_collections = set()
    uri_to_entity_map = {}

    # Begin parsing from the Service Root
    curr_path = ""

    # ServiceRoot_v1.xml also shows "/redfish/v1/" as an example
    seen_paths.add(curr_path + "/")
    parse_node(
        "ServiceRoot",
        curr_path,
        top_collections,
        uri_to_entity_map,
        False,
        "ServiceRoot_v1.xml",
    )

    print("Finished traversal!")

    generate_top_collections(uri_to_entity_map, top_collections)
