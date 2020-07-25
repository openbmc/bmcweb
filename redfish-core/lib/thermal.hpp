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

#include "node.hpp"
#include "sensors.hpp"

namespace redfish
{

class Thermal : public Node
{
  public:
    Thermal(App& app) :
        Node((app), "/redfish/v1/Chassis/<str>/Thermal/", std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            messages::internalError(res);
            res.end();
            return;
        }
        const std::string& chassisName = params[0];
        auto sensorAsyncResp = std::make_shared<SensorsAsyncResp>(
            res, chassisName, sensors::dbus::types.at(sensors::node::thermal),
            sensors::node::thermal);

        // TODO Need to get Chassis Redundancy information.
        getChassisData(sensorAsyncResp);
    }
    void doPatch(crow::Response& res, const crow::Request& req,
                 const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            res.end();
            messages::internalError(res);
            return;
        }

        const std::string& chassisName = params[0];
        std::optional<std::vector<nlohmann::json>> temperatureCollections;
        std::optional<std::vector<nlohmann::json>> fanCollections;
        std::unordered_map<std::string, std::vector<nlohmann::json>>
            allCollections;

        auto asyncResp = std::make_shared<SensorsAsyncResp>(
            res, chassisName, sensors::dbus::types.at(sensors::node::thermal),
            sensors::node::thermal);

        if (!json_util::readJson(req, asyncResp->res, "Temperatures",
                                 temperatureCollections, "Fans",
                                 fanCollections))
        {
            return;
        }
        if (!temperatureCollections && !fanCollections)
        {
            messages::resourceNotFound(asyncResp->res, "Thermal",
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

        checkAndDoSensorsOverride(asyncResp, allCollections);
    }
};

} // namespace redfish
