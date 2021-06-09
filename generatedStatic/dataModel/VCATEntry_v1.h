#ifndef VCATENTRY_V1
#define VCATENTRY_V1

#include "Resource_v1.h"
#include "VCATEntry_v1.h"

struct VCATEntryV1OemActions
{};
struct VCATEntryV1Actions
{
    VCATEntryV1OemActions oem;
};
struct VCATEntryV1VCATableEntry
{
    std::string vCMask;
    std::string threshold;
};
struct VCATEntryV1VCATEntry
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string rawEntryHex;
    VCATEntryV1VCATableEntry vCEntries;
    VCATEntryV1Actions actions;
};
#endif
