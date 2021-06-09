#ifndef VLANNETWORKINTERFACE_V1
#define VLANNETWORKINTERFACE_V1

#include "Resource_v1.h"
#include "VLanNetworkInterface_v1.h"

struct VLanNetworkInterfaceV1OemActions
{};
struct VLanNetworkInterfaceV1Actions
{
    VLanNetworkInterfaceV1OemActions oem;
};
struct VLanNetworkInterfaceV1VLAN
{
    bool vLANEnable;
    int64_t vLANId;
    int64_t vLANPriority;
};
struct VLanNetworkInterfaceV1VLanNetworkInterface
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    bool vLANEnable;
    int64_t vLANId;
    VLanNetworkInterfaceV1Actions actions;
    int64_t vLANPriority;
};
#endif
