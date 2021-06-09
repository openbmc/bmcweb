#ifndef SERVICEROOT_V1
#define SERVICEROOT_V1

#include "NavigationReference.h"
#include "Resource_v1.h"
#include "ServiceRoot_v1.h"

struct ServiceRoot_v1_DeepOperations
{
    bool deepPATCH;
    bool deepPOST;
    int64_t maxLevels;
};
struct ServiceRoot_v1_Expand
{
    bool links;
    bool noLinks;
    bool expandAll;
    bool levels;
    int64_t maxLevels;
};
struct ServiceRoot_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference_ sessions;
};
struct ServiceRoot_v1_ProtocolFeaturesSupported
{
    ServiceRoot_v1_Expand expandQuery;
    bool filterQuery;
    bool selectQuery;
    bool excerptQuery;
    bool onlyMemberQuery;
    ServiceRoot_v1_DeepOperations deepOperations;
};
struct ServiceRoot_v1_ServiceRoot
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string redfishVersion;
    std::string UUID;
    NavigationReference_ systems;
    NavigationReference_ chassis;
    NavigationReference_ managers;
    NavigationReference_ tasks;
    NavigationReference_ sessionService;
    NavigationReference_ accountService;
    NavigationReference_ eventService;
    NavigationReference_ registries;
    NavigationReference_ jsonSchemas;
    ServiceRoot_v1_Links links;
    NavigationReference_ storageSystems;
    NavigationReference_ storageServices;
    NavigationReference_ fabrics;
    NavigationReference_ updateService;
    NavigationReference_ compositionService;
    std::string product;
    ServiceRoot_v1_ProtocolFeaturesSupported protocolFeaturesSupported;
    NavigationReference_ jobService;
    NavigationReference_ telemetryService;
    std::string vendor;
    NavigationReference_ certificateService;
    NavigationReference_ resourceBlocks;
    NavigationReference_ powerEquipment;
    NavigationReference_ facilities;
    NavigationReference_ aggregationService;
    NavigationReference_ storage;
    NavigationReference_ nVMeDomains;
};
#endif
