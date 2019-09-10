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
    Power(CrowApp& app) :
        Node((app), "/redfish/v1/Chassis/<str>/Power/", std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    std::vector<const char*> typeList = {"/xyz/openbmc_project/sensors/voltage",
                                         "/xyz/openbmc_project/sensors/power"};
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            res.result(boost::beast::http::status::internal_server_error);
            res.end();
            return;
        }
        const std::string& chassis_name = params[0];

        res.jsonValue["PowerControl"] = nlohmann::json::array();

        auto sensorAsyncResp = std::make_shared<SensorsAsyncResp>(
            res, chassis_name, typeList, "Power");

        getChassisData(sensorAsyncResp);

        // This callback verifies that the power limit is only provided for the
        // chassis that implements the Chassis inventory item. This prevents
        // things like power supplies providing the chassis power limit
        auto chassisHandler = [sensorAsyncResp](
                                  const boost::system::error_code ec,
                                  const std::vector<std::string>&
                                      chassisPaths) {
            if (ec)
            {
                BMCWEB_LOG_ERROR
                    << "Power Limit GetSubTreePaths handler Dbus error " << ec;
                return;
            }

            bool found = false;
            for (const std::string& chassis : chassisPaths)
            {
                size_t len = std::string::npos;
                size_t lastPos = chassis.rfind("/");
                if (lastPos == std::string::npos)
                {
                    continue;
                }

                if (lastPos == chassis.size() - 1)
                {
                    size_t end = lastPos;
                    lastPos = chassis.rfind("/", lastPos - 1);
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
                                sdbusplus::message::variant_ns::get_if<int64_t>(
                                    &property.second);

                            if (i)
                            {
                                scale = *i;
                            }
                        }
                        else if (!property.first.compare("PowerCap"))
                        {
                            const double* d =
                                sdbusplus::message::variant_ns::get_if<double>(
                                    &property.second);
                            const int64_t* i =
                                sdbusplus::message::variant_ns::get_if<int64_t>(
                                    &property.second);
                            const uint32_t* u =
                                sdbusplus::message::variant_ns::get_if<
                                    uint32_t>(&property.second);

                            if (d)
                            {
                                powerCap = *d;
                            }
                            else if (i)
                            {
                                powerCap = *i;
                            }
                            else if (u)
                            {
                                powerCap = *u;
                            }
                        }
                        else if (!property.first.compare("PowerCapEnable"))
                        {
                            const bool* b =
                                sdbusplus::message::variant_ns::get_if<bool>(
                                    &property.second);

                            if (b)
                            {
                                enabled = *b;
                            }
                        }
                    }

                    nlohmann::json& value =
                        sensorJson["PowerLimit"]["LimitInWatts"];

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
            "/xyz/openbmc_project/inventory", int32_t(0),
            std::array<const char*, 2>{
                "xyz.openbmc_project.Inventory.Item.Chassis",
                "xyz.openbmc_project.Inventory.Item.PowerSupply"});
    }
    void doPatch(crow::Response& res, const crow::Request& req,
                 const std::vector<std::string>& params) override
    {
        setSensorOverride(res, req, params, typeList, "Power");
    }
};

} // namespace redfish
