#ifndef HOSTEDSTORAGESERVICES_V1
#define HOSTEDSTORAGESERVICES_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct HostedStorageServicesV1HostedStorageServices
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
