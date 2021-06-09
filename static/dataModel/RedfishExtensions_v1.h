#ifndef REDFISHEXTENSIONS_V1
#define REDFISHEXTENSIONS_V1

#include "RedfishExtensions_v1.h"

enum class RedfishExtensions_v1_ReleaseStatusType
{
    Standard,
    Informational,
    WorkInProgress,
    InDevelopment,
};
enum class RedfishExtensions_v1_RevisionKind
{
    Added,
    Modified,
    Deprecated,
};
struct RedfishExtensions_v1_EnumerationMember
{
    std::string member;
};
struct RedfishExtensions_v1_PropertyPattern
{
    std::string pattern;
    std::string type;
};
struct RedfishExtensions_v1_RevisionType
{
    std::string version;
    RedfishExtensions_v1_RevisionKind kind;
    std::string description;
};
#endif
