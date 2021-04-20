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
    void doGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const crow::Request&,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);

            return;
        }
        const std::string& chassisName = params[0];
        auto sensorAsyncResp = std::make_shared<SensorsAsyncResp>(
            asyncResp, chassisName,
            sensors::dbus::paths.at(sensors::node::thermal),
            sensors::node::thermal);

        // TODO Need to get Chassis Redundancy information.
        getChassisData(sensorAsyncResp);
    }
    void doPatch(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                 const crow::Request& req,
                 const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {

            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& chassisName = params[0];
        std::optional<std::vector<nlohmann::json>> temperatureCollections;
        std::optional<std::vector<nlohmann::json>> fanCollections;
        std::unordered_map<std::string, std::vector<nlohmann::json>>
            allCollections;

        auto sensorsAsyncResp = std::make_shared<SensorsAsyncResp>(
            asyncResp, chassisName,
            sensors::dbus::paths.at(sensors::node::thermal),
            sensors::node::thermal);

        if (!json_util::readJson(req, sensorsAsyncResp->asyncResp->res,
                                 "Temperatures", temperatureCollections, "Fans",
                                 fanCollections))
        {
            return;
        }
        if (!temperatureCollections && !fanCollections)
        {
            messages::resourceNotFound(sensorsAsyncResp->asyncResp->res,
                                       "Thermal", "Temperatures / Voltages");
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
    }
    void doPost(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const crow::Request& req,
                const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {

            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& chassisName = params[0];
        auto sensorsAsyncResp = std::make_shared<SensorsAsyncResp>(
            asyncResp, chassisName,
            sensors::dbus::paths.at(sensors::node::thermal),
            sensors::node::thermal);
        std::string fanMode;
        if (!json_util::readJson(req, sensorsAsyncResp->asyncResp->res, "FanMode", fanMode))
        {
            messages::actionParameterUnknown(sensorsAsyncResp->asyncResp->res, "FanMode", "");
            return;
        }

        bool munualMode;
        if (fanMode == "Manual")
        {
            munualMode = true;
        }
        else if (fanMode == "Auto")
        {
            munualMode = false;
        }
        else
        {
            messages::actionParameterValueFormatError(asyncResp->res, "Manual", "Auto",
                                                      "");
            return;
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp{std::move(asyncResp)}, munualMode](
                const boost::system::error_code ec,
                const std::vector<std::pair<
                    std::string, std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>>&
                    subtree) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "fanctrl GetSubTree failed "
                                     << " ec = ( " << ec << " )\n";
                    messages::internalError(asyncResp->res);
                    return;
                }

                for (const auto& object : subtree)
                {
                    auto iter = object.first.rfind('/');
                    if ((iter != std::string::npos) &&
                        (iter < object.first.size()))
                    {
                        std::string objName = object.first.substr(iter + 1);
                        crow::connections::systemBus->async_method_call(
                            [asyncResp, objName,
                             munualMode](const boost::system::error_code ec) {
                                if (ec)
                                {
                                    BMCWEB_LOG_ERROR
                                        << "Updated the Mode failed " << objName
                                        << ": " << munualMode << " ec = ( "
                                        << ec << " )\n";
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                            },
                            "xyz.openbmc_project.State.FanCtrl",
                            "/xyz/openbmc_project/settings/fanctrl/" + objName,
                            "org.freedesktop.DBus.Properties", "Set",
                            "xyz.openbmc_project.Control.Mode", "Manual",
                            std::variant<bool>(munualMode));
                        BMCWEB_LOG_DEBUG << "Updated the Mode success "
                                         << objName << ": " << munualMode;
                    }
                }
                messages::success(asyncResp->res);
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
            "/xyz/openbmc_project/settings/fanctrl", 1,
            std::array<const char*, 1>{"xyz.openbmc_project.Control.Mode"});
    }
};

} // namespace redfish
