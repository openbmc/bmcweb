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

#include <regex>

namespace redfish
{

struct PowerMetrics : std::enable_shared_from_this<PowerMetrics>
{
    PowerMetrics(const std::shared_ptr<SensorsAsyncResp>& asyncResp) :
        asyncResp(asyncResp)
    {
    }

    ~PowerMetrics()
    {
        nlohmann::json& pc = asyncResp->res.jsonValue["PowerControl"];
        if (!pc.empty())
        {
            nlohmann::json& pcJson = pc.back();

            // Set metric values after all have been read
            if (avgMetric != nullptr)
            {
                pcJson["PowerMetrics"]["AverageConsumedWatts"] = *avgMetric;
            }

            if (maxMetric != nullptr)
            {
                pcJson["PowerMetrics"]["MaxConsumedWatts"] =
                    *std::max_element(maxMetric->begin(), maxMetric->end());
            }

            if (avgMetric != nullptr || maxMetric != nullptr)
            {
                // Set interval based on number of history entries divided by 2
                // Collection is done every 30s, Redfish interval is in minutes
                pcJson["PowerMetrics"]["IntervalInMin"] = int30s / 2;
            }
        }
    }

    void getAverage(const std::string service, const std::string path)
    {
        std::shared_ptr<PowerMetrics> self = shared_from_this();
        // Get aggregation history
        crow::connections::systemBus->async_method_call(
            [self](const boost::system::error_code ec,
                   std::variant<std::vector<std::tuple<uint64_t, int64_t>>>&
                       history) {
                if (ec)
                {
                    // No average power aggregation history
                    return;
                }

                auto* avgHistory =
                    std::get_if<std::vector<std::tuple<uint64_t, int64_t>>>(
                        &(history));
                if (avgHistory == nullptr)
                {
                    // Do not set/return average metric value
                    return;
                }

                // Reduce down to a max of 10 minutes of aggregation history
                if (avgHistory->size() > self->maxAggHistory)
                {
                    avgHistory->resize(self->maxAggHistory);
                }

                // Set the interval for this aggregation history
                self->setInterval(avgHistory->size());

                // Accumulate average power consumed per power supply
                // as the total average power consumed
                auto sum = std::accumulate(
                    avgHistory->begin(), avgHistory->end(), 0,
                    [](int64_t sum, const std::tuple<uint64_t, int64_t>& entry) {
                        return sum + std::get<int64_t>(entry);
                    });
                if (self->avgMetric == nullptr)
                {
                    self->avgTotal = sum / avgHistory->size();
                    self->avgMetric = &self->avgTotal;
                }
                else
                {
                    self->avgTotal = self->avgTotal + sum / avgHistory->size();
                }
            },
            service, path, "org.freedesktop.DBus.Properties", "Get",
            "org.open_power.Sensor.Aggregation.History.Average", "Values");
    }

    void getMaximum(const std::string service, const std::string path)
    {
        std::shared_ptr<PowerMetrics> self = shared_from_this();
        // Get aggregation history
        crow::connections::systemBus->async_method_call(
            [self](const boost::system::error_code ec,
                   std::variant<std::vector<std::tuple<uint64_t, int64_t>>>&
                       history) {
                if (ec)
                {
                    // No maximum power aggregation history
                    return;
                }

                auto* maxHistory =
                    std::get_if<std::vector<std::tuple<uint64_t, int64_t>>>(
                        &(history));
                if (maxHistory == nullptr)
                {
                    // Do not set/return maximum metric value
                    return;
                }

                // Reduce down to a max of 10 minutes of aggregation history
                if (maxHistory->size() > self->maxAggHistory)
                {
                    maxHistory->resize(self->maxAggHistory);
                }

                // Set the interval for this aggregation history
                self->setInterval(maxHistory->size());

                // Accumulate maximum power consumed per power supply
                // as the total maximum power consumed
                if (self->maxMetric == nullptr)
                {
                    for (auto& maxEntry : *maxHistory)
                    {
                        self->maxTotals.emplace_back(
                            std::move(std::get<int64_t>(maxEntry)));
                    }
                    self->maxMetric = &self->maxTotals;
                }
                else
                {
                    auto hisIt = maxHistory->begin();
                    auto totIt = self->maxTotals.begin();
                    while (totIt != self->maxTotals.end())
                    {
                        *totIt = *totIt + std::get<int64_t>(*hisIt++);
                        ++totIt;
                        if (hisIt == maxHistory->end())
                        {
                            break;
                        }
                    }
                    if (hisIt != maxHistory->end() &&
                        totIt == self->maxTotals.end())
                    {
                        while (hisIt != maxHistory->end())
                        {
                            self->maxTotals.emplace_back(
                                std::move(std::get<int64_t>(*hisIt)));
                            ++hisIt;
                        }
                    }
                }
            },
            service, path, "org.freedesktop.DBus.Properties", "Get",
            "org.open_power.Sensor.Aggregation.History.Maximum", "Values");
    }

    void setInterval(uint64_t int30sCount)
    {
        std::shared_ptr<PowerMetrics> self = shared_from_this();
        // Redfish has a single interval for all power metrics
        // Set the interval to the highest history interval count
        if (int30sCount > self->int30s)
        {
            self->int30s = int30sCount;
        }
    }

    std::shared_ptr<SensorsAsyncResp> asyncResp;
    int64_t avgTotal = 0;
    int64_t* avgMetric = nullptr;
    std::vector<int64_t> maxTotals;
    std::vector<int64_t>* maxMetric = nullptr;
    uint64_t int30s = 0;
    const uint64_t maxAggHistory = 20;
};

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
            std::array<const char*, 1>{
                "xyz.openbmc_project.Inventory.Item.Chassis"});

        // Retrieve OpenPower power metrics
        const char* avgIntf =
            "org.open_power.Sensor.Aggregation.History.Average";
        const char* maxIntf =
            "org.open_power.Sensor.Aggregation.History.Maximum";
        crow::connections::systemBus->async_method_call(
            [sensorAsyncResp, avgIntf, maxIntf](
                const boost::system::error_code ec,
                const std::vector<std::pair<
                    std::string, std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>>&
                    subtree) {
                if (ec)
                {
                    // No OpenPower aggregation history
                    return;
                }

                auto powerMetrics =
                    std::make_shared<PowerMetrics>(sensorAsyncResp);
                const std::regex psInputRegex =
                    std::regex("(ps)([0-9]+)(_input_power)");

                for (const auto& object : subtree)
                {
                    const auto& path = object.first;
                    size_t metricPos = path.rfind("/");
                    if (metricPos == std::string::npos)
                    {
                        // Last object path '/' not found
                        continue;
                    }

                    // Get object name providing metric
                    size_t objectPos = path.rfind("/", metricPos - 1);
                    if (objectPos == std::string::npos)
                    {
                        // Invalid object path providing metric
                        continue;
                    }
                    auto objectName =
                        path.substr(objectPos + 1, (metricPos - 1) - objectPos);

                    // Only get power metrics from ps*_input_power objects
                    if (std::regex_match(objectName, psInputRegex))
                    {
                        for (const auto& service : object.second)
                        {
                            // Get aggregation history from the proper interface
                            if (std::find(service.second.begin(),
                                          service.second.end(),
                                          avgIntf) != service.second.end())
                            {
                                powerMetrics->getAverage(service.first, path);
                            }
                            if (std::find(service.second.begin(),
                                          service.second.end(),
                                          maxIntf) != service.second.end())
                            {
                                powerMetrics->getMaximum(service.first, path);
                            }
                        }
                    }
                }
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
            "/org/open_power/sensors/aggregation/per_30s", int32_t(0),
            std::array<const char*, 2>{avgIntf, maxIntf});
    }
    void doPatch(crow::Response& res, const crow::Request& req,
                 const std::vector<std::string>& params) override
    {
        setSensorOverride(res, req, params, typeList, "Power");
    }
};

} // namespace redfish
