#ifndef AGGREGATIONSERVICE_V1
#define AGGREGATIONSERVICE_V1

#include "AggregateCollection_v1.h"
#include "AggregationService_v1.h"
#include "AggregationSourceCollection_v1.h"
#include "ConnectionMethodCollection_v1.h"
#include "Resource_v1.h"

struct AggregationService_v1_Actions
{
    AggregationService_v1_OemActions oem;
};
struct AggregationService_v1_AggregationService
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    bool serviceEnabled;
    Resource_v1_Resource status;
    AggregateCollection_v1_AggregateCollection aggregates;
    AggregationSourceCollection_v1_AggregationSourceCollection aggregationSources;
    ConnectionMethodCollection_v1_ConnectionMethodCollection connectionMethods;
    AggregationService_v1_Actions actions;
};
struct AggregationService_v1_OemActions
{
};
#endif
