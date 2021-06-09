#ifndef POWEREQUIPMENT_V1
#define POWEREQUIPMENT_V1

#include "NavigationReferenceRedfish.h"
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
    NavigationReferenceRedfish managedBy;
};
struct PowerEquipmentV1PowerEquipment
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    NavigationReferenceRedfish floorPDUs;
    NavigationReferenceRedfish rackPDUs;
    NavigationReferenceRedfish switchgear;
    NavigationReferenceRedfish transferSwitches;
    PowerEquipmentV1Links links;
    PowerEquipmentV1Actions actions;
};
#endif
