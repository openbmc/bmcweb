#ifndef NVMEDOMAIN_V1
#define NVMEDOMAIN_V1

#include "NavigationReference.h"
#include "NVMeDomain_v1.h"
#include "Resource_v1.h"

struct NVMeDomain_v1_Actions
{
    NVMeDomain_v1_OemActions oem;
};
struct NVMeDomain_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference_ associatedDomains;
};
struct NVMeDomain_v1_NVMeDomain
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    NavigationReference_ domainMembers;
    int64_t totalDomainCapacityBytes;
    int64_t unallocatedDomainCapacityBytes;
    int64_t maximumCapacityPerEnduranceGroupBytes;
    NavigationReference_ availableFirmwareImages;
    NVMeDomain_v1_Links links;
    NVMeDomain_v1_Actions actions;
};
struct NVMeDomain_v1_OemActions
{
};
#endif
