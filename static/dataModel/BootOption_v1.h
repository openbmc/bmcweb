#ifndef BOOTOPTION_V1
#define BOOTOPTION_V1

#include "BootOption_v1.h"
#include "ComputerSystem_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"

struct BootOption_v1_Actions
{
    BootOption_v1_OemActions oem;
};
struct BootOption_v1_BootOption
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string bootOptionReference;
    std::string displayName;
    bool bootOptionEnabled;
    std::string uefiDevicePath;
    ComputerSystem_v1_ComputerSystem alias;
    NavigationReference__ relatedItem;
    BootOption_v1_Actions actions;
};
struct BootOption_v1_OemActions
{};
#endif
