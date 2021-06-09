#ifndef CERTIFICATECOLLECTION_V1
#define CERTIFICATECOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct CertificateCollectionV1CertificateCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
