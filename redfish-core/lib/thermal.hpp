/*
// Copyright (c) 2018 Intel Corporation
//
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

#include "sensors.hpp"

#include <app.hpp>
#include <registries/privilege_registry.hpp>

namespace redfish
{

inline void requestRoutesThermal(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Thermal/")
        .privileges(redfish::privileges::getThermal)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisName) {
                auto thermalPaths =
                    sensors::dbus::paths.find(sensors::node::thermal);
                if (thermalPaths == sensors::dbus::paths.end())
                {
                    messages::internalError(asyncResp->res);
                    return;
                }

                auto sensorAsyncResp = std::make_shared<SensorsAsyncResp>(
                    asyncResp, chassisName, thermalPaths->second,
                    sensors::node::thermal);

                // TODO Need to get Chassis Redundancy information.
                getChassisData(sensorAsyncResp);
            });

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Thermal/")
        .privileges(redfish::privileges::patchThermal)
        .methods(boost::beast::http::verb::patch)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisName) {
                auto thermalPaths =
                    sensors::dbus::paths.find(sensors::node::thermal);
                if (thermalPaths == sensors::dbus::paths.end())
                {
                    messages::internalError(asyncResp->res);
                    return;
                }

                std::optional<std::vector<nlohmann::json>>
                    temperatureCollections;
                std::optional<std::vector<nlohmann::json>> fanCollections;
                std::unordered_map<std::string, std::vector<nlohmann::json>>
                    allCollections;

                auto sensorsAsyncResp = std::make_shared<SensorsAsyncResp>(
                    asyncResp, chassisName, thermalPaths->second,
                    sensors::node::thermal);

                if (!json_util::readJson(req, sensorsAsyncResp->asyncResp->res,
                                         "Temperatures", temperatureCollections,
                                         "Fans", fanCollections))
                {
                    return;
                }
                if (!temperatureCollections && !fanCollections)
                {
                    messages::resourceNotFound(sensorsAsyncResp->asyncResp->res,
                                               "Thermal",
                                               "Temperatures / Voltages");
                    return;
                }
                if (temperatureCollections)
                {
                    allCollections.emplace("Temperatures",
                                           *std::move(temperatureCollections));
                }
                if (fanCollections)
                {
                    allCollections.emplace("Fans", *std::move(fanCollections));
                }
                setSensorsOverride(sensorsAsyncResp, allCollections);
            });
}

inline void getMemoryThermalData(std::shared_ptr<bmcweb::AsyncResp> aResp, 
                                const std::string& dimmId)
{
    BMCWEB_LOG_DEBUG << "Get available system dimm resources.";
    crow::connections::systemBus->async_method_call(
        [dimmId, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const boost::container::flat_map<
                std::string, boost::container::flat_map<
                                 std::string, std::vector<std::string>>>&
                subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);

                return;
            }
            bool found = false;
            for (const auto& [path, object] : subtree)
            {
                if (path.find(dimmId) != std::string::npos)
                {
                    for (const auto& [service, interfaces] : object)
                    {
                        if (!found &&
                            (std::find(
                                 interfaces.begin(), interfaces.end(),
                                 "xyz.openbmc_project.Inventory.Item.Dimm") !=
                             interfaces.end()))
                        {
                            BMCWEB_LOG_DEBUG
                                << "Get available system components.";
                            crow::connections::systemBus->async_method_call(
                                [dimmId, aResp{std::move(aResp)}](
                                    const boost::system::error_code ec,
                                    const DimmProperties& properties) {
                                    if (ec)
                                    {
                                        BMCWEB_LOG_DEBUG
                                            << "DBUS response error";
                                        messages::internalError(aResp->res);

                                        return;
                                    }
                                    for (const auto& property : properties)
                                    {
                                        if (property.first ==
                                            "TemperatureSummary")
                                        {
                                            auto* value = std::get_if<
                                                std::map<std::string, int>>(
                                                &property.second);
                                            if (value == nullptr)
                                            {
                                                continue;
                                            }
                                            aResp->res.jsonValue
                                                ["TemperatureSummary"] = *value;
                                        }
                                    }
                                },
                                service, path,
                                "org.freedesktop.DBus.Properties", "GetAll",
                                "");

                            found = true;
                        }
                    }
                }
            }
            // Object not found
            if (!found)
            {
                messages::resourceNotFound(aResp->res, "Memory", dimmId);
            }
            return;
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0,
        std::array<const char*, 2>{
            "xyz.openbmc_project.Inventory.Item.Dimm",
            "xyz.openbmc_project.Inventory.Item.PersistentMemory.Partition"});
}

inline void requestRoutesMemoryThermal(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/system/Memory/<str>/Chassis/ThermalMetrics")
        .privileges(redfish::privileges::getMemoryThermal)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& dimmId) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#ThermalMetrics.v1_0_0.ThermalMetrics";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Systems/system/Memory/" + dimmId +
                    "/Chassis/ThermalMetrics";
                asyncResp->res.jsonValue["Name"] =
                    "Thermal Metrics for Chassis";
                asyncResp->res.jsonValue["Description"] =
                    "Thermal Metrics for Chassis";
                asyncResp->res.jsonValue["Id"] =
                    "Thermal Metrics for Chassis for " + dimmId;

                getMemoryThermalData(asyncResp, dimmId);
            });
}

} // namespace redfish
