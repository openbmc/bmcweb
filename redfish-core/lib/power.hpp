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

#include "node.hpp"
#include "sensors.hpp"

namespace redfish
{

class Power : public Node
{
  public:
    Power(App& app) :
        Node((app), "/redfish/v1/Chassis/<str>/Power/", std::string())
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
    void setPowerCapOverride(
        const std::shared_ptr<SensorsAsyncResp>& asyncResp,
        std::vector<nlohmann::json>& powerControlCollections)
    {
        auto getChassisPath =
            [asyncResp, powerControlCollections](
                const std::optional<std::string>& chassisPath) mutable {
                if (!chassisPath)
                {
                    BMCWEB_LOG_ERROR << "Don't find valid chassis path ";
                    messages::resourceNotFound(asyncResp->res, "Chassis",
                                               asyncResp->chassisId);
                    return;
                }

                if (powerControlCollections.size() != 1)
                {
                    BMCWEB_LOG_ERROR
                        << "Don't support multiple hosts at present ";
                    messages::resourceNotFound(asyncResp->res, "Power",
                                               "PowerControl");
                    return;
                }

                auto& item = powerControlCollections[0];

                std::optional<nlohmann::json> powerLimit;
                if (!json_util::readJson(item, asyncResp->res, "PowerLimit",
                                         powerLimit))
                {
                    return;
                }
                if (!powerLimit)
                {
                    return;
                }
                std::optional<uint32_t> value;
                if (!json_util::readJson(*powerLimit, asyncResp->res,
                                         "LimitInWatts", value))
                {
                    return;
                }
                if (!value)
                {
                    return;
                }
                auto valueHandler = [value, asyncResp](
                                        const boost::system::error_code ec,
                                        const SensorVariant& powerCapEnable) {
                    if (ec)
                    {
                        messages::internalError(asyncResp->res);
                        BMCWEB_LOG_ERROR
                            << "powerCapEnable Get handler: Dbus error " << ec;
                        return;
                    }
                    // Check PowerCapEnable
                    const bool* b = std::get_if<bool>(&powerCapEnable);
                    if (b == nullptr)
                    {
                        messages::internalError(asyncResp->res);
                        BMCWEB_LOG_ERROR
                            << "Fail to get PowerCapEnable status ";
                        return;
                    }
                    if (!(*b))
                    {
                        messages::actionNotSupported(
                            asyncResp->res,
                            "Setting LimitInWatts when PowerLimit "
                            "feature is disabled");
                        BMCWEB_LOG_ERROR << "PowerLimit feature is disabled ";
                        return;
                    }

                    crow::connections::systemBus->async_method_call(
                        [asyncResp](const boost::system::error_code ec2) {
                            if (ec2)
                            {
                                BMCWEB_LOG_DEBUG
                                    << "Power Limit Set: Dbus error: " << ec2;
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            asyncResp->res.result(
                                boost::beast::http::status::no_content);
                        },
                        "xyz.openbmc_project.Settings",
                        "/xyz/openbmc_project/control/host0/power_cap",
                        "org.freedesktop.DBus.Properties", "Set",
                        "xyz.openbmc_project.Control.Power.Cap", "PowerCap",
                        std::variant<uint32_t>(*value));
                };
                crow::connections::systemBus->async_method_call(
                    std::move(valueHandler), "xyz.openbmc_project.Settings",
                    "/xyz/openbmc_project/control/host0/power_cap",
                    "org.freedesktop.DBus.Properties", "Get",
                    "xyz.openbmc_project.Control.Power.Cap", "PowerCapEnable");
            };
        getValidChassisPath(asyncResp, std::move(getChassisPath));
    }
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            res.result(boost::beast::http::status::internal_server_error);
            res.end();
            return;
        }
        const std::string& chassisName = params[0];

        res.jsonValue["PowerControl"] = nlohmann::json::array();

        auto sensorAsyncResp = std::make_shared<SensorsAsyncResp>(
            res, chassisName, sensors::dbus::types.at(sensors::node::power),
            sensors::node::power);

        getChassisData(sensorAsyncResp);

        // This callback verifies that the power limit is only provided for the
        // chassis that implements the Chassis inventory item. This prevents
        // things like power supplies providing the chassis power limit
        auto chassisHandler = [sensorAsyncResp](
                                  const boost::system::error_code e,
                                  const std::vector<std::string>&
                                      chassisPaths) {
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
                if (!interfaceChassisName.compare(sensorAsyncResp->chassisId))
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
                    const std::vector<std::pair<std::string, SensorVariant>>&
                        properties) {
                    if (ec)
                    {
                        messages::internalError(sensorAsyncResp->res);
                        BMCWEB_LOG_ERROR
                            << "Power Limit GetAll handler: Dbus error " << ec;
                        return;
                    }

                    nlohmann::json& tempArray =
                        sensorAsyncResp->res.jsonValue["PowerControl"];

                    // Put multiple "sensors" into a single PowerControl, 0, so
                    // only create the first one
                    if (tempArray.empty())
                    {
                        // Mandatory properties odata.id and MemberId
                        // A warning without a odata.type
                        tempArray.push_back(
                            {{"@odata.type", "#Power.v1_0_0.PowerControl"},
                             {"@odata.id", "/redfish/v1/Chassis/" +
                                               sensorAsyncResp->chassisId +
                                               "/Power#/PowerControl/0"},
                             {"Name", "Chassis Power Control"},
                             {"MemberId", "0"}});
                    }

                    nlohmann::json& sensorJson = tempArray.back();
                    bool enabled = false;
                    double powerCap = 0.0;
                    int64_t scale = 0;

                    for (const std::pair<std::string, SensorVariant>& property :
                         properties)
                    {
                        if (!property.first.compare("Scale"))
                        {
                            const int64_t* i =
                                std::get_if<int64_t>(&property.second);

                            if (i)
                            {
                                scale = *i;
                            }
                        }
                        else if (!property.first.compare("PowerCap"))
                        {
                            const double* d =
                                std::get_if<double>(&property.second);
                            const int64_t* i =
                                std::get_if<int64_t>(&property.second);
                            const uint32_t* u =
                                std::get_if<uint32_t>(&property.second);

                            if (d)
                            {
                                powerCap = *d;
                            }
                            else if (i)
                            {
                                powerCap = static_cast<double>(*i);
                            }
                            else if (u)
                            {
                                powerCap = *u;
                            }
                        }
                        else if (!property.first.compare("PowerCapEnable"))
                        {
                            const bool* b = std::get_if<bool>(&property.second);

                            if (b)
                            {
                                enabled = *b;
                            }
                        }
                    }

                    nlohmann::json& value =
                        sensorJson["PowerLimit"]["LimitInWatts"];

                    // LimitException is Mandatory attribute as per OCP Baseline
                    // Profile – v1.0.0, so currently making it "NoAction"
                    // as default value to make it OCP Compliant.
                    sensorJson["PowerLimit"]["LimitException"] = "NoAction";

                    if (enabled)
                    {
                        // Redfish specification indicates PowerLimit should be
                        // null if the limit is not enabled.
                        value = powerCap * std::pow(10, scale);
                    }
                };

            crow::connections::systemBus->async_method_call(
                std::move(valueHandler), "xyz.openbmc_project.Settings",
                "/xyz/openbmc_project/control/host0/power_cap",
                "org.freedesktop.DBus.Properties", "GetAll",
                "xyz.openbmc_project.Control.Power.Cap");
        };

        crow::connections::systemBus->async_method_call(
            std::move(chassisHandler), "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
            "/xyz/openbmc_project/inventory", 0,
            std::array<const char*, 2>{
                "xyz.openbmc_project.Inventory.Item.Board",
                "xyz.openbmc_project.Inventory.Item.Chassis"});
    }
    void doPatch(crow::Response& res, const crow::Request& req,
                 const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            messages::internalError(res);
            res.end();
            return;
        }

        const std::string& chassisName = params[0];
        auto asyncResp = std::make_shared<SensorsAsyncResp>(
            res, chassisName, sensors::dbus::types.at(sensors::node::power),
            sensors::node::power);

        std::optional<std::vector<nlohmann::json>> voltageCollections;
        std::optional<std::vector<nlohmann::json>> powerCtlCollections;

        if (!json_util::readJson(req, asyncResp->res, "PowerControl",
                                 powerCtlCollections, "Voltages",
                                 voltageCollections))
        {
            return;
        }

        if (powerCtlCollections)
        {
            setPowerCapOverride(asyncResp, *powerCtlCollections);
        }
        if (voltageCollections)
        {
            std::unordered_map<std::string, std::vector<nlohmann::json>>
                allCollections;
            allCollections.emplace("Voltages", *std::move(voltageCollections));
            checkAndDoSensorsOverride(asyncResp, allCollections);
        }
    }
};

} // namespace redfish
