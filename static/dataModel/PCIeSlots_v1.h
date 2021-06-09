#ifndef PCIESLOTS_V1
#define PCIESLOTS_V1

#include "NavigationReferenceRedfish.h"
#include "PCIeDevice_v1.h"
#include "PCIeSlots_v1.h"
#include "Resource_v1.h"

enum class PCIeSlotsV1SlotTypes
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
struct PCIeSlotsV1OemActions
{};
struct PCIeSlotsV1Actions
{
    PCIeSlotsV1OemActions oem;
};
struct PCIeSlotsV1PCIeLinks
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish pCIeDevice;
};
struct PCIeSlotsV1PCIeSlot
{
    ResourceV1Resource oem;
    PCIeDeviceV1PCIeDevice pCIeType;
    PCIeSlotsV1SlotTypes slotType;
    int64_t lanes;
    ResourceV1Resource status;
    ResourceV1Resource location;
    PCIeSlotsV1PCIeLinks links;
    bool hotPluggable;
    bool locationIndicatorActive;
};
struct PCIeSlotsV1PCIeSlots
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    PCIeSlotsV1PCIeSlot slots;
    PCIeSlotsV1Actions actions;
};
#endif
