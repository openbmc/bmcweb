#ifndef POWEREQUIPMENT_V1
#define POWEREQUIPMENT_V1

#include "NavigationReference__.h"
#include "PowerEquipment_v1.h"
#include "Resource_v1.h"

struct PowerEquipment_v1_Actions
{
    PowerEquipment_v1_OemActions oem;
};
struct PowerEquipment_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ managedBy;
};
struct PowerEquipment_v1_OemActions
{};
struct PowerEquipment_v1_PowerEquipment
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    NavigationReference__ floorPDUs;
    NavigationReference__ rackPDUs;
    NavigationReference__ switchgear;
    NavigationReference__ transferSwitches;
    PowerEquipment_v1_Links links;
    PowerEquipment_v1_Actions actions;
};
#endif
