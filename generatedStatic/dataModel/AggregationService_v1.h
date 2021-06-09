#ifndef AGGREGATIONSERVICE_V1
#define AGGREGATIONSERVICE_V1

#include "AggregateCollection_v1.h"
#include "AggregationService_v1.h"
#include "AggregationSourceCollection_v1.h"
#include "ConnectionMethodCollection_v1.h"
#include "Resource_v1.h"

struct AggregationServiceV1OemActions
{};
struct AggregationServiceV1Actions
{
    AggregationServiceV1OemActions oem;
};
struct AggregationServiceV1AggregationService
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    bool serviceEnabled;
    ResourceV1Resource status;
    AggregateCollectionV1AggregateCollection aggregates;
    AggregationSourceCollectionV1AggregationSourceCollection aggregationSources;
    ConnectionMethodCollectionV1ConnectionMethodCollection connectionMethods;
    AggregationServiceV1Actions actions;
};
#endif
