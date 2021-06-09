#ifndef REDUNDANCY_V1
#define REDUNDANCY_V1

#include "NavigationReference__.h"
#include "Redundancy_v1.h"
#include "Resource_v1.h"

struct Redundancy_v1_Actions
{
    Redundancy_v1_OemActions oem;
};
struct Redundancy_v1_OemActions
{};
struct Redundancy_v1_Redundancy
{
    Resource_v1_Resource oem;
    std::string memberId;
    std::string name;
    std::string mode;
    int64_t maxNumSupported;
    int64_t minNumNeeded;
    Resource_v1_Resource status;
    NavigationReference__ redundancySet;
    bool redundancyEnabled;
    Redundancy_v1_Actions actions;
};
#endif
