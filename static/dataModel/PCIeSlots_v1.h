#ifndef PCIESLOTS_V1
#define PCIESLOTS_V1

#include "NavigationReference__.h"
#include "PCIeDevice_v1.h"
#include "PCIeSlots_v1.h"
#include "Resource_v1.h"

enum class PCIeSlots_v1_SlotTypes
{
    FullLength,
    HalfLength,
    LowProfile,
    Mini,
    M2,
    OEM,
    OCP3Small,
    OCP3Large,
    U2,
};
struct PCIeSlots_v1_Actions
{
    PCIeSlots_v1_OemActions oem;
};
struct PCIeSlots_v1_OemActions
{};
struct PCIeSlots_v1_PCIeLinks
{
    Resource_v1_Resource oem;
    NavigationReference__ pCIeDevice;
};
struct PCIeSlots_v1_PCIeSlot
{
    Resource_v1_Resource oem;
    PCIeDevice_v1_PCIeDevice pCIeType;
    PCIeSlots_v1_SlotTypes slotType;
    int64_t lanes;
    Resource_v1_Resource status;
    Resource_v1_Resource location;
    PCIeSlots_v1_PCIeLinks links;
    bool hotPluggable;
};
struct PCIeSlots_v1_PCIeSlots
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    PCIeSlots_v1_PCIeSlot slots;
    PCIeSlots_v1_Actions actions;
    bool locationIndicatorActive;
};
#endif
