#ifndef SIGNATURECOLLECTION_V1
#define SIGNATURECOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct SignatureCollectionV1SignatureCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
