#include "ports.hpp"

#include <app.hpp>

namespace redfish
{

inline void handleDedicatedNetworkPortMetricsGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const int& portId)
{
      if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    // Read from /proc/net/dev
    PortInfo portInfo = getPortInfo(portId);

    asyncResp->res.jsonValue["@odata.type"] = "#PortMetrics.v1_3_0.PortMetrics";
    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Managers/bmc/DedicatedNetworkPorts/" +
        std::to_string(portId) + "/Metrics";
    asyncResp->res.jsonValue["Id"] = "Metrics";
    asyncResp->res.jsonValue["Name"] = portInfo.interfaceName + " Metrics";
    nlohmann::json networkingMetrics;
    networkingMetrics["TXDropped"] = portInfo.TXDropped;
    networkingMetrics["RXDropped"] = portInfo.RXDropped;
    asyncResp->res.jsonValue["Networking"] = networkingMetrics;
}

inline void requestPortMetricsRoutes(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Managers/bmc/DedicatedNetworkPorts/<int>/Metrics")
        .privileges(redfish::privileges::getPortMetrics)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleDedicatedNetworkPortMetricsGet, std::ref(app)));
}
} // namespace redfish