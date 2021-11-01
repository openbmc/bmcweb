/*
// Copyright (c) 2018 Intel Corporation
// Copyright (c) 2018 Ampere Computing LLC
/
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "sensors.hpp"
#include "utils/chassis_utils.hpp"

#include <sdbusplus/asio/property.hpp>

#include <array>
#include <string_view>

namespace redfish
{
inline void setPowerCapOverride(
    const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp,
    std::vector<nlohmann::json>& powerControlCollections)
{
    auto getChassisPath =
        [sensorsAsyncResp, powerControlCollections](
            const std::optional<std::string>& chassisPath) mutable {
        if (!chassisPath)
        {
            BMCWEB_LOG_ERROR << "Don't find valid chassis path ";
            messages::resourceNotFound(sensorsAsyncResp->asyncResp->res,
                                       "Chassis", sensorsAsyncResp->chassisId);
            return;
        }

        if (powerControlCollections.size() != 1)
        {
            BMCWEB_LOG_ERROR << "Don't support multiple hosts at present ";
            messages::resourceNotFound(sensorsAsyncResp->asyncResp->res,
                                       "Power", "PowerControl");
            return;
        }

        auto& item = powerControlCollections[0];

        std::optional<nlohmann::json> powerLimit;
        if (!json_util::readJson(item, sensorsAsyncResp->asyncResp->res,
                                 "PowerLimit", powerLimit))
        {
            return;
        }
        if (!powerLimit)
        {
            return;
        }
        std::optional<uint32_t> value;
        if (!json_util::readJson(*powerLimit, sensorsAsyncResp->asyncResp->res,
                                 "LimitInWatts", value))
        {
            return;
        }
        if (!value)
        {
            return;
        }
        sdbusplus::asio::getProperty<bool>(
            *crow::connections::systemBus, "xyz.openbmc_project.Settings",
            "/xyz/openbmc_project/control/host0/power_cap",
            "xyz.openbmc_project.Control.Power.Cap", "PowerCapEnable",
            [value, sensorsAsyncResp](const boost::system::error_code ec,
                                      bool powerCapEnable) {
            if (ec)
            {
                messages::internalError(sensorsAsyncResp->asyncResp->res);
                BMCWEB_LOG_ERROR << "powerCapEnable Get handler: Dbus error "
                                 << ec;
                return;
            }
            if (!powerCapEnable)
            {
                messages::actionNotSupported(
                    sensorsAsyncResp->asyncResp->res,
                    "Setting LimitInWatts when PowerLimit feature is disabled");
                BMCWEB_LOG_ERROR << "PowerLimit feature is disabled ";
                return;
            }

            crow::connections::systemBus->async_method_call(
                [sensorsAsyncResp](const boost::system::error_code ec2) {
                if (ec2)
                {
                    BMCWEB_LOG_DEBUG << "Power Limit Set: Dbus error: " << ec2;
                    messages::internalError(sensorsAsyncResp->asyncResp->res);
                    return;
                }
                sensorsAsyncResp->asyncResp->res.result(
                    boost::beast::http::status::no_content);
                },
                "xyz.openbmc_project.Settings",
                "/xyz/openbmc_project/control/host0/power_cap",
                "org.freedesktop.DBus.Properties", "Set",
                "xyz.openbmc_project.Control.Power.Cap", "PowerCap",
                std::variant<uint32_t>(*value));
            });
    };
    redfish::chassis_utils::getValidChassisPath(sensorsAsyncResp->asyncResp,
                                                sensorsAsyncResp->chassisId,
                                                std::move(getChassisPath));
}
inline void requestRoutesPower(App& app)
{

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Power/")
        .privileges(redfish::privileges::getPower)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& chassisName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        asyncResp->res.jsonValue["PowerControl"] = nlohmann::json::array();

        auto sensorAsyncResp = std::make_shared<SensorsAsyncResp>(
            asyncResp, chassisName, sensors::dbus::powerPaths,
            sensors::node::power);

        getChassisData(sensorAsyncResp);

        // This callback verifies that the power limit is only provided
        // for the chassis that implements the Chassis inventory item.
        // This prevents things like power supplies providing the
        // chassis power limit

        using Mapper = dbus::utility::MapperGetSubTreePathsResponse;
        auto chassisHandler =
            [sensorAsyncResp](const boost::system::error_code& e,
                              const Mapper& chassisPaths) {
            if (e)
            {
                BMCWEB_LOG_ERROR
                    << "Power Limit GetSubTreePaths handler Dbus error " << e;
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

                std::string interfaceChassisName =
                    chassis.substr(lastPos + 1, len);
                if (interfaceChassisName == sensorAsyncResp->chassisId)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                BMCWEB_LOG_DEBUG << "Power Limit not present for "
                                 << sensorAsyncResp->chassisId;
                return;
            }

            auto valueHandler =
                [sensorAsyncResp](
                    const boost::system::error_code ec,
                    const dbus::utility::DBusPropertiesMap& properties) {
                if (ec)
                {
                    messages::internalError(sensorAsyncResp->asyncResp->res);
                    BMCWEB_LOG_ERROR
                        << "Power Limit GetAll handler: Dbus error " << ec;
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
                    powerControl["@odata.id"] = "/redfish/v1/Chassis/" +
                                                sensorAsyncResp->chassisId +
                                                "/Power#/PowerControl/0";
                    powerControl["Name"] = "Chassis Power Control";
                    powerControl["MemberId"] = "0";
                    tempArray.push_back(std::move(powerControl));
                }

                nlohmann::json& sensorJson = tempArray.back();
                bool enabled = false;
                double powerCap = 0.0;
                int64_t scale = 0;

                for (const std::pair<std::string,
                                     dbus::utility::DbusVariantType>& property :
                     properties)
                {
                    if (property.first == "Scale")
                    {
                        const int64_t* i =
                            std::get_if<int64_t>(&property.second);

                        if (i != nullptr)
                        {
                            scale = *i;
                        }
                    }
                    else if (property.first == "PowerCap")
                    {
                        const double* d = std::get_if<double>(&property.second);
                        const int64_t* i =
                            std::get_if<int64_t>(&property.second);
                        const uint32_t* u =
                            std::get_if<uint32_t>(&property.second);

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

                nlohmann::json& value =
                    sensorJson["PowerLimit"]["LimitInWatts"];

                // LimitException is Mandatory attribute as per OCP
                // Baseline Profile – v1.0.0, so currently making it
                // "NoAction" as default value to make it OCP Compliant.
                sensorJson["PowerLimit"]["LimitException"] = "NoAction";

                if (enabled)
                {
                    // Redfish specification indicates PowerLimit should
                    // be null if the limit is not enabled.
                    value = powerCap * std::pow(10, scale);
                }
            };

            sdbusplus::asio::getAllProperties(
                *crow::connections::systemBus, "xyz.openbmc_project.Settings",
                "/xyz/openbmc_project/control/host0/power_cap",
                "xyz.openbmc_project.Control.Power.Cap",
                std::move(valueHandler));
        };

        getPowerMetricData(sensorAsyncResp);

        constexpr std::array<std::string_view, 2> interfaces = {
            "xyz.openbmc_project.Inventory.Item.Board",
            "xyz.openbmc_project.Inventory.Item.Chassis"};

        dbus::utility::getSubTreePaths("/xyz/openbmc_project/inventory", 0,
                                       interfaces, std::move(chassisHandler));
        });

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Power/")
        .privileges(redfish::privileges::patchPower)
        .methods(boost::beast::http::verb::patch)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& chassisName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        auto sensorAsyncResp = std::make_shared<SensorsAsyncResp>(
            asyncResp, chassisName, sensors::dbus::powerPaths,
            sensors::node::power);

        std::optional<std::vector<nlohmann::json>> voltageCollections;
        std::optional<std::vector<nlohmann::json>> powerCtlCollections;

        if (!json_util::readJsonPatch(req, sensorAsyncResp->asyncResp->res,
                                      "PowerControl", powerCtlCollections,
                                      "Voltages", voltageCollections))
        {
            return;
        }

        if (powerCtlCollections)
        {
            setPowerCapOverride(sensorAsyncResp, *powerCtlCollections);
        }
        if (voltageCollections)
        {
            std::unordered_map<std::string, std::vector<nlohmann::json>>
                allCollections;
            allCollections.emplace("Voltages", *std::move(voltageCollections));
            setSensorsOverride(sensorAsyncResp, allCollections);
        }
        });
}

} // namespace redfish
