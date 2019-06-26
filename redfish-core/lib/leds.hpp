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

void getLedState(std::shared_ptr<AsyncResp> aResp, nlohmann::json &ledEntry,
                 const std::string owner, const std::string ledPath)
{

    crow::connections::systemBus->async_method_call(
        [aResp, &ledEntry](const boost::system::error_code ec,
                           const boost::container::flat_map<std::string,
                              std::variant<std::string, uint16_t,
                                  uint8_t>>& ret)
        {
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

            std::string ledState;

            if (boost::ends_with(*state, "On"))
            {
                ledState = "Lit";
            }
            else if (boost::ends_with(*state, "Blink"))
            {
                ledState = "Blinking";
            }
            else if(boost::ends_with(*state, "Off"))
            {
                ledState = "Off";
            }
            else
            {
                ledState = "Unknown";
            }

            ledEntry = ledState;
        },
        owner, ledPath,
        "org.freedesktop.DBus.Properties",
        "GetAll", "xyz.openbmc_project.Led.Physical");
}

} // namespace redfish
