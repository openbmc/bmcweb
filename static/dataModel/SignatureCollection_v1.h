#ifndef SIGNATURECOLLECTION_V1
#define SIGNATURECOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct SignatureCollection_v1_SignatureCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
