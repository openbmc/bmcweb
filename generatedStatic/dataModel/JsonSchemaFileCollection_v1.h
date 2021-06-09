#ifndef JSONSCHEMAFILECOLLECTION_V1
#define JSONSCHEMAFILECOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct JsonSchemaFileCollectionV1JsonSchemaFileCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
