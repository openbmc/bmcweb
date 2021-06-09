#ifndef POWERDOMAIN_V1
#define POWERDOMAIN_V1

#include "NavigationReference_.h"
#include "PowerDomain_v1.h"
#include "Resource_v1.h"

struct PowerDomainV1OemActions
{};
struct PowerDomainV1Actions
{
    PowerDomainV1OemActions oem;
};
struct PowerDomainV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ floorPDUs;
    NavigationReference_ rackPDUs;
    NavigationReference_ transferSwitches;
    NavigationReference_ switchgear;
    NavigationReference_ managedBy;
};
struct PowerDomainV1PowerDomain
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    PowerDomainV1Links links;
    PowerDomainV1Actions actions;
};
#endif
