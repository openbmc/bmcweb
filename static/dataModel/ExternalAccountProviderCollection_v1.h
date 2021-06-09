#ifndef EXTERNALACCOUNTPROVIDERCOLLECTION_V1
#define EXTERNALACCOUNTPROVIDERCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct ExternalAccountProviderCollectionV1ExternalAccountProviderCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
