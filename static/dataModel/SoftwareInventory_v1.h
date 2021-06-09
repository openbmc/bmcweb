#ifndef SOFTWAREINVENTORY_V1
#define SOFTWAREINVENTORY_V1

#include <chrono>
#include "NavigationReference__.h"
#include "Resource_v1.h"
#include "SoftwareInventory_v1.h"

struct SoftwareInventory_v1_Actions
{
    SoftwareInventory_v1_OemActions oem;
};
struct SoftwareInventory_v1_OemActions
{
};
struct SoftwareInventory_v1_SoftwareInventory
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    std::string version;
    bool updateable;
    SoftwareInventory_v1_Actions actions;
    std::string softwareId;
    std::string lowestSupportedVersion;
    std::string uefiDevicePaths;
    NavigationReference__ relatedItem;
    std::string manufacturer;
    std::chrono::time_point releaseDate;
    bool writeProtected;
};
#endif
