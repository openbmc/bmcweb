"""TODO(shounak): DO NOT SUBMIT without one-line documentation for schemas.

TODO(shounak): DO NOT SUBMIT without a detailed description of schemas.
"""
import os
import xml.etree.ElementTree as ET
import pprint
import requests
from urllib.parse import urlparse
import enum
import dataclasses
from typing import List, Mapping, Optional, Sequence, Set, Tuple, Union
import functools
import multiprocessing


SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
STATIC_DIR = os.path.realpath(os.path.join(SCRIPT_DIR, '..', 'static'))
REDFISH_DIR = os.path.join(STATIC_DIR, 'redfish', 'v1')
REDFISH_SCHEMA_DIR = os.path.join(REDFISH_DIR, 'schema')

circular_imports = ["SubProcessors", "AllocatedPools", "CapacitySources",
                    "StorageGroups", "Steps", "MetricReportDefinition", "SubTasks", "DataProtectionLinesOfService"]


class BaseType(enum.Enum):
    STRING = 1
    BOOLEAN = 2
    DURATION = 3
    TIME = 4
    FLOAT = 5
    INT64 = 6
    INT32 = 7
    DECIMAL = 8
    GUID = 9




BASETYPE_FROM_EDM_TYPE = {
    "Edm.String": BaseType.STRING,
    "Edm.Boolean": BaseType.BOOLEAN,
    "Edm.Decimal": BaseType.DECIMAL,
    "Edm.Int64": BaseType.INT64,
    "Edm.Int32": BaseType.INT32,
    "Edm.DateTimeOffset": BaseType.TIME,
    "Edm.Duration": BaseType.DURATION,
    "Edm.Guid": BaseType.GUID,
    "Edm.PropertyPath": BaseType.STRING,
    "Edm.NavigationPropertyPath": BaseType.STRING,
    "Edm.PrimitiveType": BaseType.STRING, #TODO is it though
}


@dataclasses.dataclass
class EntityBase:
    name: str
    namespace: str
    from_file: str


@dataclasses.dataclass
class EntityType(EntityBase):
    basetype: Optional["Type"]
    abstract: bool
    properties: List['Property'] = dataclasses.field(default_factory=list)

    @functools.cached_property
    def basetype_flat(self) -> Sequence["Type"]:
        if isinstance(self.basetype, EntityType):
            return [self.basetype] + self.basetype.basetype_flat
        elif self.basetype is not None:
            return [self.basetype]
        return []


@dataclasses.dataclass
class Enum(EntityBase):
    values: Sequence[str]


@dataclasses.dataclass
class Complex(EntityBase):
    basetype: Optional["Type"]
    abstract: bool
    properties: List['Property'] = dataclasses.field(default_factory=list)

    @functools.cached_property
    def basetype_flat(self) -> Sequence["Type"]:
        if isinstance(self.basetype, EntityType):
            return [self.basetype] + self.basetype.basetype_flat
        elif self.basetype is not None:
            return [self.basetype]
        return []



@dataclasses.dataclass
class TypeDef(EntityBase):
    basetype: "Type"


@dataclasses.dataclass
class Collection:
    contained_type: "Type"
    from_file: str

Type = Union[BaseType, EntityBase, Collection]

class PropertyPermissions(enum.Enum):
    READ_ONLY = 0
    READ_WRITE = 1


@dataclasses.dataclass
class Property:
    name: str
    type: Type
    permissions: PropertyPermissions
    description: str
    long_description: str
    from_file: str


@dataclasses.dataclass
class NavigationProperty(Property):
    auto_expand: bool
    expand_references: bool
    contains_target: bool


Namespaces = Mapping[str, str]
References = Sequence[Tuple[str, Namespaces]]

class SchemaError(Exception):
    pass

def find_element_in_scope(element_name: str, references: References, this_file: str) -> Optional[Type]:
    if element_name.startswith("Collection(") and element_name.endswith(")"):
        containted_type_name = element_name[11: -1]
        contained_element = find_element_in_scope(
            containted_type_name, references, this_file)
        if contained_element is None:
            raise SchemaError(f"Could not find contained type '{containted_type_name}'")
        return Collection(contained_element, this_file)

    edmtype = BASETYPE_FROM_EDM_TYPE.get(element_name)
    if edmtype is not None:
        return edmtype

    for reference_uri, namespaces in references:
        uri = urlparse(reference_uri)

        filepath = os.path.join(REDFISH_SCHEMA_DIR, os.path.basename(uri.path))
        if not os.path.exists(filepath):
            print(f"File doesn't exist, downloading from {reference_uri}")
            r = requests.get(reference_uri)
            r.raise_for_status()
            with open(filepath, "wb") as downloaded:
                downloaded.write(r.content)

        elements = parse_file(filepath, namespaces, element_name)

        if len(elements) == 0:
            continue
        if len(elements) > 1:
            print(f"Found {len(elements)} {element_name} elements with referencelist {pprint.PrettyPrinter(indent=4).pformat(references)}")
            continue
        return elements[0]

    # finish by searching the file we're in now
    elements = parse_file(this_file, namespaces, element_name)
    if len(elements) != 1:
        print(f"Unable to find {element_name}")
        return None
    return elements[0]

@functools.lru_cache(maxsize=None)
def get_root_element(filename: str) -> ET.Element:
    tree = ET.parse(filename)
    return tree.getroot()

XMLNS = {
    "edmx": "http://docs.oasis-open.org/odata/ns/edmx",
    "edm": "http://docs.oasis-open.org/odata/ns/edm",
}

def parse_file(filename: str, namespaces_to_check: Namespaces = {}, element_name_filter: Optional[str] = None) -> Sequence[EntityBase]:
    root = get_root_element(filename)
    localfilename = filename.split("/")[-1].split(".")[0]
    # list of references and namespaces
    references = []
    entity_types: List[EntityBase] = []

    for reference in root.findall("edmx:Reference", XMLNS):
        uri = reference.attrib["Uri"]
        namespaces = {}
        for include in root.findall("edmx:Include", XMLNS):
            ns = include.attrib["Namespace"]
            alias = include.attrib.get("Alias", ns)
            namespaces[alias] = ns
            namespaces[ns] = ns
        references.append((uri, namespaces))

    for element in root.findall("edmx:DataServices/edm:Schema", XMLNS):
        namespace = element.attrib["Namespace"]
        aliasNamespace = element.get("Alias",None)
        if len(namespaces_to_check) == 0 or namespace in namespaces_to_check:
            for schema_element in element:
                name = schema_element.attrib.get("Name")
                names: Set[str] = set()
                if name is None:
                    continue
                names = {name, f"{namespace}.{name}"}
                if aliasNamespace:
                     names.add(f"{aliasNamespace}.{name}")

                if namespaces_to_check:
                    # Unaliased namespace
                    names.add(f"{namespaces_to_check[namespace]}.{name}")

                if element_name_filter is not None and element_name_filter not in names:
                    continue

                if schema_element.tag == "{http://docs.oasis-open.org/odata/ns/edm}EntityType":
                    basetypename = schema_element.attrib.get(
                        "BaseType", None)
                    abstract = schema_element.attrib.get(
                        "Abstract", "false") == "true"
                    if basetypename is not None:
                        basetype = find_element_in_scope(
                            basetypename, references, filename)
                        if basetype is None:
                            print(f"Unable to find basetype {basetypename}")
                    else:
                        basetype = None
                    entity = EntityType(name, namespace, filename, basetype, abstract)

                    for property_element in schema_element:
                        permission = PropertyPermissions.READ_WRITE
                        description = ""
                        long_description = ""
                        if property_element.tag == "{http://docs.oasis-open.org/odata/ns/edm}Property":
                            prop_type = property_element.attrib["Type"]
                            property_entity = find_element_in_scope(
                                prop_type, references, filename)
                            if property_entity is None:
                                raise SchemaError(
                                    f"Unable to find type for '{prop_type}'")
                            for child in property_element.findall("edm:Annotation", XMLNS):
                                term = child.attrib.get("Term", "")
                                if term == "OData.Permissions":
                                    perm = child.attrib.get(
                                        "EnumMember", "")
                                    if perm == "OData.Permission/Read":
                                        permission = PropertyPermissions.READ_ONLY
                                elif term == "OData.Description":
                                    description = child.attrib.get(
                                        "String", "")
                                elif term == "OData.LongDescription":
                                    long_description = child.attrib.get(
                                        "String", "")
                            # TODO(ed) subprocessor has a circular import
                            if property_element.attrib["Name"] in circular_imports:
                                pass
                            else:
                                entity.properties.append(
                                    Property(property_element.attrib["Name"], property_entity, permission, description, long_description, filename))
                                #print("stress 5")
                                #print(property_element.attrib["Name"])
                                #print(property_entity)
                        elif property_element.tag == "{http://docs.oasis-open.org/odata/ns/edm}NavigationProperty":
                            expand_references = False
                            auto_expand = False
                            prop_type = property_element.attrib["Type"]
                            property_entity = find_element_in_scope(
                                prop_type, references, filename)
                            contains_target = property_element.attrib.get(
                                "ContainsTarget", "false") == "true"
                            if property_entity is None:
                                raise SchemaError(
                                    f"Unable to find type for {prop_type}")
                            for child in property_element:
                                term = child.attrib.get("Term", "")
                                if term == "OData.AutoExpandReferences":
                                    expand_references = True
                                elif term == "OData.AutoExpand":
                                    auto_expand = True
                                elif term == "OData.Permissions":
                                    perm = child.attrib.get(
                                        "EnumMember", "")
                                    if perm == "OData.Permission/Read":
                                        permission = PropertyPermissions.READ_ONLY
                                elif term == "OData.Description":
                                    description = child.attrib.get(
                                        "String", "")
                                elif term == "OData.LongDescription":
                                    long_description = child.attrib.get(
                                        "String", "")
                            # TODO(ed) subprocessor has a circular import
                            if property_element.attrib["Name"] in circular_imports:
                                pass
                            else:
                                entity.properties.append(
                                    NavigationProperty(property_element.attrib["Name"], property_entity, permission, description, long_description,
                                                        filename, auto_expand, expand_references, contains_target))

                    entity_types.append(entity)
                if schema_element.tag == "{http://docs.oasis-open.org/odata/ns/edm}EnumType":
                    enums = []
                    for member in schema_element.findall("edm:Member", XMLNS):
                        enums.append(member.attrib["Name"])

                    entity_types.append(
                        Enum(name, namespace, filename, enums))
                if schema_element.tag == "{http://docs.oasis-open.org/odata/ns/edm}ComplexType":
                    basetypename = schema_element.attrib.get(
                        "BaseType", None)
                    # basetypename can be None
                    abstract = schema_element.attrib.get(
                        "Abstract", "false") == "true"
                    # all complex types are abstract
                    if basetypename is not None:
                        basetype = find_element_in_scope(
                            basetypename, references, filename)
                        if basetype is None:
                            print(f"Unable to find basetype for complex type {basetypename}")
                    else:
                         basetype = None
                    # keep the entity similar to entitytype
                    entity = Complex(name, namespace, filename, basetype, abstract)

                    for property_element in schema_element:
                        permission = PropertyPermissions.READ_WRITE
                        description = ""
                        long_description = ""
                        if property_element.tag == "{http://docs.oasis-open.org/odata/ns/edm}Property":
                            prop_type = property_element.attrib["Type"]
                            property_entity = find_element_in_scope(prop_type, references, filename)
                            if property_entity is None:
                                raise SchemaError(f"Unable to find type for '{prop_type}'")
                                #property_entity = None
                            for child in property_element.findall("edm:Annotation", XMLNS):
                                term = child.attrib.get("Term", "")
                                if term == "OData.Permissions":
                                    perm = child.attrib.get(
                                        "EnumMember", "")
                                    if perm == "OData.Permission/Read":
                                        permission = PropertyPermissions.READ_ONLY
                                elif term == "OData.Description":
                                    description = child.attrib.get(
                                        "String", "")
                                elif term == "OData.LongDescription":
                                    long_description = child.attrib.get(
                                        "String", "")
                            #if property_element.tag == "{http://docs.oasis-open.org/odata/ns/edm}Property":
                            #    prop_type = property_element.attrib["Type"]
                            if property_element.get("Name") in circular_imports:
                                pass
                            else:
                                #property_entity = None
                                entity.properties.append(
                                    Property(property_element.attrib["Name"], property_entity, permission, description, long_description, filename))
                    #todo fix entity_types.append(entity)a

                        elif property_element.tag == "{http://docs.oasis-open.org/odata/ns/edm}NavigationProperty":
                            expand_references = False
                            auto_expand = False
                            prop_type = property_element.attrib["Type"]
                            if str(namespace) == "Resource" or str(namespace) == "StorageGroup.v1_0_0" or str(namespace) == "Drive.v1_0_0"\
                                or str(namespace) == "SpareResourceSet.v1_0_0": #and str(property_element.attrib["Name"]) == "OriginOfCondition":
                                property_entity = "JEBR TO FIX"
                                #print("call skipped")
                            else:
                                property_entity = find_element_in_scope(
                                    prop_type, references, filename)

                            contains_target = property_element.attrib.get(
                                "ContainsTarget", "false") == "true"
                            if property_entity is None:
                                raise SchemaError(
                                    f"Unable to find type for {prop_type}")
                            for child in property_element:
                                term = child.attrib.get("Term", "")
                                if term == "OData.AutoExpandReferences":
                                    expand_references = True
                                elif term == "OData.AutoExpand":
                                    auto_expand = True
                                elif term == "OData.Permissions":
                                    perm = child.attrib.get(
                                        "EnumMember", "")
                                    if perm == "OData.Permission/Read":
                                        permission = PropertyPermissions.READ_ONLY
                                elif term == "OData.Description":
                                    description = child.attrib.get(
                                        "String", "")
                                elif term == "OData.LongDescription":
                                    long_description = child.attrib.get(
                                        "String", "")
                            # TODO(ed) subprocessor has a circular import
                            if property_element.attrib["Name"] in circular_imports:
                                pass
                            else:
                                entity.properties.append(
                                    NavigationProperty(property_element.attrib["Name"], property_entity, permission, description, long_description,
                                                        filename, auto_expand, expand_references, contains_target))

                    entity_types.append(entity)

                if schema_element.tag == "{http://docs.oasis-open.org/odata/ns/edm}TypeDefinition":
                    underlying_type = schema_element.attrib["UnderlyingType"]

                    typedef_entity = find_element_in_scope(
                        underlying_type, references, filename)
                    if typedef_entity is None:
                        raise SchemaError(f"Underlying type not found: '{underlying_type}'")
                    entity_types.append(
                        TypeDef(name, namespace, filename, typedef_entity))

    return entity_types

def get_lowest_type(this_class: Type, depth: int = 0) -> Type:
    if not isinstance(this_class, EntityType):
        return this_class
    if this_class.abstract:
        return this_class
    if this_class.basetype is None:
        return this_class
    return get_lowest_type(this_class.basetype, depth+1)

def find_type_for_abstract(class_list: Sequence[Type], abs: Type) -> Type:
    for element in class_list:
        lt = get_lowest_type(element)
        if abs == lt or (isinstance(abs, EntityBase) and isinstance(lt, EntityBase) and lt.name == abs.name and lt.from_file == abs.from_file):
            return element
    '''
    for element in class_list:
        lt = get_lowest_type(element)
        if lt.name == abs.name:
            return element
    '''
    return abs

def instantiate_abstract_classes(class_list: Sequence[Type], this_class: Type):
    if isinstance(this_class, EntityType):
        for class_to_fix in [this_class] + this_class.basetype_flat:
            if isinstance(class_to_fix, EntityType):
                for property_instance in class_to_fix.properties:
                    property_instance.type = find_type_for_abstract(class_list, property_instance.type)
        for class_to_fix in [this_class] + this_class.basetype_flat:
            if isinstance(class_to_fix, EntityType):
                for property_instance in class_to_fix.properties:
                    instantiate_abstract_classes(class_list, property_instance.type)

            if isinstance(class_to_fix, Collection):
                class_to_fix.contained_type = find_type_for_abstract(class_list, class_to_fix.contained_type)
    if isinstance(this_class, Collection):
        this_class.contained_type = find_type_for_abstract(class_list, this_class.contained_type)

def remove_old_schemas(flat_list: Sequence[EntityBase]) -> Sequence[EntityBase]:
    # remove all but the last schema version for a type by loading them into a
    # dict with Namespace + name
    elements = {}
    for item in flat_list:
        elements[item.namespace.split(".")[0] + item.name] = item
    flat_list = [elements[item] for item in elements]
    return flat_list

def parse_toplevel(filepath: str) -> Sequence[EntityBase]:
    print(f"Parsing {filepath}")
    return parse_file(filepath)

def load_schemas(multithread: bool = True) -> Sequence[EntityBase]:
    flat_list = []

    for root, _, files in os.walk(REDFISH_SCHEMA_DIR):
        # Todo(ed) Oem account service is totally wrong odata wise, and
        # its naming conflicts with the "real" account service
        filepaths = [os.path.join(
            root, filename) for filename in files if not filename.startswith("OemAccountService")]
        if multithread:
            with multiprocessing.Pool(int(multiprocessing.cpu_count() / 2)) as p:
                out = p.map(parse_toplevel, filepaths)

                flat_list.extend(
                    [item for sublist in out for item in sublist])
        else:

            for filepath in filepaths:
                out = parse_toplevel(filepath)
                flat_list.extend(out)

    flat_list = remove_old_schemas(flat_list)

    flat_list.sort(key=lambda x: x.name.lower())
    flat_list.sort(key=lambda x: not x.name.startswith("ServiceRoot"))
    for element in flat_list:
        instantiate_abstract_classes(flat_list, element)
    return flat_list
