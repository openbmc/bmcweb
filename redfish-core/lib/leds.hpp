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

void getLedState(std::shared_ptr<SensorsAsyncResp> aResp, nlohmann::json& sensorEntry,
                 const std::string& owner, const std::string& ledPath)
{
    BMCWEB_LOG_DEBUG << "getLedState enter: led="
                     << ledPath;
    BMCWEB_LOG_DEBUG << "getLedState enter led=" << ledPath << "sensorEntry ref=" << &sensorEntry;
    sensorEntry["IndicatorLed"] = "Lit";
/*
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
            else
            {
                BMCWEB_LOG_DEBUG << "ledstate=" << *state;
            }

            if (boost::ends_with(*state, "On"))
            {
                sensorEntry["IndicatorLed"] = "Lit";
            }
            else if (boost::ends_with(*state, "Blink"))
            {
                sensorEntry["IndicatorLed"] = "Blinking";
            }
            else if(boost::ends_with(*state, "Off"))
            {
                sensorEntry["IndicatorLed"] = "Off";
            }
            else
            {
                sensorEntry["IndicatorLed"] = "Unknown";
            }

            //BMCWEB_LOG_DEBUG << "ledState=" << ledState;

            BMCWEB_LOG_DEBUG << "getLedState respHandler1 exit: led="
                             << ledPath;
        },
        owner, ledPath,
        "org.freedesktop.DBus.Properties",
        "GetAll", "xyz.openbmc_project.Led.Physical");
*/
}

void checkSensorLed(std::shared_ptr<SensorsAsyncResp> aResp,
                    const std::string& sensorPath, nlohmann::json& sensorEntry)
{
    //sensorEntry["IndicatorLed"] = "Lit"; //This one doesn't crash
    nlohmann::json* sensor_json = &sensorEntry;
    //These debug checks were here to make sure we still had the original
    //json object reference and we weren't somehow making a copy of the object
    //from the lambda capture
    BMCWEB_LOG_DEBUG << "checkSensorLed enter sensor=" << sensorPath << "sensorEntry ref=" << &sensorEntry;
    crow::connections::systemBus->async_method_call(
        [sensor_json, sensorPath, aResp](const boost::system::error_code ec,
                            std::variant<std::vector<std::string>>
                                variantEndpoints)
        {
            BMCWEB_LOG_DEBUG << "respHandler1 enter: sensor="
                             << sensorPath;
            BMCWEB_LOG_DEBUG << "checkSensorLed enter asnyc1 sensor=" << sensorPath << "sensorEntry ref=" << sensor_json;
            if (ec)
            {
                return; // if they don't have an association we
                        // just skip it
            }
            (*sensor_json)["IndicatorLED"] = "Lit"; //Crashes here
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
                [sensor_json, sensorPath, aResp](const boost::system::error_code ec,
                                    std::variant<std::vector<std::string>>
                                        variantEndpoints)
                {
                    BMCWEB_LOG_DEBUG << "respHandler2 enter: sensor="
                                     << sensorPath;
                    BMCWEB_LOG_DEBUG << "checkSensorLed enter asnyc2 sensor=" << sensorPath << "sensorEntry ref=" << sensor_json;
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
                    "xyz.openbmc_project.Led.Physical"};//cut sensor_path
                    crow::connections::systemBus->async_method_call(
                        [aResp, endpoint, sensor_json, sensorPath]
                            (const boost::system::error_code ec,
                            const GetSubTreeType& subtree)
                        {
                            BMCWEB_LOG_DEBUG << "respHandler3 enter";
                            BMCWEB_LOG_DEBUG << "checkSensorLed enter asnyc3 sensor=" << sensorPath << "sensorEntry ref=" << sensor_json;
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

                            getLedState(aResp, *sensor_json, owner, endpoint);
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
