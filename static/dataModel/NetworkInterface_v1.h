#ifndef NETWORKINTERFACE_V1
#define NETWORKINTERFACE_V1

#include "NavigationReference_.h"
#include "NetworkDeviceFunctionCollection_v1.h"
#include "NetworkInterface_v1.h"
#include "NetworkPortCollection_v1.h"
#include "PortCollection_v1.h"
#include "Resource_v1.h"

struct NetworkInterfaceV1OemActions
{};
struct NetworkInterfaceV1Actions
{
    NetworkInterfaceV1OemActions oem;
};
struct NetworkInterfaceV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ networkAdapter;
};
struct NetworkInterfaceV1NetworkInterface
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    NetworkInterfaceV1Links links;
    NetworkPortCollectionV1NetworkPortCollection networkPorts;
    NetworkDeviceFunctionCollectionV1NetworkDeviceFunctionCollection
        networkDeviceFunctions;
    NetworkInterfaceV1Actions actions;
    PortCollectionV1PortCollection ports;
};
#endif
