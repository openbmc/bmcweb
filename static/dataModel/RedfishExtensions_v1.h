#ifndef REDFISHEXTENSIONS_V1
#define REDFISHEXTENSIONS_V1

#include "RedfishExtensions_v1.h"

enum class RedfishExtensionsV1ReleaseStatusType
{
    Standard,
    Informational,
    WorkInProgress,
    InDevelopment,
};
enum class RedfishExtensionsV1RevisionKind
{
    Added,
    Modified,
    Deprecated,
};
struct RedfishExtensionsV1EnumerationMember
{
    std::string member;
};
struct RedfishExtensionsV1PropertyPattern
{
    std::string pattern;
    std::string type;
};
struct RedfishExtensionsV1RevisionType
{
    std::string version;
    RedfishExtensionsV1RevisionKind kind;
    std::string description;
};
#endif
