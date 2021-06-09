import schemas as ed
import os
SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
STATIC_DIR = os.path.realpath(os.path.join(SCRIPT_DIR, '..', 'static'))
REDFISH_DIR = os.path.join(STATIC_DIR, 'redfish', 'v1')
REDFISH_SCHEMA_DIR = os.path.join(REDFISH_DIR, 'schema')
DATAMODEL_DIR = os.path.join(STATIC_DIR, "dataModel")
DEFAULT_TYPES = ["int8_t", "int16_t", "int32_t", "int64_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t", "std::chrono::time_point<std::chrono::system_clock>"]

def get_filename_from_entity(entity):
    if hasattr(entity, 'from_file'):
        return  os.path.splitext(os.path.basename(entity.from_file))[0]
    else:
        return "nothing"

def lowerCase(name):
    if not name.isupper():
        return name[:1].lower() + name[1:]
    else:
        return name

class DMfile:
    def __init__(self, includes, enums, structs):
        self.includes = includes #list
        self.enums = enums # list
        self.structs = structs # dict of structs, type is key

    def combine(self, otherFile):
        if self.includes:
            self.includes.extend(otherFile.includes)
        else: self.includes = otherFile.includes

        if self.enums:
            self.enums.extend(otherFile.enums)
        else: self.enums = otherFile.enums

        #if self.structs:
        #    self.structs.extend(otherFile.structs)
        #else: self.structs = otherFile.structs
        if self.structs and otherFile.structs:
              self.structs = {**self.structs, **otherFile.structs}

        elif otherFile.structs: self.structs = otherFile.structs

        return self

    def printFile(self, filepath):
        #include guards
        name = filepath.split("/")[-1].split(".")[0]
        file_out = "#ifndef {}\n".format(name.upper())
        file_out += "#define {}\n\n".format(name.upper())

        # sort and de duplicate before print
        self.includes = sorted(set(self.includes), key=str.casefold)
        for include in self.includes:
            if include.startswith("<"):
                file_out += "#include {}\n".format(include)
            else:
                file_out += "#include \"{}.h\"\n".format(include)
        file_out += "\n"

        for enum in self.enums:
            file_out += enum.printString()

        # this struct need to be printed in a very specific order
        # Either this struct can be printed, or we should its unmet dependencies
        # reversely
        if name == "Resource_v1":
          print(self.structs)
        for struct in self.structs:
            if struct == "Resource_v1_Oem":
                print("resource_v1_oem found?")
                print(self.structs[struct])
            file_out += self.writeStruct(self.structs[struct])

        file_out+= "#endif\n"

        with open(filepath, 'w') as datamodel_file:
            datamodel_file.write(file_out)

    def writeStruct(self, struct):
        if not struct.written:
            listOfDepName = struct.dependency()
            struct_out = ""
            for depName in listOfDepName:
                dep = self.structs.get(depName)
                #TODO, if it is an enum, ignore it
                if dep:
                   struct_out += self.writeStruct(dep)
            struct_out += struct.printString()
            struct.written = True
            return struct_out
        return ""



class Struct:
    def __init__(self, name, properties):
        self.name = name
        self.properties = properties # list of tuple
        self.written = False

    def clangCompatibleName(self, name):
      if name in DEFAULT_TYPES:
          return name
      parts = name.split("_")
      # skip the first letter
      newName = parts[0]
      for i in range(1,len(parts)):
        thisPart = parts[i]
        if len(thisPart) > 0:
            newName += str( thisPart[0].upper() + thisPart[1:])
        else:
             newName += thisPart
      return newName

    # returns a list of children that must be written first
    def dependency(self):
        thisPrefix = self.name.split("_")[0]
        dependsList = []
        for _type , _ , _ in self.properties:
            if _type.startswith(thisPrefix):
                dependsList.append(_type)
        return dependsList


    def printString(self):
        clangName = self.clangCompatibleName(self.name)
        if self.name == "Resource_v1_Oem":
            print("in print for resource_v1_Oem???")
            print(clangName)
        text = "struct {}\n".format(clangName)+"{\n"
        for _type, _id , used in self.properties:
            if used:
              text += "    {} {};\n".format( self.clangCompatibleName(_type), _id)
              #text += "    {} {};\n".format(_type, _id)
            else:
                text += "//  {} {};\n".format( _type, _id)
        text +="};\n"
        if self.name == "Resource_v1_Oem":
            print(" I don't get it")
            print(text)

        return text
    

    def commentOut(self, unusedId):
        for _type, _id, used in properites:
            if _id == unusedId:
                used = False

class Enum:
    def __init__(self, name, enumerators):
        self.name = name
        self.enumerators = enumerators

    def clangCompatibleName(self, name):
      if name in DEFAULT_TYPES:
          return name
      parts = name.split("_")
      # skip the first letter
      newName = parts[0]
      for i in range(1,len(parts)):
        thisPart = parts[i]
        if len(thisPart) > 0:
            newName += str(thisPart[0].upper() + thisPart[1:])
        else:
             newName += thisPart
      return newName

    def printString(self):
        text = "enum class {} {{\n".format(self.clangCompatibleName(self.name))
        for val, name in enumerate(self.enumerators):
            text += "    {},\n".format(name, val)
        text += "};\n"
        return text


#almost the same as grpc, but the time types were different
def basetype_to_datamodel(basetype):
    if basetype == ed.BaseType.STRING:
        return "std::string", []
    if basetype == ed.BaseType.BOOLEAN:
        return "bool", []
    if basetype == ed.BaseType.DECIMAL:
        return "double", []
    if basetype == ed.BaseType.INT64:
        return "int64_t", []
    if basetype == ed.BaseType.INT32:
        return "int32_t", []
    if basetype == ed.BaseType.TIME:
        return "std::chrono::time_point<std::chrono::system_clock>", ["<chrono>"]
    if basetype == ed.BaseType.DURATION:
        return "std::chrono::milliseconds", ["<chrono>"]
    if basetype == ed.BaseType.GUID:
        return "std::string", []
    else:
        print("Can't find type for {}")

#recursive
def get_datamodel_property_type_string(object_type, this_package):
    required_imports = []
    if isinstance(object_type, ed.BaseType):
        return basetype_to_datamodel(object_type)
    if isinstance(object_type, ed.TypeDef):
        return get_datamodel_property_type_string(object_type.basetype, this_package)
    if isinstance(object_type, ed.Collection):
        text, imports = get_datamodel_property_type_string(
            object_type.contained_type, this_package)
        return text, imports

    filename = get_filename_from_entity(object_type)

    localfilename = object_type.from_file.split("/")[-1].split(".")[0]
    required_imports.append(filename)
    if hasattr(object_type, "namespace"):
        if this_package == object_type.namespace.split(".")[0]:
            return localfilename + "_" + object_type.name, required_imports
        #return  object_type.namespace.split(".")[0] + "_" + object_type.name, required_imports
        return  localfilename + "_" + object_type.namespace.split(".")[0], required_imports
        #return  object_type.name, required_imports
    return None, required_imports

#recursive
def generate_datamodel_member(typedef, message_name, package_name):
    datamodel_out = ""
    required_imports = []
    props = []
    if isinstance(typedef, ed.EntityType):
        if typedef.basetype is not None:
            rprops, includes  = generate_datamodel_member(
                typedef.basetype, message_name, package_name)
            required_imports.extend(includes)
            props.extend(rprops)

        #if len(typedef.properties) != 0:
        #    datamodel_out+= "    // from {}.{}\n".format(typedef.namespace, typedef.name)

        for property_obj in typedef.properties:
            if isinstance(property_obj, ed.NavigationProperty) and not (property_obj.contains_target or property_obj.auto_expand):
                datamodel_type = "NavigationReference"
                if isinstance(property_obj.type, ed.Collection):
                    datamodel_type = datamodel_type
                else:
                    prop_obj_name = property_obj.name
                props.append([datamodel_type,lowerCase(property_obj.name), True])
                required_imports.append("NavigationReference")

            else:
                propType, imports = get_datamodel_property_type_string(
                    property_obj.type, package_name)
                props.append([propType, lowerCase(property_obj.name), True])
                required_imports.extend(imports)

    elif isinstance(typedef, ed.Complex) and  hasattr(typedef, 'properties'):
        if typedef.basetype is not None:
            rprops, includes  = generate_datamodel_member(
                typedef.basetype, message_name, package_name)
            required_imports.extend(includes)
            props.extend(rprops)

        for property_obj in typedef.properties:
            if isinstance(property_obj, ed.NavigationProperty) and not (property_obj.contains_target or property_obj.auto_expand):
                datamodel_type = "NavigationReference"
                if isinstance(property_obj.type, ed.Collection):
                    datamodel_type = datamodel_type
                else:
                    prop_obj_name = property_obj.name
                props.append([datamodel_type,lowerCase(property_obj.name), True])
                required_imports.append("NavigationReference")

            else:
                propType, imports = get_datamodel_property_type_string(
                    property_obj.type, package_name)
                props.append([propType, lowerCase(property_obj.name), True])
                required_imports.extend(imports)

    return props, required_imports




def generate_datamodel_filename_from_typedef(typedef, dm):
    new_filename = get_filename_from_entity(typedef)
    #filepath = os.path.join(new_filename, typedef.name + ".h")
    filepath = new_filename +".h"
    filepath = os.path.join(DATAMODEL_DIR, filepath)
    try:
        os.makedirs(os.path.dirname(filepath))
    except FileExistsError:
        pass

    message_name = new_filename + "_" + typedef.name
    package_name = typedef.namespace.split(".")[0]

    required_imports = []
    datamodel_out = ""

    # Entity
    if isinstance(typedef, ed.EntityType):
         props, include = generate_datamodel_member(typedef, message_name, package_name)
         required_imports.extend(include)
         newStruct = {}
         newStruct[message_name] = Struct(message_name, props)
         newdm = DMfile(include, [], newStruct)

    # Enum
    elif isinstance(typedef, ed.Enum):
         enumerators = []
         for prop_index, member in enumerate(typedef.values):
              enumerators.append(member)
         newEnum = Enum(new_filename + "_" + typedef.name, enumerators)
         newdm = DMfile([], [newEnum], [])

    # Complex
    elif isinstance(typedef, ed.Complex):
         props, include = generate_datamodel_member(typedef, message_name, package_name)
         required_imports.extend(include)
         newStruct = {}
         newStruct[message_name]=Struct(message_name, props)

         newdm = DMfile(include, [], newStruct)

    else:
         print("Unsure how to generate for type ".format(type(typedef)))
         newdm = DMfile([], [], [])

    if filepath in dm.keys():
        dm[filepath] = dm[filepath].combine(newdm)
    else:
        dm[filepath] = newdm

    return dm



def main():
    for root, dirs, files in os.walk(REDFISH_SCHEMA_DIR):
        filepaths = [os.path.join(root, filename) for filename in files if not filename.startswith("OemAccountService")]
        flat_list = []

        for filepath in filepaths:
            flat_list.extend(ed.parse_file(filepath, []))
        flat_list = ed.remove_old_schemas(flat_list)

        flat_list.sort(key=lambda x: x.name.lower())
        flat_list.sort(key=lambda x: not x.name.startswith("ServiceRoot"))
        # build a data structure based on the xml
        for element in flat_list:
            ed.instantiate_abstract_classes(flat_list, element)

        # dict of datamodel files
        dm = {}
        for thistype in flat_list:
          #print("{}.{}".format(thistype.name, thistype.namespace))
          # go thought the types, and add them to the correct file
          dm = generate_datamodel_filename_from_typedef(thistype, dm)


        for dmFile in dm:
            dm[dmFile].printFile(dmFile)

if __name__ == "__main__":
    main()

