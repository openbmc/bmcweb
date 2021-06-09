#ifndef NETWORKINTERFACE_V1
#define NETWORKINTERFACE_V1

#include "NavigationReference__.h"
#include "NetworkDeviceFunctionCollection_v1.h"
#include "NetworkInterface_v1.h"
#include "NetworkPortCollection_v1.h"
#include "PortCollection_v1.h"
#include "Resource_v1.h"

struct NetworkInterface_v1_Actions
{
    NetworkInterface_v1_OemActions oem;
};
struct NetworkInterface_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ networkAdapter;
};
struct NetworkInterface_v1_NetworkInterface
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    NetworkInterface_v1_Links links;
    NetworkPortCollection_v1_NetworkPortCollection networkPorts;
    NetworkDeviceFunctionCollection_v1_NetworkDeviceFunctionCollection networkDeviceFunctions;
    NetworkInterface_v1_Actions actions;
    PortCollection_v1_PortCollection ports;
};
struct NetworkInterface_v1_OemActions
{
};
#endif
