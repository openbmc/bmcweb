#ifndef AGGREGATIONSOURCE_V1
#define AGGREGATIONSOURCE_V1

#include "AggregationSource_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"

struct AggregationSource_v1_Actions
{
    AggregationSource_v1_OemActions oem;
};
struct AggregationSource_v1_AggregationSource
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string hostName;
    std::string userName;
    std::string password;
    AggregationSource_v1_Links links;
    AggregationSource_v1_Actions actions;
};
struct AggregationSource_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ connectionMethod;
    NavigationReference__ resourcesAccessed;
};
struct AggregationSource_v1_OemActions
{};
#endif
