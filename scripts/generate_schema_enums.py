#!/usr/bin/python3
import os
import xml.etree.ElementTree as ET
import pprint
from urllib.parse import urlparse
from enum import Enum
from collections import defaultdict
from multiprocessing import Pool, cpu_count
import shutil
import subprocess
import re

multithread = True

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
REDFISH_SCHEMA_DIR = os.path.realpath(
    os.path.join(SCRIPT_DIR, "..", "static", "redfish", "v1", "schema")
)
OUTFOLDER = os.path.realpath(
    os.path.join(SCRIPT_DIR, "..", "redfish-core", "include", "redfish_defs")
)

# The current mechanisms of generation have problems with circular imports.
# Will likely need a way in the future to set an explicit max depth
circular_imports = [
    "SubProcessors",
    "AllocatedPools",
    "CapacitySources",
    "StorageGroups",
    "Steps",
    "MetricReportDefinition",
    "SubTasks",
    "DataProtectionLinesOfService",
]


class BaseType(Enum):
    STRING = 1
    BOOLEAN = 2
    DURATION = 3
    TIME = 4
    FLOAT = 5
    INT64 = 6
    INT32 = 7
    DECIMAL = 8
    GUID = 9


def basetype_from_edm(edm_type):
    if edm_type == "Edm.String":
        return BaseType.STRING
    if edm_type == "Edm.Boolean":
        return BaseType.BOOLEAN
    if edm_type == "Edm.Decimal":
        return BaseType.DECIMAL
    if edm_type == "Edm.Int64":
        return BaseType.INT64
    if edm_type == "Edm.Int32":
        return BaseType.INT32
    if edm_type == "Edm.DateTimeOffset":
        return BaseType.TIME
    if edm_type == "Edm.Duration":
        return BaseType.DURATION
    if edm_type == "Edm.Guid":
        return BaseType.GUID
    return None


class EntityType:
    def __init__(
        self, name, basetype, basetype_flat, namespace, abstract, from_file
    ):
        self.name = name
        self.properties = []
        self.basetype = basetype
        self.basetype_flat = basetype_flat
        self.namespace = namespace
        self.abstract = abstract
        self.from_file = from_file


class Enum:
    def __init__(self, name, values, namespace, from_file):
        self.name = name
        self.values = values
        self.namespace = namespace
        self.from_file = from_file


class Complex:
    def __init__(self, name, namespace, from_file):
        self.name = name
        self.namespace = namespace
        self.from_file = from_file


class TypeDef:
    def __init__(self, name, basetype, namespace, from_file):
        self.name = name
        self.basetype = basetype
        self.namespace = namespace
        self.from_file = from_file


class Collection:
    def __init__(self, name, contained_type, from_file):
        self.name = name
        self.contained_type = contained_type
        self.from_file = from_file


class PropertyPermissions(Enum):
    READ_ONLY = 0
    READ_WRITE = 1


class Property:
    def __init__(
        self,
        name,
        thistype,
        permissions,
        description,
        long_description,
        from_file,
    ):
        self.name = name
        self.type = thistype
        self.permissions = permissions
        self.description = description
        self.long_description = long_description
        self.from_file = from_file


class NavigationProperty:
    def __init__(
        self,
        name,
        thistype,
        permissions,
        auto_expand,
        expand_references,
        description,
        long_description,
        from_file,
        contains_target,
    ):
        self.name = name
        self.type = thistype
        self.permissions = permissions
        self.auto_expand = auto_expand
        self.expand_references = expand_references
        self.description = description
        self.long_description = long_description
        self.from_file = from_file
        self.contains_target = contains_target


def find_element_in_scope(element_name, references, this_file):
    if element_name.startswith("Collection(") and element_name.endswith(")"):
        contained_element = find_element_in_scope(
            element_name[11:-1], references, this_file
        )
        return Collection(element_name, contained_element, this_file)

    edmtype = basetype_from_edm(element_name)
    if edmtype is not None:
        return edmtype

    for reference_uri, namespaces in references:
        uri = urlparse(reference_uri)

        filepath = os.path.join(REDFISH_SCHEMA_DIR, os.path.basename(uri.path))
        if not os.path.exists(filepath):
            continue

        elements = parse_file(filepath, namespaces, element_name)

        if len(elements) == 0:
            continue
        if len(elements) > 1:
            print(
                "Found {} {} elements with referencelist {}",
                len(elements),
                element_name,
                pprint.PrettyPrinter(indent=4).pprint(references),
            )
            continue
        return elements[0]

    # finish by searching the file we're in now
    elements = parse_file(this_file, namespaces, element_name)
    if len(elements) != 1:
        return None
    return elements[0]

    # print("Unable to find {}".format(element_name))
    return None


xml_cache = {}


def parse_file(filename, namespaces_to_check=[], element_name_filter=None):

    root = xml_cache.get(filename, None)
    if root is None:
        tree = ET.parse(filename)
        root = tree.getroot()
        xml_cache[filename] = root

    # list of references and namespaces
    references = []

    EntityTypes = []

    for reference in root.findall(
        "{http://docs.oasis-open.org/odata/ns/edmx}Reference"
    ):
        uri = reference.attrib["Uri"]
        namespaces = []
        for include in root.findall(
            "{http://docs.oasis-open.org/odata/ns/edmx}Include"
        ):
            ns = include.attrib["Namespace"]
            alias = include.attrib.get("Alias", ns)
            namespaces.append((ns, alias))
        references.append((uri, namespaces))

    data_services = root.findall(
        "{http://docs.oasis-open.org/odata/ns/edmx}DataServices"
    )
    for ds in data_services:
        for element in ds:
            if (
                element.tag
                == "{http://docs.oasis-open.org/odata/ns/edm}Schema"
            ):
                namespace = element.attrib["Namespace"]
                if (
                    len(namespaces_to_check) == 0
                    or namespace in namespaces_to_check
                ):
                    for schema_element in element:
                        name = schema_element.attrib.get("Name", None)
                        if name != None:
                            scoped_name = namespace + "." + name
                        else:
                            scoped_name = ""

                        # TODO(ed) It would be better if name, and scopename were
                        # combined so this was one search instead of two
                        if (
                            element_name_filter is not None
                            and name != element_name_filter
                            and scoped_name != element_name_filter
                        ):
                            continue

                        if (
                            schema_element.tag
                            == "{http://docs.oasis-open.org/odata/ns/edm}EntityType"
                        ):
                            basetypename = schema_element.attrib.get(
                                "BaseType", None
                            )
                            abstract = (
                                schema_element.attrib.get("Abstract", "false")
                                == "true"
                            )
                            if basetypename is not None:
                                basetype = find_element_in_scope(
                                    basetypename, references, filename
                                )
                                if basetype is None:
                                    print(
                                        "Unable to find basetype {}".format(
                                            basetypename
                                        )
                                    )
                            else:
                                basetype = None
                            basetype_flat = []
                            if basetype is not None:
                                basetype_flat.append(basetype)
                                if isinstance(basetype, EntityType):
                                    basetype_flat.extend(
                                        basetype.basetype_flat
                                    )
                            entity = EntityType(
                                name,
                                basetype,
                                basetype_flat,
                                namespace,
                                abstract,
                                filename,
                            )

                            for property_element in schema_element:
                                permission = PropertyPermissions.READ_WRITE
                                description = ""
                                long_description = ""
                                if (
                                    property_element.tag
                                    == "{http://docs.oasis-open.org/odata/ns/edm}Property"
                                ):
                                    prop_type = property_element.attrib["Type"]
                                    property_entity = find_element_in_scope(
                                        prop_type, references, filename
                                    )
                                    # if property_entity is None:
                                    #    print(
                                    #        "Unable to find type for {}".format(
                                    #            prop_type
                                    #        )
                                    #    )
                                    for child in property_element:
                                        if (
                                            child.tag
                                            == "{http://docs.oasis-open.org/odata/ns/edm}Annotation"
                                        ):
                                            term = child.attrib.get("Term", "")
                                            if term == "OData.Permissions":
                                                perm = child.attrib.get(
                                                    "EnumMember", ""
                                                )
                                                if (
                                                    perm
                                                    == "OData.Permission/Read"
                                                ):
                                                    permission = (
                                                        PropertyPermissions.READ_ONLY
                                                    )
                                            elif term == "OData.Description":
                                                description = child.attrib.get(
                                                    "String", ""
                                                )
                                            elif (
                                                term == "OData.LongDescription"
                                            ):
                                                long_description = (
                                                    child.attrib.get(
                                                        "String", ""
                                                    )
                                                )
                                    # TODO(ed) subprocessor has a circular import
                                    if (
                                        property_element.attrib["Name"]
                                        in circular_imports
                                    ):
                                        pass
                                    else:
                                        entity.properties.append(
                                            Property(
                                                property_element.attrib[
                                                    "Name"
                                                ],
                                                property_entity,
                                                permission,
                                                description,
                                                long_description,
                                                filename,
                                            )
                                        )
                                elif (
                                    property_element.tag
                                    == "{http://docs.oasis-open.org/odata/ns/edm}NavigationProperty"
                                ):
                                    expand_references = False
                                    auto_expand = False
                                    prop_type = property_element.attrib["Type"]
                                    property_entity = find_element_in_scope(
                                        prop_type, references, filename
                                    )
                                    contains_target = (
                                        property_element.attrib.get(
                                            "ContainsTarget", "false"
                                        )
                                        == "true"
                                    )
                                    if property_entity is None:
                                        print(
                                            "Unable to find type for {}".format(
                                                prop_type
                                            )
                                        )
                                    for child in property_element:
                                        term = child.attrib.get("Term", "")
                                        if (
                                            term
                                            == "OData.AutoExpandReferences"
                                        ):
                                            expand_references = True
                                        elif term == "OData.AutoExpand":
                                            auto_expand = True
                                        elif term == "OData.Permissions":
                                            perm = child.attrib.get(
                                                "EnumMember", ""
                                            )
                                            if perm == "OData.Permission/Read":
                                                permission = (
                                                    PropertyPermissions.READ_ONLY
                                                )
                                        elif term == "OData.Description":
                                            description = child.attrib.get(
                                                "String", ""
                                            )
                                        elif term == "OData.LongDescription":
                                            long_description = (
                                                child.attrib.get("String", "")
                                            )
                                    # TODO(ed) subprocessor has a circular import
                                    if (
                                        property_element.attrib["Name"]
                                        in circular_imports
                                    ):
                                        pass
                                    else:
                                        entity.properties.append(
                                            NavigationProperty(
                                                property_element.attrib[
                                                    "Name"
                                                ],
                                                property_entity,
                                                permission,
                                                auto_expand,
                                                expand_references,
                                                description,
                                                long_description,
                                                filename,
                                                contains_target,
                                            )
                                        )

                            EntityTypes.append(entity)

                            # print("{} {}".format(namespace, name))
                        if (
                            schema_element.tag
                            == "{http://docs.oasis-open.org/odata/ns/edm}EnumType"
                        ):
                            enums = []
                            for member in schema_element.findall(
                                "{http://docs.oasis-open.org/odata/ns/edm}Member"
                            ):
                                enums.append(member.attrib["Name"])

                            EntityTypes.append(
                                Enum(name, enums, namespace, filename)
                            )
                        if (
                            schema_element.tag
                            == "{http://docs.oasis-open.org/odata/ns/edm}ComplexType"
                        ):
                            EntityTypes.append(
                                Complex(name, namespace, filename)
                            )

                        if (
                            schema_element.tag
                            == "{http://docs.oasis-open.org/odata/ns/edm}TypeDefinition"
                        ):
                            underlying_type = schema_element.attrib[
                                "UnderlyingType"
                            ]

                            typedef_entity = find_element_in_scope(
                                underlying_type, references, filename
                            )
                            EntityTypes.append(
                                TypeDef(
                                    name, typedef_entity, namespace, filename
                                )
                            )

    return EntityTypes


# parse file is recursive, and we only want to print "parsing" once
def parse_toplevel(filepath):
    print("Parsing {}".format(filepath))
    return parse_file(filepath)


# TODO(ed) this shouldn't be a global
folders_added_to_grpc = []


def get_lowest_type(this_class, depth=0):
    if not isinstance(this_class, EntityType):
        return this_class
    if this_class.abstract:
        return this_class
    if this_class.basetype is None:
        return this_class
    return get_lowest_type(this_class.basetype, depth + 1)


def find_type_for_abstract(class_list, abs):
    for element in class_list:
        lt = get_lowest_type(element)
        if lt.name == abs.name and lt.from_file == abs.from_file:
            return element
    return abs


def camel_to_snake(name):
    # snake casing PCIeDevice and PCIeFunction results in mediocre results
    # given that the standard didn't camel case those "correctly", so change
    # the casing explicitly to generate sane snake case results.
    name = name.replace("PCIe", "Pcie")
    name = re.sub("(.)([A-Z][a-z]+)", r"\1_\2", name)
    return re.sub("([a-z0-9])([A-Z])", r"\1_\2", name).lower()


def generate_enums(flat_list):
    if os.path.exists(OUTFOLDER):
        shutil.rmtree(OUTFOLDER)
    os.makedirs(OUTFOLDER)
    enum_list = [element for element in flat_list if isinstance(element, Enum)]
    enum_list.sort(key=lambda x: x.namespace + x.name)
    enum_by_namespace = defaultdict(list)
    for enum in enum_list:
        namespace_split = enum.namespace.split(".")[0]
        enum_by_namespace[namespace_split].append(enum)

    for namespace, enum_list in enum_by_namespace.items():
        snake_case_namespace = camel_to_snake(namespace)
        outfile = os.path.join(
            OUTFOLDER, "{}.hpp".format(snake_case_namespace)
        )

        with open(outfile, "w") as redfish_defs:

            redfish_defs.write(
                "#pragma once\n"
                "#include <nlohmann/json.hpp>\n\n"
                "namespace {}\n"
                "{{\n"
                "// clang-format off\n\n".format(snake_case_namespace)
            )
            for element in enum_list:
                enum_name = element.name

                redfish_defs.write("enum class {}{{\n".format(enum_name))
                if not "Invalid" in element.values:
                    values = ["Invalid"] + element.values

                for value in values:
                    redfish_defs.write("    {},\n".format(value))

                redfish_defs.write("};\n\n")

                redfish_defs.write(
                    "NLOHMANN_JSON_SERIALIZE_ENUM({}, {{\n".format(enum_name)
                )
                for value in values:
                    redfish_defs.write(
                        '    {{{}::{}, "{}"}},\n'.format(
                            enum_name, value, value
                        )
                    )

                redfish_defs.write("});\n\n")

                print(element.name)

            redfish_defs.write("}\n" "// clang-format on\n")


def main():
    flat_list = []

    print("Reading from {}".format(REDFISH_SCHEMA_DIR))
    dir_list = os.listdir(REDFISH_SCHEMA_DIR)

    filepaths = [
        os.path.join(REDFISH_SCHEMA_DIR, filename) for filename in dir_list
    ]
    if multithread:
        with Pool(int(cpu_count() / 2)) as p:
            out = p.map(parse_toplevel, filepaths)

            flat_list.extend([item for sublist in out for item in sublist])
    else:

        for filepath in filepaths:
            out = parse_toplevel(filepath)
            flat_list.extend(out)

    print("Parsing done")

    flat_list.sort(key=lambda x: x.name.lower())
    flat_list.sort(key=lambda x: not x.name.startswith("ServiceRoot"))

    generate_enums(flat_list)


if __name__ == "__main__":
    main()
