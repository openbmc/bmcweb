#ifndef LINEOFSERVICE_V1
#define LINEOFSERVICE_V1

#include "Resource_v1.h"

struct LineOfServiceV1LineOfService
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
};
#endif
