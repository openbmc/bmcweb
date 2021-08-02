#pragma once

#include "utils/telemetry_utils.hpp"

#include <app.hpp>
#include <registries/privilege_registry.hpp>

namespace redfish
{

inline void requestRoutesTriggerCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/Triggers/")
        .privileges(redfish::privileges::getTriggersCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#TriggersCollection.TriggersCollection";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/TelemetryService/Triggers";
                asyncResp->res.jsonValue["Name"] = "Triggers Collection";

                telemetry::getCollection(
                    asyncResp, telemetry::triggerUri,
                    telemetry::CollectionParams(telemetry::triggerSubtree, 1,
                                                {telemetry::triggerInterface}));
            });
}

} // namespace redfish
