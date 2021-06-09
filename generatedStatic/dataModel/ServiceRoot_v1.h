#ifndef SERVICEROOT_V1
#define SERVICEROOT_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"
#include "ServiceRoot_v1.h"

struct ServiceRootV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish sessions;
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
    NavigationReferenceRedfish systems;
    NavigationReferenceRedfish chassis;
    NavigationReferenceRedfish managers;
    NavigationReferenceRedfish tasks;
    NavigationReferenceRedfish sessionService;
    NavigationReferenceRedfish accountService;
    NavigationReferenceRedfish eventService;
    NavigationReferenceRedfish registries;
    NavigationReferenceRedfish jsonSchemas;
    ServiceRootV1Links links;
    NavigationReferenceRedfish storageSystems;
    NavigationReferenceRedfish storageServices;
    NavigationReferenceRedfish fabrics;
    NavigationReferenceRedfish updateService;
    NavigationReferenceRedfish compositionService;
    std::string product;
    ServiceRootV1ProtocolFeaturesSupported protocolFeaturesSupported;
    NavigationReferenceRedfish jobService;
    NavigationReferenceRedfish telemetryService;
    std::string vendor;
    NavigationReferenceRedfish certificateService;
    NavigationReferenceRedfish resourceBlocks;
    NavigationReferenceRedfish powerEquipment;
    NavigationReferenceRedfish facilities;
    NavigationReferenceRedfish aggregationService;
    NavigationReferenceRedfish storage;
    NavigationReferenceRedfish nVMeDomains;
};
#endif
