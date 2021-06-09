#ifndef NVMEDOMAIN_V1
#define NVMEDOMAIN_V1

#include "NVMeDomain_v1.h"
#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct NVMeDomainV1OemActions
{};
struct NVMeDomainV1Actions
{
    NVMeDomainV1OemActions oem;
};
struct NVMeDomainV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish associatedDomains;
};
struct NVMeDomainV1NVMeDomain
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    NavigationReferenceRedfish domainMembers;
    int64_t totalDomainCapacityBytes;
    int64_t unallocatedDomainCapacityBytes;
    int64_t maximumCapacityPerEnduranceGroupBytes;
    NavigationReferenceRedfish availableFirmwareImages;
    NVMeDomainV1Links links;
    NVMeDomainV1Actions actions;
};
#endif
