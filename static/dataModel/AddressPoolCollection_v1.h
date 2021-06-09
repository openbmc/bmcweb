#ifndef ADDRESSPOOLCOLLECTION_V1
#define ADDRESSPOOLCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct AddressPoolCollectionV1AddressPoolCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
