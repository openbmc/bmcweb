#ifndef SERVICEROOT_V1
#define SERVICEROOT_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"
#include "ServiceRoot_v1.h"

struct ServiceRootV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ sessions;
};
struct ServiceRootV1Expand
{
    bool links;
    bool noLinks;
    bool expandAll;
    bool levels;
    int64_t maxLevels;
};
struct ServiceRootV1DeepOperations
{
    bool deepPATCH;
    bool deepPOST;
    int64_t maxLevels;
};
struct ServiceRootV1ProtocolFeaturesSupported
{
    ServiceRootV1Expand expandQuery;
    bool filterQuery;
    bool selectQuery;
    bool excerptQuery;
    bool onlyMemberQuery;
    ServiceRootV1DeepOperations deepOperations;
};
struct ServiceRootV1ServiceRoot
{
    ResourceV1Resource oem;
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
    ServiceRootV1Links links;
    NavigationReference_ storageSystems;
    NavigationReference_ storageServices;
    NavigationReference_ fabrics;
    NavigationReference_ updateService;
    NavigationReference_ compositionService;
    std::string product;
    ServiceRootV1ProtocolFeaturesSupported protocolFeaturesSupported;
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
