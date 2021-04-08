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

namespace redfish
{

inline void requestRoutesThermal(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Thermal/")
        .privileges({"Login"})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisName) {
                auto sensorAsyncResp = std::make_shared<SensorsAsyncResp>(
                    asyncResp, chassisName,
                    sensors::dbus::paths.at(sensors::node::thermal),
                    sensors::node::thermal);

                // TODO Need to get Chassis Redundancy information.
                getChassisData(sensorAsyncResp);
            });

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Thermal/")
        .privileges({"ConfigureManager"})
        .methods(boost::beast::http::verb::patch)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisName) {
                std::optional<std::vector<nlohmann::json>>
                    temperatureCollections;
                std::optional<std::vector<nlohmann::json>> fanCollections;
                std::unordered_map<std::string, std::vector<nlohmann::json>>
                    allCollections;

                auto sensorsAsyncResp = std::make_shared<SensorsAsyncResp>(
                    asyncResp, chassisName,
                    sensors::dbus::paths.at(sensors::node::thermal),
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

                checkAndDoSensorsOverride(sensorsAsyncResp, allCollections);
            });
}

} // namespace redfish
