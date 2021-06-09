#ifndef CONNECTIONMETHOD_V1
#define CONNECTIONMETHOD_V1

#include "ConnectionMethod_v1.h"
#include "NavigationReference_.h"
#include "Resource_v1.h"

enum class ConnectionMethodV1ConnectionMethodType
{
    Redfish,
    SNMP,
    IPMI15,
    IPMI20,
    NETCONF,
    OEM,
};
struct ConnectionMethodV1OemActions
{};
struct ConnectionMethodV1Actions
{
    ConnectionMethodV1OemActions oem;
};
struct ConnectionMethodV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ aggregationSources;
};
struct ConnectionMethodV1ConnectionMethod
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ConnectionMethodV1ConnectionMethodType connectionMethodType;
    std::string connectionMethodVariant;
    ConnectionMethodV1Links links;
    ConnectionMethodV1Actions actions;
};
#endif
