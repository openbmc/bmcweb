#ifndef BIOS_V1
#define BIOS_V1

#include "Bios_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"

struct Bios_v1_Actions
{
    Bios_v1_OemActions oem;
};
struct Bios_v1_Attributes
{};
struct Bios_v1_Bios
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string attributeRegistry;
    Bios_v1_Actions actions;
    Bios_v1_Attributes attributes;
    Bios_v1_Links links;
};
struct Bios_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ activeSoftwareImage;
    NavigationReference__ softwareImages;
};
struct Bios_v1_OemActions
{};
#endif
