#ifndef JSONSCHEMAFILE_V1
#define JSONSCHEMAFILE_V1

#include "JsonSchemaFile_v1.h"
#include "Resource_v1.h"

struct JsonSchemaFileV1OemActions
{};
struct JsonSchemaFileV1Actions
{
    JsonSchemaFileV1OemActions oem;
};
struct JsonSchemaFileV1Location
{
    std::string language;
    std::string uri;
    std::string archiveUri;
    std::string publicationUri;
    std::string archiveFile;
};
struct JsonSchemaFileV1JsonSchemaFile
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string languages;
    std::string schema;
    JsonSchemaFileV1Location location;
    JsonSchemaFileV1Actions actions;
};
#endif
