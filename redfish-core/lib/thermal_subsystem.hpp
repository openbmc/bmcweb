// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "human_sort.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>
#include <dbus_utility.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace redfish
{
inline void getFanRedundancy(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    constexpr std::array<std::string_view, 1> interfaces{
        "xyz.openbmc_project.Sensor.Value"};

    const std::string& fantachPath = "/xyz/openbmc_project/sensors/fan_tach";

    asyncResp->res.jsonValue["FanRedundancy"] = nlohmann::json::array();

    dbus::utility::getSubTree(
        fantachPath, 0, interfaces,
        [chassisId,
         asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG("DBUS response error {}", ec.value());
            messages::internalError(asyncResp->res);
            return;
        }

        std::vector<std::string> pathNames;

        for (const auto& [fanPath, serviceMap] : subtree)
        {
            for (const auto& [service, interfacesList] : serviceMap)
            {
                if ((service == "xyz.openbmc_project.FanSensor") &&
                    (!interfacesList.empty()))
                {
                    sdbusplus::message::object_path path(fanPath);
                    std::string leaf = path.filename();
                    if (leaf.empty())
                    {
                        continue;
                    }
                    pathNames.push_back(leaf);
                }
            }
        }
        if (pathNames.empty())
        {
            return;
        }

        nlohmann::json::object_t redundandy;
        redundandy["RedundancyType"] = "NPlusM";
        redundandy["MinNeededInGroup"] = 1;
        redundandy["Status"]["State"] = "Enabled";
        redundandy["Status"]["Health"] = "OK";
        redundandy["RedundancyGroup"] = nlohmann::json::array();

        std::sort(pathNames.begin(), pathNames.end(),
                  AlphanumLess<std::string>());

        for (const std::string& leaf : pathNames)
        {
            boost::urls::url url = boost::urls::format(
                "/redfish/v1/Chassis/{}/ThermalSubsystem/Fans", chassisId);
            crow::utility::appendUrlPieces(url, leaf);
            nlohmann::json::object_t member;
            member["@odata.id"] = std::move(url);
            redundandy["RedundancyGroup"].push_back(std::move(member));
        }
        redundandy["MaxSupportedInGroup"] =
            redundandy["RedundancyGroup"].size();

        asyncResp->res.jsonValue["FanRedundancy"].push_back(
            std::move(redundandy));
        });
}


inline void doThermalSubsystemCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId,
    const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        BMCWEB_LOG_WARNING("Not a valid chassis ID{}", chassisId);
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ThermalSubsystem/ThermalSubsystem.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] =
        "#ThermalSubsystem.v1_0_0.ThermalSubsystem";
    asyncResp->res.jsonValue["Name"] = "Thermal Subsystem";
    asyncResp->res.jsonValue["Id"] = "ThermalSubsystem";

    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/ThermalSubsystem", chassisId);

    asyncResp->res.jsonValue["Fans"]["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/ThermalSubsystem/Fans", chassisId);

    asyncResp->res.jsonValue["ThermalMetrics"]["@odata.id"] =
        boost::urls::format(
            "/redfish/v1/Chassis/{}/ThermalSubsystem/ThermalMetrics",
            chassisId);

    asyncResp->res.jsonValue["Status"]["State"] = resource::State::Enabled;
    asyncResp->res.jsonValue["Status"]["Health"] = resource::Health::OK;
    getFanRedundancy(asyncResp, chassisId);
}

inline void handleThermalSubsystemCollectionHead(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    auto respHandler = [asyncResp, chassisId](
                           const std::optional<std::string>& validChassisPath) {
        if (!validChassisPath)
        {
            messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
            return;
        }
        asyncResp->res.addHeader(
            boost::beast::http::field::link,
            "</redfish/v1/JsonSchemas/ThermalSubsystem/ThermalSubsystem.json>; rel=describedby");
    };
    redfish::chassis_utils::getValidChassisPath(asyncResp, chassisId,
                                                std::bind_front(respHandler));
}

inline void handleThermalSubsystemCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        std::bind_front(doThermalSubsystemCollection, asyncResp, chassisId));
}

inline void requestRoutesThermalSubsystem(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/")
        .privileges(redfish::privileges::headThermalSubsystem)
        .methods(boost::beast::http::verb::head)(std::bind_front(
            handleThermalSubsystemCollectionHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/")
        .privileges(redfish::privileges::getThermalSubsystem)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleThermalSubsystemCollectionGet, std::ref(app)));
}

} // namespace redfish
