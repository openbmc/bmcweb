#ifndef METRICREPORTCOLLECTION_V1
#define METRICREPORTCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct MetricReportCollectionV1MetricReportCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
