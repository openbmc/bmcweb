#ifndef POWERDOMAIN_V1
#define POWERDOMAIN_V1

#include "NavigationReferenceRedfish.h"
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
    NavigationReferenceRedfish floorPDUs;
    NavigationReferenceRedfish rackPDUs;
    NavigationReferenceRedfish transferSwitches;
    NavigationReferenceRedfish switchgear;
    NavigationReferenceRedfish managedBy;
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
