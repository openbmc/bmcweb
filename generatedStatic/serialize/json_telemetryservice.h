#ifndef SERIALIZE_JSONTELEMETRY
#define SERIALIZE_JSONTELEMETRY

#include "redfish-core/include/utils/time_utils.hpp"

#include <generatedStatic/dataModel/TelemetryService_v1.h>

void jsonSerializeTelemetryservice(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    TelemetryServiceV1TelemetryService* telemetryService)
{
    asyncResp->res.jsonValue["@odata.type"] = telemetryService->type;
    asyncResp->res.jsonValue["@odata.id"] = telemetryService->odata;
    asyncResp->res.jsonValue["Id"] = telemetryService->id;
    asyncResp->res.jsonValue["Name"] = telemetryService->name;
    asyncResp->res.jsonValue["MetricReportDefinitions"]["@odata.id"] =
        telemetryService->metricDefinitions.id;
    asyncResp->res.jsonValue["MetricReports"]["@odata.id"] =
        telemetryService->metricReports.id;
    switch (telemetryService->status.state)
    {
        case (ResourceV1State::Absent):
        {
            asyncResp->res.jsonValue["Status"]["State"] = "Absent";
            break;
        }
        case (ResourceV1State::Enabled):
        {
            asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
            break;
        }
        default:
            break;
    }
    asyncResp->res.jsonValue["MaxReports"] = telemetryService->maxReports;
    asyncResp->res.jsonValue["MinCollectionInterval"] =
        redfish::time_utils::toDurationString(
            telemetryService->minCollectionInterval);
    return;
}
#endif
