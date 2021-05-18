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
 * @brief Get the service name of the LED manager
 *
 * @param[in] aResp     Shared pointer for generating response message.
 * @param[in] path      The D-Bus Object path
 * @param[in] handler   Call back
 *
 * @return None.
 */
template <typename Handler>
inline void getLEDService(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                          const std::string& path, Handler&& handler)
{
    using GetObjectType =
        std::vector<std::pair<std::string, std::vector<std::string>>>;

    crow::connections::systemBus->async_method_call(
        [aResp, handler{std::move(handler)}](const boost::system::error_code ec,
                                             const GetObjectType& objectNames) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error, ec: " << ec.value();
                messages::internalError(aResp->res);
                return;
            }

            if (objectNames.size() != 1)
            {
                messages::internalError(aResp->res);
                return;
            }

            std::string service = objectNames.begin()->first;
            handler(service);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", path,
        std::array<const char*, 1>{"xyz.openbmc_project.Led.Group"});
}

/**
 * @brief Retrieves identify led group properties over dbus
 *
 * @param[in] aResp     Shared pointer for generating response message.
 *
 * @return None.
 */
// TODO (Gunnar): Remove IndicatorLED after enough time has passed
inline void
    getIndicatorLedState(const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    BMCWEB_LOG_DEBUG << "Get led groups";

    auto callback = [aResp](const std::string& serviceName) {
        crow::connections::systemBus->async_method_call(
            [aResp, serviceName](const boost::system::error_code ec,
                                 const std::variant<bool> asserted) {
                // Some systems may not have enclosure_identify_blink object
                // so proceed to get enclosure_identify state.
                if (!ec)
                {
                    const bool* blinking = std::get_if<bool>(&asserted);
                    if (!blinking)
                    {
                        BMCWEB_LOG_DEBUG << "Get identity blinking LED failed";
                        messages::internalError(aResp->res);
                        return;
                    }
                    // Blinking ON, no need to check enclosure_identify
                    // assert.
                    if (*blinking)
                    {
                        aResp->res.jsonValue["IndicatorLED"] = "Blinking";
                        return;
                    }
                }
                crow::connections::systemBus->async_method_call(
                    [aResp, serviceName](const boost::system::error_code ec2,
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
                    serviceName,
                    "/xyz/openbmc_project/led/groups/enclosure_identify",
                    "org.freedesktop.DBus.Properties", "Get",
                    "xyz.openbmc_project.Led.Group", "Asserted");
            },
            serviceName,
            "/xyz/openbmc_project/led/groups/enclosure_identify_blink",
            "org.freedesktop.DBus.Properties", "Get",
            "xyz.openbmc_project.Led.Group", "Asserted");
    };

    getLEDService(aResp,
                  "/xyz/openbmc_project/led/groups/enclosure_identify_blink",
                  std::move(callback));
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
inline void
    setIndicatorLedState(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
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

    auto callback = [aResp, &ledOn,
                     ledBlinkng](const std::string& serviceName) {
        crow::connections::systemBus->async_method_call(
            [aResp, ledOn, ledBlinkng,
             serviceName](const boost::system::error_code ec) mutable {
                if (ec)
                {
                    // Some systems may not have enclosure_identify_blink object
                    // so Lets set enclosure_identify state to true if Blinking
                    // is true.
                    if (ledBlinkng)
                    {
                        ledOn = true;
                    }
                }
                crow::connections::systemBus->async_method_call(
                    [aResp, serviceName](const boost::system::error_code ec2) {
                        if (ec2)
                        {
                            BMCWEB_LOG_DEBUG << "DBUS response error " << ec2;
                            messages::internalError(aResp->res);
                            return;
                        }
                        messages::success(aResp->res);
                    },
                    serviceName,
                    "/xyz/openbmc_project/led/groups/enclosure_identify",
                    "org.freedesktop.DBus.Properties", "Set",
                    "xyz.openbmc_project.Led.Group", "Asserted",
                    std::variant<bool>(ledOn));
            },
            serviceName,
            "/xyz/openbmc_project/led/groups/enclosure_identify_blink",
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Led.Group", "Asserted",
            std::variant<bool>(ledBlinkng));
    };

    getLEDService(aResp,
                  "/xyz/openbmc_project/led/groups/enclosure_identify_blink",
                  std::move(callback));
}

/**
 * @brief Retrieves identify led group properties over dbus
 *
 * @param[in] aResp     Shared pointer for generating response message.
 *
 * @return None.
 */
inline void
    getLocationIndicatorActive(const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    BMCWEB_LOG_DEBUG << "Get LocationIndicatorActive";
    auto callback = [aResp](const std::string& serviceName) {
        crow::connections::systemBus->async_method_call(
            [aResp, serviceName](const boost::system::error_code ec,
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
                                aResp->res
                                    .jsonValue["LocationIndicatorActive"] =
                                    true;
                            }
                            else
                            {
                                aResp->res
                                    .jsonValue["LocationIndicatorActive"] =
                                    false;
                            }
                        }
                        return;
                    },
                    serviceName,
                    "/xyz/openbmc_project/led/groups/enclosure_identify",
                    "org.freedesktop.DBus.Properties", "Get",
                    "xyz.openbmc_project.Led.Group", "Asserted");
            },
            serviceName,
            "/xyz/openbmc_project/led/groups/enclosure_identify_blink",
            "org.freedesktop.DBus.Properties", "Get",
            "xyz.openbmc_project.Led.Group", "Asserted");
    };

    getLEDService(aResp,
                  "/xyz/openbmc_project/led/groups/enclosure_identify_blink",
                  std::move(callback));
}

/**
 * @brief Sets identify led group properties
 *
 * @param[in] aResp     Shared pointer for generating response message.
 * @param[in] ledState  LED state passed from request
 *
 * @return None.
 */
inline void
    setLocationIndicatorActive(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                               const bool ledState)
{
    BMCWEB_LOG_DEBUG << "Set LocationIndicatorActive";

    auto callback = [aResp, ledState](const std::string& serviceName) {
        crow::connections::systemBus->async_method_call(
            [aResp, ledState,
             serviceName](const boost::system::error_code ec) mutable {
                if (ec)
                {
                    // Some systems may not have enclosure_identify_blink
                    // object so lets set enclosure_identify state also if
                    // enclosure_identify_blink failed
                    crow::connections::systemBus->async_method_call(
                        [aResp, ledState,
                         serviceName](const boost::system::error_code ec2) {
                            if (ec2)
                            {
                                BMCWEB_LOG_DEBUG << "DBUS response error "
                                                 << ec2;
                                messages::internalError(aResp->res);
                                return;
                            }
                        },
                        serviceName,
                        "/xyz/openbmc_project/led/groups/"
                        "enclosure_identify",
                        "org.freedesktop.DBus.Properties", "Set",
                        "xyz.openbmc_project.Led.Group", "Asserted",
                        std::variant<bool>(ledState));
                }
            },
            serviceName,
            "/xyz/openbmc_project/led/groups/enclosure_identify_blink",
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Led.Group", "Asserted",
            std::variant<bool>(ledState));
    };

    getLEDService(aResp,
                  "/xyz/openbmc_project/led/groups/enclosure_identify_blink",
                  std::move(callback));
}

/**
 * @brief Retrieves identify led group properties over D-Bus
 *
 * @param[in] aResp     Shared pointer for generating response message.
 * @param[in] objPath   Object path
 *
 * @return None.
 */
inline void getLocationIndicatorActive(std::shared_ptr<bmcweb::AsyncResp> aResp,
                                       const std::string& objPath)
{
    // TODO Use mapperGetService instead of using hard-coded
    // LED.GroupManager directly in the future
    BMCWEB_LOG_DEBUG << "Get location indicator active";

    crow::connections::systemBus->async_method_call(
        [objPath, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const std::variant<std::vector<std::string>>& resp) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error, ec: " << ec.value();
                return;
            }

            const std::vector<std::string>* endpoints =
                std::get_if<std::vector<std::string>>(&resp);
            if (endpoints == nullptr)
            {
                BMCWEB_LOG_DEBUG << "No endpoints, skipping get location "
                                    "indicator active";
                messages::internalError(aResp->res);
                return;
            }

            for (const auto& endpoint : *endpoints)
            {
                auto callback = [aResp,
                                 endpoint](const std::string& serviceName) {
                    crow::connections::systemBus->async_method_call(
                        [aResp](const boost::system::error_code ec,
                                const std::variant<bool> asserted) {
                            if (ec)
                            {
                                BMCWEB_LOG_ERROR
                                    << "async_method_call failed with ec "
                                    << ec.value();
                                messages::internalError(aResp->res);
                                return;
                            }

                            const bool* ledOn = std::get_if<bool>(&asserted);
                            if (!ledOn)
                            {
                                BMCWEB_LOG_ERROR
                                    << "Fail to get Asserted status ";
                                messages::internalError(aResp->res);
                                return;
                            }

                            aResp->res.jsonValue["LocationIndicatorActive"] =
                                *ledOn;
                        },
                        serviceName, endpoint,
                        "org.freedesktop.DBus.Properties", "Get",
                        "xyz.openbmc_project.Led.Group", "Asserted");
                };
                getLEDService(aResp, endpoint, std::move(callback));
                break;
            }
        },
        "xyz.openbmc_project.ObjectMapper", objPath + "/identify_led_group",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Association", "endpoints");
}

/**
 * @brief Sets identify led group properties
 *
 * @param[in] aResp     Shared pointer for generating response message.
 * @param[in] objPath   Object path
 * @param[in] locationIndicatorActive true or fasle
 *
 * @return None.
 */
inline void setLocationIndicatorActive(std::shared_ptr<bmcweb::AsyncResp> aResp,
                                       const std::string& objPath,
                                       bool locationIndicatorActive)
{
    BMCWEB_LOG_DEBUG << "Set location indicator active";

    crow::connections::systemBus->async_method_call(
        [aResp{std::move(aResp)}, locationIndicatorActive](
            const boost::system::error_code ec,
            const std::variant<std::vector<std::string>>& resp) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error, ec: " << ec.value();
                return;
            }

            const std::vector<std::string>* endpoints =
                std::get_if<std::vector<std::string>>(&resp);
            if (endpoints == nullptr)
            {
                BMCWEB_LOG_DEBUG << "No endpoints, skipping get location "
                                    "indicator active";
                messages::internalError(aResp->res);
                return;
            }

            for (auto& endpoint : *endpoints)
            {
                auto callback = [aResp, endpoint, locationIndicatorActive](
                                    const std::string& serviceName) {
                    crow::connections::systemBus->async_method_call(
                        [aResp, locationIndicatorActive](
                            const boost::system::error_code ec) {
                            if (ec)
                            {
                                BMCWEB_LOG_ERROR
                                    << "async_method_call failed with ec "
                                    << ec.value();
                                messages::internalError(aResp->res);
                                return;
                            }
                        },
                        serviceName, endpoint,
                        "org.freedesktop.DBus.Properties", "Set",
                        "xyz.openbmc_project.Led.Group", "Asserted",
                        std::variant<bool>(locationIndicatorActive));
                };
                getLEDService(aResp, endpoint, std::move(callback));
                break;
            }
        },
        "xyz.openbmc_project.ObjectMapper", objPath + "/identify_led_group",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Association", "endpoints");
}
} // namespace redfish
