#ifndef MESSAGEREGISTRYFILE_V1
#define MESSAGEREGISTRYFILE_V1

#include "MessageRegistryFile_v1.h"
#include "Resource_v1.h"

struct MessageRegistryFileV1OemActions
{};
struct MessageRegistryFileV1Actions
{
    MessageRegistryFileV1OemActions oem;
};
struct MessageRegistryFileV1Location
{
    std::string language;
    std::string uri;
    std::string archiveUri;
    std::string publicationUri;
    std::string archiveFile;
};
struct MessageRegistryFileV1MessageRegistryFile
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string languages;
    std::string registry;
    MessageRegistryFileV1Location location;
    MessageRegistryFileV1Actions actions;
};
#endif
