#ifndef VCATENTRY_V1
#define VCATENTRY_V1

#include "Resource_v1.h"
#include "VCATEntry_v1.h"

struct VCATEntry_v1_Actions
{
    VCATEntry_v1_OemActions oem;
};
struct VCATEntry_v1_OemActions
{
};
struct VCATEntry_v1_VCATableEntry
{
    std::string vCMask;
    std::string threshold;
};
struct VCATEntry_v1_VCATEntry
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string rawEntryHex;
    VCATEntry_v1_VCATableEntry vCEntries;
    VCATEntry_v1_Actions actions;
};
#endif
