/*
// Copyright (c) 2019 Intel Corporation
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

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "redfish_util.hpp"

#include <variant>

namespace redfish
{
/**
 * @brief Retrieves identify led group properties over dbus
 *
 * @param[in] aResp     Shared pointer for generating response message.
 *
 * @return None.
 */
// TODO (Gunnar): Remove IndicatorLED after enough time has passed
inline void getIndicatorLedState(const std::shared_ptr<AsyncResp>& aResp)
{
    BMCWEB_LOG_DEBUG << "Get led groups";
    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                const std::variant<bool> asserted) {
            // Some systems may not have enclosure_identify_blink object so
            // proceed to get enclosure_identify state.
            if (!ec)
            {
                const bool* blinking = std::get_if<bool>(&asserted);
                if (!blinking)
                {
                    BMCWEB_LOG_DEBUG << "Get identity blinking LED failed";
                    messages::internalError(aResp->res);
                    return;
                }
                // Blinking ON, no need to check enclosure_identify assert.
                if (*blinking)
                {
                    aResp->res.jsonValue["IndicatorLED"] = "Blinking";
                    return;
                }
            }
            crow::connections::systemBus->async_method_call(
                [aResp](const boost::system::error_code ec2,
                        const std::variant<bool> asserted2) {
                    if (!ec2)
                    {
                        const bool* ledOn = std::get_if<bool>(&asserted2);
                        if (!ledOn)
                        {
                            BMCWEB_LOG_DEBUG
                                << "Get enclosure identity led failed";
                            messages::internalError(aResp->res);
                            return;
                        }

                        if (*ledOn)
                        {
                            aResp->res.jsonValue["IndicatorLED"] = "Lit";
                        }
                        else
                        {
                            aResp->res.jsonValue["IndicatorLED"] = "Off";
                        }
                    }
                    return;
                },
                "xyz.openbmc_project.LED.GroupManager",
                "/xyz/openbmc_project/led/groups/enclosure_identify",
                "org.freedesktop.DBus.Properties", "Get",
                "xyz.openbmc_project.Led.Group", "Asserted");
        },
        "xyz.openbmc_project.LED.GroupManager",
        "/xyz/openbmc_project/led/groups/enclosure_identify_blink",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Led.Group", "Asserted");
}

/**
 * @brief Sets identify led group properties
 *
 * @param[in] aResp     Shared pointer for generating response message.
 * @param[in] ledState  LED state passed from request
 *
 * @return None.
 */
// TODO (Gunnar): Remove IndicatorLED after enough time has passed
inline void setIndicatorLedState(const std::shared_ptr<AsyncResp>& aResp,
                                 const std::string& ledState)
{
    BMCWEB_LOG_DEBUG << "Set led groups";
    bool ledOn = false;
    bool ledBlinkng = false;

    if (ledState == "Lit")
    {
        ledOn = true;
    }
    else if (ledState == "Blinking")
    {
        ledBlinkng = true;
    }
    else if (ledState != "Off")
    {
        messages::propertyValueNotInList(aResp->res, ledState, "IndicatorLED");
        return;
    }

    crow::connections::systemBus->async_method_call(
        [aResp, ledOn, ledBlinkng](const boost::system::error_code ec) mutable {
            if (ec)
            {
                // Some systems may not have enclosure_identify_blink object so
                // Lets set enclosure_identify state to true if Blinking is
                // true.
                if (ledBlinkng)
                {
                    ledOn = true;
                }
            }
            crow::connections::systemBus->async_method_call(
                [aResp](const boost::system::error_code ec2) {
                    if (ec2)
                    {
                        BMCWEB_LOG_DEBUG << "DBUS response error " << ec2;
                        messages::internalError(aResp->res);
                        return;
                    }
                },
                "xyz.openbmc_project.LED.GroupManager",
                "/xyz/openbmc_project/led/groups/enclosure_identify",
                "org.freedesktop.DBus.Properties", "Set",
                "xyz.openbmc_project.Led.Group", "Asserted",
                std::variant<bool>(ledOn));
        },
        "xyz.openbmc_project.LED.GroupManager",
        "/xyz/openbmc_project/led/groups/enclosure_identify_blink",
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Led.Group", "Asserted",
        std::variant<bool>(ledBlinkng));
}

/**
 * @brief Retrieves identify led group properties over dbus
 *
 * @param[in] aResp     Shared pointer for generating response message.
 *
 * @return None.
 */
inline void getLocationIndicatorActive(const std::shared_ptr<AsyncResp>& aResp)
{
    BMCWEB_LOG_DEBUG << "Get LocationIndicatorActive";
    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                const std::variant<bool> asserted) {
            // Some systems may not have enclosure_identify_blink object so
            // proceed to get enclosure_identify state.
            if (!ec)
            {
                const bool* blinking = std::get_if<bool>(&asserted);
                if (!blinking)
                {
                    BMCWEB_LOG_DEBUG << "Get identity blinking LED failed";
                    messages::internalError(aResp->res);
                    return;
                }
                // Blinking ON, no need to check enclosure_identify assert.
                if (*blinking)
                {
                    aResp->res.jsonValue["LocationIndicatorActive"] = true;
                    return;
                }
            }
            crow::connections::systemBus->async_method_call(
                [aResp](const boost::system::error_code ec2,
                        const std::variant<bool> asserted2) {
                    if (!ec2)
                    {
                        const bool* ledOn = std::get_if<bool>(&asserted2);
                        if (!ledOn)
                        {
                            BMCWEB_LOG_DEBUG
                                << "Get enclosure identity led failed";
                            messages::internalError(aResp->res);
                            return;
                        }

                        if (*ledOn)
                        {
                            aResp->res.jsonValue["LocationIndicatorActive"] =
                                true;
                        }
                        else
                        {
                            aResp->res.jsonValue["LocationIndicatorActive"] =
                                false;
                        }
                    }
                    return;
                },
                "xyz.openbmc_project.LED.GroupManager",
                "/xyz/openbmc_project/led/groups/enclosure_identify",
                "org.freedesktop.DBus.Properties", "Get",
                "xyz.openbmc_project.Led.Group", "Asserted");
        },
        "xyz.openbmc_project.LED.GroupManager",
        "/xyz/openbmc_project/led/groups/enclosure_identify_blink",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Led.Group", "Asserted");
}

/**
 * @brief Sets identify led group properties
 *
 * @param[in] aResp     Shared pointer for generating response message.
 * @param[in] ledState  LED state passed from request
 *
 * @return None.
 */
inline void setLocationIndicatorActive(const std::shared_ptr<AsyncResp>& aResp,
                                       const bool ledState)
{
    BMCWEB_LOG_DEBUG << "Set LocationIndicatorActive";

    crow::connections::systemBus->async_method_call(
        [aResp, ledState](const boost::system::error_code ec) mutable {
            if (ec)
            {
                // Some systems may not have enclosure_identify_blink object so
                // lets set enclosure_identify state also if
                // enclosure_identify_blink failed
                crow::connections::systemBus->async_method_call(
                    [aResp](const boost::system::error_code ec2) {
                        if (ec2)
                        {
                            BMCWEB_LOG_DEBUG << "DBUS response error " << ec2;
                            messages::internalError(aResp->res);
                            return;
                        }
                    },
                    "xyz.openbmc_project.LED.GroupManager",
                    "/xyz/openbmc_project/led/groups/enclosure_identify",
                    "org.freedesktop.DBus.Properties", "Set",
                    "xyz.openbmc_project.Led.Group", "Asserted",
                    std::variant<bool>(ledState));
            }
        },
        "xyz.openbmc_project.LED.GroupManager",
        "/xyz/openbmc_project/led/groups/enclosure_identify_blink",
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Led.Group", "Asserted",
        std::variant<bool>(ledState));
}
} // namespace redfish
