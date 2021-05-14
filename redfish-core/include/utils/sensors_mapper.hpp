#pragma once

#include "utils/sensors_utils.hpp"

namespace redfish
{

/**
 * @brief Retrieves mapping of Redfish URIs to sensor value property to D-Bus
 * path of the sensor.
 *
 * Function builds valid Redfish response for sensor query of given chassis and
 * node. It then builds metadata about Redfish<->D-Bus correlations and
 * provides it to caller in a callback.
 *
 * @param id   Resource id for which retrieval should be performed
 * @param node  Group of sensors. See sensors::node for supported values
 * @param mapComplete   Callback to be called with retrieval result
 */
inline void retrieveUriToDbusMap(const std::string& id, const std::string& node,
                                 SensorsAsyncResp::DataCompleteCb&& mapComplete)
{
    auto pathIt = sensors::dbus::paths.find(node);
    if (pathIt == sensors::dbus::paths.end())
    {
        BMCWEB_LOG_ERROR << "Wrong node provided : " << node;
        mapComplete(boost::beast::http::status::bad_request, {});
        return;
    }

    auto res = std::make_shared<crow::Response>();
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>(*res);
    auto callback =
        [res, asyncResp, mapCompleteCb{std::move(mapComplete)}](
            const boost::beast::http::status status,
            const boost::container::flat_map<std::string, std::string>&
                uriToDbus) { mapCompleteCb(status, uriToDbus); };

    auto resp = std::make_shared<SensorsAsyncResp>(
        std::move(callback), asyncResp, id, node, pathIt->second);
    getChassisData(resp);
}

} // namespace redfish