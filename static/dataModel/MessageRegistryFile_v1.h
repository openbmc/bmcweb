#ifndef MESSAGEREGISTRYFILE_V1
#define MESSAGEREGISTRYFILE_V1

#include "MessageRegistryFile_v1.h"
#include "Resource_v1.h"

struct MessageRegistryFile_v1_Actions
{
    MessageRegistryFile_v1_OemActions oem;
};
struct MessageRegistryFile_v1_Location
{
    std::string language;
    std::string uri;
    std::string archiveUri;
    std::string publicationUri;
    std::string archiveFile;
};
struct MessageRegistryFile_v1_MessageRegistryFile
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string languages;
    std::string registry;
    MessageRegistryFile_v1_Location location;
    MessageRegistryFile_v1_Actions actions;
};
struct MessageRegistryFile_v1_OemActions
{};
#endif
