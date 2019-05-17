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

using PropertyVariant =
    sdbusplus::message::variant<std::string, int64_t, double, uint32_t, bool>;

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
    std::initializer_list<const char*> typeList = {
        "/xyz/openbmc_project/sensors/voltage",
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

        auto sensorAsyncResp = std::make_shared<SensorsAsyncResp>(
            res, chassis_name, typeList, "Power");

        struct PowerProperty
        {
            PowerProperty(const char* i, const char* n, const char* o,
                          const char* p, const char* r) :
                interface(i),
                name(n), output(o), path(p), redfish(r)
            {
            }

            std::string interface;
            std::string name;
            std::string output;
            std::string path;
            std::string redfish;
        };
        std::array<PowerProperty, 2> props = {
            PowerProperty("xyz.openbmc_project.Sensor.Value", "total_power",
                          "PowerConsumedWatts", "/xyz/openbmc_project/sensors/",
                          "PowerControl"),
            PowerProperty("xyz.openbmc_project.Control.Power.Cap", "power_cap",
                          "LimitInWatts", "/xyz/openbmc_project/control/",
                          "PowerLimit")};

        for (const PowerProperty& prop : props)
        {
            auto respHandler = [sensorAsyncResp,
                                prop](const boost::system::error_code ec,
                                      const GetSubTreeType& subtree) {
                if (ec)
                {
                    messages::resourceNotFound(sensorAsyncResp->res, prop.name,
                                               sensorAsyncResp->chassisId);
                    return;
                }

                for (const std::pair<
                         std::string,
                         std::vector<
                             std::pair<std::string, std::vector<std::string>>>>&
                         object : subtree)
                {
                    if (object.second.empty())
                    {
                        continue;
                    }

                    std::vector<std::string> split;

                    split.reserve(6);
                    boost::algorithm::split(split, object.first,
                                            boost::is_any_of("/"));
                    if (split.size() < 6)
                    {
                        continue;
                    }

                    if (split[5].compare(prop.name))
                    {
                        continue;
                    }

                    if (!prop.redfish.compare("PowerLimit"))
                    {
                        auto limitHandler = [sensorAsyncResp, prop](
                                                const boost::system::error_code
                                                    ec,
                                                const std::vector<
                                                    std::pair<std::string,
                                                              PropertyVariant>>&
                                                    properties) {
                            if (ec)
                            {
                                messages::internalError(sensorAsyncResp->res);
                                BMCWEB_LOG_ERROR
                                    << "get valueHandler: Dbus error " << ec;
                                return;
                            }

                            nlohmann::json& tempArray =
                                sensorAsyncResp->res.jsonValue[prop.redfish];

                            if (tempArray.empty())
                            {
                                tempArray.push_back({});
                            }

                            nlohmann::json& sensorJson = tempArray.back();
                            bool enabled = false;
                            double powerCap = 0.0;

                            for (const std::pair<std::string, PropertyVariant>&
                                     property : properties)
                            {
                                if (!property.first.compare("PowerCap"))
                                {
                                    const double* d =
                                        sdbusplus::message::variant_ns::get_if<
                                            double>(&property.second);
                                    const int64_t* i =
                                        sdbusplus::message::variant_ns::get_if<
                                            int64_t>(&property.second);
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
                                    else
                                    {
                                        continue;
                                    }
                                }
                                else if (property.first.compare(
                                             "PowerCapEnable"))
                                {
                                    const bool* b =
                                        sdbusplus::message::variant_ns::get_if<
                                            bool>(&property.second);

                                    if (b)
                                    {
                                        enabled = *b;
                                    }
                                    else
                                    {
                                        continue;
                                    }
                                }
                            }

                            nlohmann::json& value = sensorJson[prop.output];

                            if (enabled)
                            {
                                value = powerCap;
                            }
                        };

                        crow::connections::systemBus->async_method_call(
                            std::move(limitHandler),
                            object.second.front().first, object.first,
                            "org.freedesktop.DBus.Properties", "GetAll",
                            prop.interface);
                    }
                    else
                    {
                        auto valueHandler = [sensorAsyncResp, prop](
                                                const boost::system::error_code
                                                    ec,
                                                const std::vector<
                                                    std::pair<std::string,
                                                              PropertyVariant>>&
                                                    properties) {
                            if (ec)
                            {
                                messages::internalError(sensorAsyncResp->res);
                                BMCWEB_LOG_ERROR
                                    << "get valueHandler: Dbus error " << ec;
                                return;
                            }

                            int64_t scale = 0;
                            nlohmann::json& tempArray =
                                sensorAsyncResp->res.jsonValue[prop.redfish];

                            if (tempArray.empty())
                            {
                                tempArray.push_back(
                                    {{"@odata.id",
                                      "/redfish/v1/Chassis/" +
                                          sensorAsyncResp->chassisId + "/" +
                                          sensorAsyncResp->chassisSubNode +
                                          "#/" + prop.redfish + "/"}});
                            }

                            nlohmann::json& sensorJson = tempArray.back();

                            sensorJson["MemberId"] = prop.name;
                            sensorJson["Name"] =
                                boost::replace_all_copy(prop.name, "_", " ");

                            sensorJson["Status"]["State"] = "Enabled";
                            sensorJson["Status"]["Health"] = "OK";

                            for (const std::pair<std::string, PropertyVariant>&
                                     property : properties)
                            {
                                if (!property.first.compare("Scale"))
                                {
                                    const int64_t* i =
                                        sdbusplus::message::variant_ns::get_if<
                                            int64_t>(&property.second);

                                    if (i)
                                    {
                                        scale = *i;
                                    }
                                }
                                else if (!property.first.compare("Value"))
                                {
                                    const double* d =
                                        sdbusplus::message::variant_ns::get_if<
                                            double>(&property.second);
                                    const int64_t* i =
                                        sdbusplus::message::variant_ns::get_if<
                                            int64_t>(&property.second);
                                    const uint32_t* u =
                                        sdbusplus::message::variant_ns::get_if<
                                            uint32_t>(&property.second);
                                    double val = 0.0;
                                    nlohmann::json& value =
                                        sensorJson[prop.output];

                                    if (i)
                                    {
                                        val = *i;
                                    }
                                    else if (d)
                                    {
                                        val = *d;
                                    }
                                    else if (u)
                                    {
                                        val = *u;
                                    }
                                    else
                                    {
                                        continue;
                                    }

                                    val = val * std::pow(10, scale);
                                    value = val;
                                }
                            }
                        };
                        crow::connections::systemBus->async_method_call(
                            std::move(valueHandler),
                            object.second.front().first, object.first,
                            "org.freedesktop.DBus.Properties", "GetAll",
                            prop.interface);
                    }
                }
            };

            crow::connections::systemBus->async_method_call(
                std::move(respHandler), "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree", prop.path, 2,
                std::array<std::string, 1>{prop.interface});
        }

        getChassisData(sensorAsyncResp);
    }
    void doPatch(crow::Response& res, const crow::Request& req,
                 const std::vector<std::string>& params) override
    {
        setSensorOverride(res, req, params, typeList, "Power");
    }
};

} // namespace redfish
