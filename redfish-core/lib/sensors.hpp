// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2018 Intel Corporation
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/redundancy.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "sensors_async_response.hpp"
#include "str_utility.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"
#include "utils/query_param.hpp"
#include "utils/sensor_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace redfish
{

/**
 * @brief Retrieves mapping of Redfish URIs to sensor value property to D-Bus
 * path of the sensor.
 *
 * Function builds valid Redfish response for sensor query of given chassis and
 * node. It then builds metadata about Redfish<->D-Bus correlations and provides
 * it to caller in a callback.
 *
 * @param chassis   Chassis for which retrieval should be performed
 * @param node  Node (group) of sensors. See sensor_utils::node for supported
 * values
 * @param mapComplete   Callback to be called with retrieval result
 */
template <typename Callback>
inline void retrieveUriToDbusMap(
    const std::string& chassis, const std::string& node, Callback&& mapComplete)
{
    decltype(sensors::paths)::const_iterator pathIt =
        std::find_if(sensors::paths.cbegin(), sensors::paths.cend(),
                     [&node](auto&& val) { return val.first == node; });
    if (pathIt == sensors::paths.cend())
    {
        BMCWEB_LOG_ERROR("Wrong node provided : {}", node);
        std::map<std::string, std::string> noop;
        mapComplete(boost::beast::http::status::bad_request, noop);
        return;
    }

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    auto callback =
        [asyncResp, mapCompleteCb = std::forward<Callback>(mapComplete)](
            const boost::beast::http::status status,
            const std::map<std::string, std::string>& uriToDbus) {
            mapCompleteCb(status, uriToDbus);
        };

    auto resp = std::make_shared<SensorsAsyncResp>(
        asyncResp, chassis, pathIt->second, node, std::move(callback));
    getChassisData(resp);
}

namespace sensors
{

inline void getChassisCallback(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::string_view chassisId, std::string_view chassisSubNode,
    const std::shared_ptr<std::set<std::string>>& sensorNames)
{
    BMCWEB_LOG_DEBUG("getChassisCallback enter ");

    nlohmann::json& entriesArray = asyncResp->res.jsonValue["Members"];
    for (const std::string& sensor : *sensorNames)
    {
        BMCWEB_LOG_DEBUG("Adding sensor: {}", sensor);

        sdbusplus::message::object_path path(sensor);
        std::string sensorName = path.filename();
        if (sensorName.empty())
        {
            BMCWEB_LOG_ERROR("Invalid sensor path: {}", sensor);
            messages::internalError(asyncResp->res);
            return;
        }
        std::string type = path.parent_path().filename();
        std::string id = redfish::sensor_utils::getSensorId(sensorName, type);

        nlohmann::json::object_t member;
        member["@odata.id"] = boost::urls::format(
            "/redfish/v1/Chassis/{}/{}/{}", chassisId, chassisSubNode, id);

        entriesArray.emplace_back(std::move(member));
    }

    asyncResp->res.jsonValue["Members@odata.count"] = entriesArray.size();
    BMCWEB_LOG_DEBUG("getChassisCallback exit");
}

inline void handleSensorCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    query_param::QueryCapabilities capabilities = {
        .canDelegateExpandLevel = 1,
    };
    query_param::Query delegatedQuery;
    if (!redfish::setUpRedfishRouteWithDelegation(app, req, asyncResp,
                                                  delegatedQuery, capabilities))
    {
        return;
    }

    if (delegatedQuery.expandType != query_param::ExpandType::None)
    {
        // we perform efficient expand.
        auto sensorsAsyncResp = std::make_shared<SensorsAsyncResp>(
            asyncResp, chassisId, sensors::dbus::sensorPaths,
            sensors::sensorsNodeStr,
            /*efficientExpand=*/true);
        getChassisData(sensorsAsyncResp);

        BMCWEB_LOG_DEBUG(
            "SensorCollection doGet exit via efficient expand handler");
        return;
    }

    // We get all sensors as hyperlinkes in the chassis (this
    // implies we reply on the default query parameters handler)
    getChassis(asyncResp, chassisId, sensors::sensorsNodeStr, dbus::sensorPaths,
               std::bind_front(sensors::getChassisCallback, asyncResp,
                               chassisId, sensors::sensorsNodeStr));
}

inline void getSensorFromDbus(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& sensorPath,
    const ::dbus::utility::MapperGetObject& mapperResponse)
{
    if (mapperResponse.size() != 1)
    {
        messages::internalError(asyncResp->res);
        return;
    }
    const auto& valueIface = *mapperResponse.begin();
    const std::string& connectionName = valueIface.first;
    BMCWEB_LOG_DEBUG("Looking up {}", connectionName);
    BMCWEB_LOG_DEBUG("Path {}", sensorPath);

    ::dbus::utility::getAllProperties(
        *crow::connections::systemBus, connectionName, sensorPath, "",
        [asyncResp,
         sensorPath](const boost::system::error_code& ec,
                     const ::dbus::utility::DBusPropertiesMap& valuesDict) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            sdbusplus::message::object_path path(sensorPath);
            std::string name = path.filename();
            path = path.parent_path();
            std::string type = path.filename();
            sensor_utils::objectPropertiesToJson(
                name, type, sensor_utils::ChassisSubNode::sensorsNode,
                valuesDict, asyncResp->res.jsonValue, nullptr);
        });
}

inline void handleSensorGet(App& app, const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& chassisId,
                            const std::string& sensorId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    std::pair<std::string, std::string> nameType =
        redfish::sensor_utils::splitSensorNameAndType(sensorId);
    if (nameType.first.empty() || nameType.second.empty())
    {
        messages::resourceNotFound(asyncResp->res, sensorId, "Sensor");
        return;
    }

    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/Sensors/{}", chassisId, sensorId);

    BMCWEB_LOG_DEBUG("Sensor doGet enter");

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Sensor.Value"};
    std::string sensorPath = "/xyz/openbmc_project/sensors/" + nameType.first +
                             '/' + nameType.second;
    // Get a list of all of the sensors that implement Sensor.Value
    // and get the path and service name associated with the sensor
    ::dbus::utility::getDbusObject(
        sensorPath, interfaces,
        [asyncResp, sensorId,
         sensorPath](const boost::system::error_code& ec,
                     const ::dbus::utility::MapperGetObject& subtree) {
            BMCWEB_LOG_DEBUG("respHandler1 enter");
            if (ec == boost::system::errc::io_error)
            {
                BMCWEB_LOG_WARNING("Sensor not found from getSensorPaths");
                messages::resourceNotFound(asyncResp->res, sensorId, "Sensor");
                return;
            }
            if (ec)
            {
                messages::internalError(asyncResp->res);
                BMCWEB_LOG_ERROR(
                    "Sensor getSensorPaths resp_handler: Dbus error {}", ec);
                return;
            }
            getSensorFromDbus(asyncResp, sensorPath, subtree);
            BMCWEB_LOG_DEBUG("respHandler1 exit");
        });
}

} // namespace sensors

inline void requestRoutesSensorCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Sensors/")
        .privileges(redfish::privileges::getSensorCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(sensors::handleSensorCollectionGet, std::ref(app)));
}

inline void requestRoutesSensor(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Sensors/<str>/")
        .privileges(redfish::privileges::getSensor)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(sensors::handleSensorGet, std::ref(app)));
}

} // namespace redfish
