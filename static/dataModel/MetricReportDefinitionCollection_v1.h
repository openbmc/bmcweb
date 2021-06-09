#ifndef METRICREPORTDEFINITIONCOLLECTION_V1
#define METRICREPORTDEFINITIONCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct MetricReportDefinitionCollectionV1MetricReportDefinitionCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
