#ifndef CONNECTIONMETHOD_V1
#define CONNECTIONMETHOD_V1

#include "ConnectionMethod_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"

enum class ConnectionMethod_v1_ConnectionMethodType
{
    Redfish,
    SNMP,
    IPMI15,
    IPMI20,
    NETCONF,
    OEM,
};
struct ConnectionMethod_v1_Actions
{
    ConnectionMethod_v1_OemActions oem;
};
struct ConnectionMethod_v1_ConnectionMethod
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ConnectionMethod_v1_ConnectionMethodType connectionMethodType;
    std::string connectionMethodVariant;
    ConnectionMethod_v1_Links links;
    ConnectionMethod_v1_Actions actions;
};
struct ConnectionMethod_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ aggregationSources;
};
struct ConnectionMethod_v1_OemActions
{};
#endif
