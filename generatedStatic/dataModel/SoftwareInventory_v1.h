#ifndef SOFTWAREINVENTORY_V1
#define SOFTWAREINVENTORY_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"
#include "SoftwareInventory_v1.h"

#include <chrono>

struct SoftwareInventoryV1OemActions
{};
struct SoftwareInventoryV1Actions
{
    SoftwareInventoryV1OemActions oem;
};
struct SoftwareInventoryV1MeasurementBlock
{
    int64_t measurementSpecification;
    int64_t measurementSize;
    std::string measurement;
};
struct SoftwareInventoryV1SoftwareInventory
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    std::string version;
    bool updateable;
    SoftwareInventoryV1Actions actions;
    std::string softwareId;
    std::string lowestSupportedVersion;
    std::string uefiDevicePaths;
    NavigationReferenceRedfish relatedItem;
    std::string manufacturer;
    std::chrono::time_point<std::chrono::system_clock> releaseDate;
    bool writeProtected;
    SoftwareInventoryV1MeasurementBlock measurement;
};
#endif
