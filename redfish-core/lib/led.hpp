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

#include <app.hpp>
#include <sdbusplus/asio/property.hpp>

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
inline void
    getIndicatorLedState(const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    BMCWEB_LOG_DEBUG << "Get led groups";
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, "xyz.openbmc_project.LED.GroupManager",
        "/xyz/openbmc_project/led/groups/enclosure_identify_blink",
        "xyz.openbmc_project.Led.Group", "Asserted",
        [aResp](const boost::system::error_code ec, bool blinking) {
        // Some systems may not have enclosure_identify_blink object so
        // proceed to get enclosure_identify state.
        if (ec == boost::system::errc::invalid_argument)
        {
            BMCWEB_LOG_DEBUG
                << "Get identity blinking LED failed, missmatch in property type";
            messages::internalError(aResp->res);
            return;
        }

        // Blinking ON, no need to check enclosure_identify assert.
        if (!ec && blinking)
        {
            aResp->res.jsonValue["IndicatorLED"] = "Blinking";
            return;
        }

        sdbusplus::asio::getProperty<bool>(
            *crow::connections::systemBus,
            "xyz.openbmc_project.LED.GroupManager",
            "/xyz/openbmc_project/led/groups/enclosure_identify",
            "xyz.openbmc_project.Led.Group", "Asserted",
            [aResp](const boost::system::error_code ec2, bool ledOn) {
            if (ec2 == boost::system::errc::invalid_argument)
            {
                BMCWEB_LOG_DEBUG
                    << "Get enclosure identity led failed, missmatch in property type";
                messages::internalError(aResp->res);
                return;
            }

            if (ec2)
            {
                return;
            }

            if (ledOn)
            {
                aResp->res.jsonValue["IndicatorLED"] = "Lit";
            }
            else
            {
                aResp->res.jsonValue["IndicatorLED"] = "Off";
            }
            });
        });
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
            messages::success(aResp->res);
            },
            "xyz.openbmc_project.LED.GroupManager",
            "/xyz/openbmc_project/led/groups/enclosure_identify",
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Led.Group", "Asserted",
            dbus::utility::DbusVariantType(ledOn));
        },
        "xyz.openbmc_project.LED.GroupManager",
        "/xyz/openbmc_project/led/groups/enclosure_identify_blink",
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Led.Group", "Asserted",
        dbus::utility::DbusVariantType(ledBlinkng));
}

inline void getLedAsset(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                        const std::string& ledGroup)
{
    auto respHanler = [aResp, ledGroup](const std::string& service) {
        sdbusplus::asio::getProperty<bool>(
            *crow::connections::systemBus, service, ledGroup,
            "xyz.openbmc_project.Led.Group", "Asserted",
            [aResp](const boost::system::error_code ec, bool assert) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }

            aResp->res.jsonValue["LocationIndicatorActive"] = assert;
            });
    };

    std::vector<std::string> interfaces{"xyz.openbmc_project.Led.Group"};
    dbus::utility::getDbusObject(aResp, ledGroup, interfaces,
                                 std::move(respHanler));
}

inline void setLedAsset(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                        const std::string& ledGroup, bool ledState)
{
    auto respHanler = [aResp, ledGroup, ledState](const std::string& service) {
        crow::connections::systemBus->async_method_call(
            [aResp](const boost::system::error_code ec) {
            if (ec)
            {
                messages::internalError(aResp->res);
                return;
            }
            },
            service, ledGroup, "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Led.Group", "Asserted",
            dbus::utility::DbusVariantType(ledState));
    };

    std::vector<std::string> interfaces{"xyz.openbmc_project.Led.Group"};
    dbus::utility::getDbusObject(aResp, ledGroup, interfaces,
                                 std::move(respHanler));
}

inline void
    getLedGroup(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                const std::string& objPath,
                const std::function<void(const std::string&)>&& callback)
{
    auto respHandler = [objPath, callback](const std::string& service) {
        sdbusplus::asio::getProperty<
            dbus::utility::MapperGetAssociationResponse>(
            *crow::connections::systemBus, service, objPath,
            "xyz.openbmc_project.Association.Definitions", "Associations",
            [callback](const boost::system::error_code ec,
                       const dbus::utility::MapperGetAssociationResponse&
                           associations) {
            if (ec)
            {
                return;
            }

            for (const auto& [rType, tType, ledGroup] : associations)
            {
                if (rType == "identify_led_group")
                {
                    callback(ledGroup);
                }
            }
            });
    };

    std::vector<std::string> interfaces{
        "xyz.openbmc_project.Association.Definitions"};
    dbus::utility::getDbusObject(aResp, objPath, interfaces,
                                 std::move(respHandler));
}

/**
 * @brief Retrieves identify led group properties over dbus
 *
 * @param[in] aResp     Shared pointer for generating response message.
 * @param[in] objPath   Object path on PIM
 *
 * @return None.
 */
inline void
    getLocationIndicatorActive(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                               const std::string& objPath)
{
    BMCWEB_LOG_DEBUG << "Get LocationIndicatorActive";
    auto respHandler = [aResp](const std::string& ledGroup) {
        getLedAsset(aResp, ledGroup);
    };

    getLedGroup(aResp, objPath, std::move(respHandler));
}

/**
 * @brief Sets identify led group properties
 *
 * @param[in] aResp     Shared pointer for generating response message.
 * @param[in] objPath   Object path on PIM
 * @param[in] ledState  LED state passed from request
 *
 * @return None.
 */
inline void
    setLocationIndicatorActive(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                               const std::string& objPath, bool ledState)
{
    BMCWEB_LOG_DEBUG << "Set LocationIndicatorActive";

    auto respHandler = [aResp, ledState](const std::string& ledGroup) {
        setLedAsset(aResp, ledGroup, ledState);
    };

    getLedGroup(aResp, objPath, std::move(respHandler));
}
} // namespace redfish
