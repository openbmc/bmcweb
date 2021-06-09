#ifndef BIOS_V1
#define BIOS_V1

#include "Action.h"
#include "Bios_v1.h"
#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct BiosV1OemActions
{};
struct BiosV1Actions
{
    Action resetBios; // JEBR_ADD
    BiosV1OemActions oem;
};
struct BiosV1Attributes
{};
struct BiosV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish activeSoftwareImage;
    NavigationReferenceRedfish softwareImages;
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
    std::string type;
};
#endif
