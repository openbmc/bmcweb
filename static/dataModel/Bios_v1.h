#ifndef BIOS_V1
#define BIOS_V1

#include "Bios_v1.h"
#include "NavigationReference_.h"
#include "Resource_v1.h"

struct BiosV1OemActions
{};
struct BiosV1Actions
{
    BiosV1OemActions oem;
};
struct BiosV1Attributes
{};
struct BiosV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ activeSoftwareImage;
    NavigationReference_ softwareImages;
};
struct BiosV1Bios
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string attributeRegistry;
    BiosV1Actions actions;
    BiosV1Attributes attributes;
    BiosV1Links links;
    bool resetBiosToDefaultsPending;
};
#endif
