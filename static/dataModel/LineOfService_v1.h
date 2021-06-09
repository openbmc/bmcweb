#ifndef LINEOFSERVICE_V1
#define LINEOFSERVICE_V1

#include "Resource_v1.h"

struct LineOfService_v1_LineOfService
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
};
#endif
