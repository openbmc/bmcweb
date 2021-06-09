#ifndef SECUREBOOTDATABASECOLLECTION_V1
#define SECUREBOOTDATABASECOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct SecureBootDatabaseCollectionV1SecureBootDatabaseCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
