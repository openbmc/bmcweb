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
        Node((app),
#ifdef BMCWEB_ENABLE_REDFISH_ONE_CHASSIS
             "/redfish/v1/Chassis/chassis/Power/")
#else
             "/redfish/v1/Chassis/<str>/Power/", std::string())
#endif
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {

#ifdef BMCWEB_ENABLE_REDFISH_ONE_CHASSIS
        const std::string chassis_name = "chassis";
#else
        if (params.size() != 1)
        {
            res.result(boost::beast::http::status::internal_server_error);
            res.end();
            return;
        }
        const std::string& chassis_name = params[0];
#endif

        res.jsonValue["@odata.id"] =
            "/redfish/v1/Chassis/" + chassis_name + "/Power";
        res.jsonValue["@odata.type"] = "#Power.v1_2_1.Power";
        res.jsonValue["@odata.context"] = "/redfish/v1/$metadata#Power.Power";
        res.jsonValue["Id"] = "Power";
        res.jsonValue["Name"] = "Power";

        auto sensorAsyncResp = std::make_shared<SensorsAsyncResp>(
            res, chassis_name,
            std::initializer_list<const char*>{
                "/xyz/openbmc_project/sensors/voltage",
                "/xyz/openbmc_project/sensors/power"},
            "Power");

#ifdef BMCWEB_ENABLE_REDFISH_ONE_CHASSIS
        const std::string path = "/xyz/openbmc_project/sensors";
        const std::array<std::string, 1> interfaces = { "xyz.openbmc_project.Sensor.Value" };

        // Response handler for parsing objects subtree
        auto respHandler = [sensorAsyncResp](const boost::system::error_code ec, const GetSubTreeType& subtree) {
            if (ec)
            {
                messages::internalError(sensorAsyncResp->res);
                BMCWEB_LOG_ERROR << "getSubTree respHandler: Dbus error "
                                 << ec;
                return;
            }

            for (const std::pair<std::string, std::vector<std::pair<std::string, std::vector<std::string>>>>& object : subtree)
            {
                if (object.second.empty())
                {
                    continue;
                }

                BMCWEB_LOG_DEBUG << "sensor: " << object.first << " interface: " << object.second.front().first;

                auto valueHandler = [sensorAsyncResp](const boost::system::error_code ec, const SensorVariant& value) {
                    if (ec)
                    {
                        messages::internalError(sensorAsyncResp->res);
                        BMCWEB_LOG_ERROR << "get valueHandler: Dbus error " << ec;
                        return;
                    }

                    const int64_t *i = sdbusplus::message::variant_ns::get_if<int64_t>(&value);
                    const double *d = sdbusplus::message::variant_ns::get_if<double>(&value);

                    BMCWEB_LOG_DEBUG << "val: " << (i ? *i : (d ? *d : 0.0));
                };

		crow::connections::systemBus->async_method_call(
                    std::move(valueHandler), object.second.front().first,
                    object.first, "org.freedesktop.DBus.Properties", "Get",
                    "xyz.openbmc_project.Sensor.Value", "int");
            }
        };

        // Make call to ObjectMapper to find all sensors objects
        crow::connections::systemBus->async_method_call(
            std::move(respHandler), "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree", path, 2,
            interfaces);
#else
        // TODO Need to retrieve Power Control information.
        getChassisData(sensorAsyncResp);
#endif
    }
};

} // namespace redfish
