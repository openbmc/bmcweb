#ifndef BOOTOPTION_V1
#define BOOTOPTION_V1

#include "BootOption_v1.h"
#include "ComputerSystem_v1.h"
#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct BootOptionV1OemActions
{};
struct BootOptionV1Actions
{
    BootOptionV1OemActions oem;
};
struct BootOptionV1BootOption
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string bootOptionReference;
    std::string displayName;
    bool bootOptionEnabled;
    std::string uefiDevicePath;
    ComputerSystemV1ComputerSystem alias;
    NavigationReferenceRedfish relatedItem;
    BootOptionV1Actions actions;
};
#endif
