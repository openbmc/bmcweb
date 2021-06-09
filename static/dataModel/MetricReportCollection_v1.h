#ifndef METRICREPORTCOLLECTION_V1
#define METRICREPORTCOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct MetricReportCollection_v1_MetricReportCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
