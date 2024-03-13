// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2018 Intel Corporation
// SPDX-FileCopyrightText: Copyright 2018 Ampere Computing LLC

#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/power.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "sensors.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"
#include "utils/sensor_utils.hpp"

#include <boost/beast/http/verb.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace redfish
{

inline void afterGetPowerCapEnable(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
    uint32_t valueToSet, const boost::system::error_code& ec,
    bool powerCapEnable)
{
    if (ec)
    {
        messages::internalError(sensorsAsyncResp->asyncResp->res);
        BMCWEB_LOG_ERROR("powerCapEnable Get handler: Dbus error {}", ec);
        return;
    }
    if (!powerCapEnable)
    {
        messages::actionNotSupported(
            sensorsAsyncResp->asyncResp->res,
            "Setting LimitInWatts when PowerLimit feature is disabled");
        BMCWEB_LOG_ERROR("PowerLimit feature is disabled ");
        return;
    }

    setDbusProperty(sensorsAsyncResp->asyncResp, "PowerControl",
                    "xyz.openbmc_project.Settings",
                    sdbusplus::message::object_path(
                        "/xyz/openbmc_project/control/host0/power_cap"),
                    "xyz.openbmc_project.Control.Power.Cap", "PowerCap",
                    valueToSet);
}

inline void afterGetChassisPath(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
    std::vector<nlohmann::json::object_t>& powerControlCollections,
    const std::optional<std::string>& chassisPath)
{
    if (!chassisPath)
    {
        BMCWEB_LOG_WARNING("Don't find valid chassis path ");
        messages::resourceNotFound(sensorsAsyncResp->asyncResp->res, "Chassis",
                                   sensorsAsyncResp->chassisId);
        return;
    }

    if (powerControlCollections.size() != 1)
    {
        BMCWEB_LOG_WARNING("Don't support multiple hosts at present ");
        messages::resourceNotFound(sensorsAsyncResp->asyncResp->res, "Power",
                                   "PowerControl");
        return;
    }

    auto& item = powerControlCollections[0];

    std::optional<uint32_t> value;
    if (!json_util::readJsonObject(                 //
            item, sensorsAsyncResp->asyncResp->res, //
            "PowerLimit/LimitInWatts", value        //
            ))
    {
        return;
    }
    if (!value)
    {
        return;
    }
    dbus::utility::getProperty<bool>(
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/power_cap",
        "xyz.openbmc_project.Control.Power.Cap", "PowerCapEnable",
        std::bind_front(afterGetPowerCapEnable, sensorsAsyncResp, *value));
}

inline void afterPowerCapSettingGet(
    const std::shared_ptr<SensorsAsyncResp>& sensorAsyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        messages::internalError(sensorAsyncResp->asyncResp->res);
        BMCWEB_LOG_ERROR("Power Limit GetAll handler: Dbus error {}", ec);
        return;
    }

    nlohmann::json& tempArray =
        sensorAsyncResp->asyncResp->res.jsonValue["PowerControl"];

    // Put multiple "sensors" into a single PowerControl, 0,
    // so only create the first one
    if (tempArray.empty())
    {
        // Mandatory properties odata.id and MemberId
        // A warning without a odata.type
        nlohmann::json::object_t powerControl;
        powerControl["@odata.type"] = "#Power.v1_0_0.PowerControl";
        powerControl["@odata.id"] =
            "/redfish/v1/Chassis/" + sensorAsyncResp->chassisId +
            "/Power#/PowerControl/0";
        powerControl["Name"] = "Chassis Power Control";
        powerControl["MemberId"] = "0";
        tempArray.emplace_back(std::move(powerControl));
    }

    nlohmann::json& sensorJson = tempArray.back();
    bool enabled = false;
    double powerCap = 0.0;
    int64_t scale = 0;

    for (const std::pair<std::string, dbus::utility::DbusVariantType>&
             property : properties)
    {
        if (property.first == "Scale")
        {
            const int64_t* i = std::get_if<int64_t>(&property.second);

            if (i != nullptr)
            {
                scale = *i;
            }
        }
        else if (property.first == "PowerCap")
        {
            const double* d = std::get_if<double>(&property.second);
            const int64_t* i = std::get_if<int64_t>(&property.second);
            const uint32_t* u = std::get_if<uint32_t>(&property.second);

            if (d != nullptr)
            {
                powerCap = *d;
            }
            else if (i != nullptr)
            {
                powerCap = static_cast<double>(*i);
            }
            else if (u != nullptr)
            {
                powerCap = *u;
            }
        }
        else if (property.first == "PowerCapEnable")
        {
            const bool* b = std::get_if<bool>(&property.second);

            if (b != nullptr)
            {
                enabled = *b;
            }
        }
    }

    // LimitException is Mandatory attribute as per OCP
    // Baseline Profile - v1.0.0, so currently making it
    // "NoAction" as default value to make it OCP Compliant.
    sensorJson["PowerLimit"]["LimitException"] =
        power::PowerLimitException::NoAction;

    if (enabled)
    {
        // Redfish specification indicates PowerLimit should
        // be null if the limit is not enabled.
        sensorJson["PowerLimit"]["LimitInWatts"] =
            powerCap * std::pow(10, scale);
    }
}

using Mapper = dbus::utility::MapperGetSubTreePathsResponse;
inline void afterGetChassis(
    const std::shared_ptr<SensorsAsyncResp>& sensorAsyncResp,
    const boost::system::error_code& ec2, const Mapper& chassisPaths)
{
    if (ec2)
    {
        BMCWEB_LOG_ERROR("Power Limit GetSubTreePaths handler Dbus error {}",
                         ec2);
        return;
    }

    bool found = false;
    for (const std::string& chassis : chassisPaths)
    {
        size_t len = std::string::npos;
        size_t lastPos = chassis.rfind('/');
        if (lastPos == std::string::npos)
        {
            continue;
        }

        if (lastPos == chassis.size() - 1)
        {
            size_t end = lastPos;
            lastPos = chassis.rfind('/', lastPos - 1);
            if (lastPos == std::string::npos)
            {
                continue;
            }

            len = end - (lastPos + 1);
        }

        std::string interfaceChassisName = chassis.substr(lastPos + 1, len);
        if (interfaceChassisName == sensorAsyncResp->chassisId)
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        BMCWEB_LOG_DEBUG("Power Limit not present for {}",
                         sensorAsyncResp->chassisId);
        return;
    }

    dbus::utility::getAllProperties(
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/power_cap",
        "xyz.openbmc_project.Control.Power.Cap",
        [sensorAsyncResp](const boost::system::error_code& ec,
                          const dbus::utility::DBusPropertiesMap& properties

        ) { afterPowerCapSettingGet(sensorAsyncResp, ec, properties); });
}

inline void handleChassisPowerGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.jsonValue["PowerControl"] = nlohmann::json::array();

    auto sensorAsyncResp = std::make_shared<SensorsAsyncResp>(
        asyncResp, chassisName, sensors::dbus::powerPaths,
        sensor_utils::chassisSubNodeToString(
            sensor_utils::ChassisSubNode::powerNode));

    getChassisData(sensorAsyncResp);

    // This callback verifies that the power limit is only provided
    // for the chassis that implements the Chassis inventory item.
    // This prevents things like power supplies providing the
    // chassis power limit

    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, chassisInterfaces,
        std::bind_front(afterGetChassis, sensorAsyncResp));
}

inline void handleChassisPowerPatch(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    auto sensorAsyncResp = std::make_shared<SensorsAsyncResp>(
        asyncResp, chassisName, sensors::dbus::powerPaths,
        sensor_utils::chassisSubNodeToString(
            sensor_utils::ChassisSubNode::powerNode));

    std::optional<std::vector<nlohmann::json::object_t>> voltageCollections;
    std::optional<std::vector<nlohmann::json::object_t>> powerCtlCollections;

    if (!json_util::readJsonPatch(                //
            req, sensorAsyncResp->asyncResp->res, //
            "PowerControl", powerCtlCollections,  //
            "Voltages", voltageCollections        //
            ))
    {
        return;
    }

    if (powerCtlCollections)
    {
        redfish::chassis_utils::getValidChassisPath(
            sensorAsyncResp->asyncResp, sensorAsyncResp->chassisId,
            std::bind_front(afterGetChassisPath, sensorAsyncResp,
                            *powerCtlCollections));
    }
    if (voltageCollections)
    {
        std::unordered_map<std::string, std::vector<nlohmann::json::object_t>>
            allCollections;
        allCollections.emplace("Voltages", std::move(*voltageCollections));
        setSensorsOverride(sensorAsyncResp, allCollections);
    }
}

inline void requestRoutesPower(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Power/")
        .privileges(redfish::privileges::getPower)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleChassisPowerGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Power/")
        .privileges(redfish::privileges::patchPower)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handleChassisPowerPatch, std::ref(app)));
}

} // namespace redfish
