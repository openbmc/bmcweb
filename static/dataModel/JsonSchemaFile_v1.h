#ifndef JSONSCHEMAFILE_V1
#define JSONSCHEMAFILE_V1

#include "JsonSchemaFile_v1.h"
#include "Resource_v1.h"

struct JsonSchemaFile_v1_Actions
{
    JsonSchemaFile_v1_OemActions oem;
};
struct JsonSchemaFile_v1_JsonSchemaFile
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string languages;
    std::string schema;
    JsonSchemaFile_v1_Location location;
    JsonSchemaFile_v1_Actions actions;
};
struct JsonSchemaFile_v1_Location
{
    std::string language;
    std::string uri;
    std::string archiveUri;
    std::string publicationUri;
    std::string archiveFile;
};
struct JsonSchemaFile_v1_OemActions
{
};
#endif
