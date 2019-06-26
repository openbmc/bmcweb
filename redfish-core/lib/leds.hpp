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

#include <math.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/asio.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/range/algorithm/replace_copy_if.hpp>
#include <dbus_singleton.hpp>
#include <utils/json_utils.hpp>
#include <variant>

namespace redfish
{

using GetSubTreeType = std::vector<
    std::pair<std::string,
              std::vector<std::pair<std::string, std::vector<std::string>>>>>;

void getLedState(std::shared_ptr<SensorsAsyncResp> aResp, std::shared_ptr<nlohmann::json> sensorEntry,
                 const std::string owner, const std::string ledPath)
{

    crow::connections::systemBus->async_method_call(
        [ledPath, aResp, &sensorEntry](const boost::system::error_code ec,
                           const boost::container::flat_map<std::string,
                              std::variant<std::string, uint16_t,
                                  uint8_t>>& ret)
        {
            BMCWEB_LOG_DEBUG << "getLedState respHandler1 enter: led="
                             << ledPath;
            if (ec)
            {
                messages::internalError(aResp->res);
                BMCWEB_LOG_ERROR
                    << "Sensor getSensorValues resp_handler: "
                    << "Dbus error " << ec;
                return;
            }

            auto findState = ret.find("State");

            if (findState == ret.end())
            {
                BMCWEB_LOG_ERROR
                    << "Invalid LED interface";
                messages::internalError(aResp->res);
                return;
            }

            auto state = std::get_if<std::string>(&(findState->second));

            if (state == nullptr)
            {

                BMCWEB_LOG_ERROR
                    << "Invalid redundancy interface "
                        "types";
                messages::internalError(aResp->res);
                return;
            }

            if (boost::ends_with(*state, "On"))
            {
                (*sensorEntry.get())["IndicatorLed"] = "Lit";
            }
            else if (boost::ends_with(*state, "Blink"))
            {
                (*sensorEntry.get())["IndicatorLed"] = "Blinking";
            }
            else if(boost::ends_with(*state, "Off"))
            {
                (*sensorEntry.get())["IndicatorLed"] = "Off";
            }
            else
            {
                (*sensorEntry.get())["IndicatorLed"] = "Unknown";
            }

            //BMCWEB_LOG_DEBUG << "ledState=" << ledState;

            BMCWEB_LOG_DEBUG << "getLedState respHandler1 exit: led="
                             << ledPath;
        },
        owner, ledPath,
        "org.freedesktop.DBus.Properties",
        "GetAll", "xyz.openbmc_project.Led.Physical");
}

void checkSensorLed(std::shared_ptr<redfish::SensorsAsyncResp> aResp,
                    const std::string& sensorPath, std::shared_ptr<nlohmann::json> sensorEntry)
{
    crow::connections::systemBus->async_method_call(
        [&sensorEntry, sensorPath, aResp](const boost::system::error_code ec,
                            std::variant<std::vector<std::string>>
                                variantEndpoints)
        {
            BMCWEB_LOG_DEBUG << "respHandler1 enter: sensor="
                             << sensorPath;
            if (ec)
            {
                return; // if they don't have an association we
                        // just skip it
            }

            auto endpoints = std::get_if<std::vector<std::string>>(
                                &variantEndpoints);

            if (endpoints == nullptr)
            {
                BMCWEB_LOG_ERROR << "Invalid association interface";
                messages::internalError(aResp->res);
                return;
            }

            // Assume there is only one endpoint
            std::string endpoint = *(endpoints->begin());
            BMCWEB_LOG_DEBUG << "endpoint=" << endpoint;

            crow::connections::systemBus->async_method_call(
                [&sensorEntry, sensorPath, aResp](const boost::system::error_code ec,
                                    std::variant<std::vector<std::string>>
                                        variantEndpoints)
                {
                    BMCWEB_LOG_DEBUG << "respHandler2 enter: sensor="
                                     << sensorPath;
                    if (ec)
                    {
                        return; // if they don't have an association we
                                // just skip it
                    }

                    auto endpoints = std::get_if<std::vector<std::string>>(
                                        &variantEndpoints);

                    if (endpoints == nullptr)
                    {
                        BMCWEB_LOG_ERROR << "Invalid association interface";
                        messages::internalError(aResp->res);
                        return;
                    }

                    // Assume there is only one endpoint
                    std::string endpoint = *(endpoints->begin());
                    BMCWEB_LOG_DEBUG << "endpoint=" << endpoint;

                    const std::array<const char*, 1> interfaces = {
                    "xyz.openbmc_project.Led.Physical"};
                    crow::connections::systemBus->async_method_call(
                        [aResp, endpoint, &sensorEntry]
                            (const boost::system::error_code ec,
                            const GetSubTreeType& subtree)
                        {
                            BMCWEB_LOG_DEBUG << "respHandler3 enter";
                            if (ec)
                            {
                                messages::internalError(aResp->res);
                                BMCWEB_LOG_ERROR
                                    << "checkSensorLed get owner resp_handler: "
                                    << "Dbus error " << ec;
                                return;
                            }

                            auto it = std::find_if(subtree.begin(), subtree.end(),
                                        [endpoint](const std::pair<std::string,
                                                       std::vector<std::pair<
                                                       std::string, std::vector<
                                                       std::string>>>> &object)
                                        {
                                            return object.first == endpoint;
                                        });

                            if(it == subtree.end())
                            {
                                BMCWEB_LOG_ERROR
                                    << "Could not find path for physical led: "
                                    << endpoint;
                                return;
                            }

                            std::string ledPath = (*it).first;
                            BMCWEB_LOG_DEBUG << "Found path for Led '" <<
                                                ledPath;

                            // This should always be correct if we found the sensorPath
                            std::string owner = (*it).second.begin()->first;
                            BMCWEB_LOG_DEBUG << "Found service for Led '" <<
                                                ledPath << "': " << owner;

                            getLedState(aResp, sensorEntry, owner, endpoint);
                            BMCWEB_LOG_DEBUG << "respHandler3 exit";
                        },
                        "xyz.openbmc_project.ObjectMapper",
                        "/xyz/openbmc_project/object_mapper",
                        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                        "/xyz/openbmc_project/led/physical", 2, interfaces);

                    BMCWEB_LOG_DEBUG << "respHandler2 exit";
                },
                "xyz.openbmc_project.ObjectMapper",
                endpoint + "/leds",
                "org.freedesktop.DBus.Properties", "Get",
                "xyz.openbmc_project.Association", "endpoints");

            BMCWEB_LOG_DEBUG << "respHandler1 exit";
        },
        "xyz.openbmc_project.ObjectMapper",
        sensorPath + "/inventory",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Association", "endpoints");
}

} // namespace redfish
