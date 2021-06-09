#ifndef VLANNETWORKINTERFACE_V1
#define VLANNETWORKINTERFACE_V1

#include "Resource_v1.h"
#include "VLanNetworkInterface_v1.h"

struct VLanNetworkInterface_v1_Actions
{
    VLanNetworkInterface_v1_OemActions oem;
};
struct VLanNetworkInterface_v1_OemActions
{};
struct VLanNetworkInterface_v1_VLAN
{
    bool vLANEnable;
    int64_t vLANId;
};
struct VLanNetworkInterface_v1_VLanNetworkInterface
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    bool vLANEnable;
    int64_t vLANId;
    VLanNetworkInterface_v1_Actions actions;
};
#endif
