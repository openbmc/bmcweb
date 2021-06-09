#ifndef POWERDOMAIN_V1
#define POWERDOMAIN_V1

#include "NavigationReference__.h"
#include "PowerDomain_v1.h"
#include "Resource_v1.h"

struct PowerDomain_v1_Actions
{
    PowerDomain_v1_OemActions oem;
};
struct PowerDomain_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ floorPDUs;
    NavigationReference__ rackPDUs;
    NavigationReference__ transferSwitches;
    NavigationReference__ switchgear;
    NavigationReference__ managedBy;
};
struct PowerDomain_v1_OemActions
{};
struct PowerDomain_v1_PowerDomain
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    PowerDomain_v1_Links links;
    PowerDomain_v1_Actions actions;
};
#endif
