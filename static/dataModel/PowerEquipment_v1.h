#ifndef POWEREQUIPMENT_V1
#define POWEREQUIPMENT_V1

#include "NavigationReference_.h"
#include "PowerEquipment_v1.h"
#include "Resource_v1.h"

struct PowerEquipmentV1OemActions
{};
struct PowerEquipmentV1Actions
{
    PowerEquipmentV1OemActions oem;
};
struct PowerEquipmentV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ managedBy;
};
struct PowerEquipmentV1PowerEquipment
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    NavigationReference_ floorPDUs;
    NavigationReference_ rackPDUs;
    NavigationReference_ switchgear;
    NavigationReference_ transferSwitches;
    PowerEquipmentV1Links links;
    PowerEquipmentV1Actions actions;
};
#endif
