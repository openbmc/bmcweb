/*
Copyright (c) 2019 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "generated/enums/chassis.hpp"
#include "redfish_util.hpp"

#include <boost/system/error_code.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/message/types.hpp>

#include <array>
#include <functional>
#include <memory>
#include <string_view>

namespace redfish
{
static constexpr std::array<std::string_view, 1> ledGroupInterface = {
    "xyz.openbmc_project.Led.Group"};
/**
 * @brief Retrieves identify led group properties over dbus
 *
 * @param[in] asyncResp     Shared pointer for generating response message.
 *
 * @return None.
 */
// TODO (Gunnar): Remove IndicatorLED after enough time has passed
inline void
    getIndicatorLedState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("Get led groups");
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, "xyz.openbmc_project.LED.GroupManager",
        "/xyz/openbmc_project/led/groups/enclosure_identify_blink",
        "xyz.openbmc_project.Led.Group", "Asserted",
        [asyncResp](const boost::system::error_code& ec, const bool blinking) {
            // Some systems may not have enclosure_identify_blink object so
            // proceed to get enclosure_identify state.
            if (ec == boost::system::errc::invalid_argument)
            {
                BMCWEB_LOG_DEBUG(
                    "Get identity blinking LED failed, mismatch in property type");
                messages::internalError(asyncResp->res);
                return;
            }

            // Blinking ON, no need to check enclosure_identify assert.
            if (!ec && blinking)
            {
                asyncResp->res.jsonValue["IndicatorLED"] =
                    chassis::IndicatorLED::Blinking;
                return;
            }

            sdbusplus::asio::getProperty<bool>(
                *crow::connections::systemBus,
                "xyz.openbmc_project.LED.GroupManager",
                "/xyz/openbmc_project/led/groups/enclosure_identify",
                "xyz.openbmc_project.Led.Group", "Asserted",
                [asyncResp](const boost::system::error_code& ec2,
                            const bool ledOn) {
                    if (ec2 == boost::system::errc::invalid_argument)
                    {
                        BMCWEB_LOG_DEBUG(
                            "Get enclosure identity led failed, mismatch in property type");
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    if (ec2)
                    {
                        return;
                    }

                    if (ledOn)
                    {
                        asyncResp->res.jsonValue["IndicatorLED"] =
                            chassis::IndicatorLED::Lit;
                    }
                    else
                    {
                        asyncResp->res.jsonValue["IndicatorLED"] =
                            chassis::IndicatorLED::Off;
                    }
                });
        });
}

/**
 * @brief Sets identify led group properties
 *
 * @param[in] asyncResp     Shared pointer for generating response message.
 * @param[in] ledState  LED state passed from request
 *
 * @return None.
 */
// TODO (Gunnar): Remove IndicatorLED after enough time has passed
inline void
    setIndicatorLedState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& ledState)
{
    BMCWEB_LOG_DEBUG("Set led groups");
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
        messages::propertyValueNotInList(asyncResp->res, ledState,
                                         "IndicatorLED");
        return;
    }

    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, "xyz.openbmc_project.LED.GroupManager",
        "/xyz/openbmc_project/led/groups/enclosure_identify_blink",
        "xyz.openbmc_project.Led.Group", "Asserted", ledBlinkng,
        [asyncResp, ledOn,
         ledBlinkng](const boost::system::error_code& ec) mutable {
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
            setDbusProperty(
                asyncResp, "IndicatorLED",
                "xyz.openbmc_project.LED.GroupManager",
                sdbusplus::message::object_path(
                    "/xyz/openbmc_project/led/groups/enclosure_identify"),
                "xyz.openbmc_project.Led.Group", "Asserted", ledBlinkng);
        });
}

/**
 * @brief Retrieves identify system led group properties over dbus
 *
 * @param[in] asyncResp     Shared pointer for generating response message.
 *
 * @return None.
 */
inline void getSystemLocationIndicatorActive(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("Get LocationIndicatorActive");
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, "xyz.openbmc_project.LED.GroupManager",
        "/xyz/openbmc_project/led/groups/enclosure_identify_blink",
        "xyz.openbmc_project.Led.Group", "Asserted",
        [asyncResp](const boost::system::error_code& ec, const bool blinking) {
            // Some systems may not have enclosure_identify_blink object so
            // proceed to get enclosure_identify state.
            if (ec == boost::system::errc::invalid_argument)
            {
                BMCWEB_LOG_DEBUG(
                    "Get identity blinking LED failed, mismatch in property type");
                messages::internalError(asyncResp->res);
                return;
            }

            // Blinking ON, no need to check enclosure_identify assert.
            if (!ec && blinking)
            {
                asyncResp->res.jsonValue["LocationIndicatorActive"] = true;
                return;
            }

            sdbusplus::asio::getProperty<bool>(
                *crow::connections::systemBus,
                "xyz.openbmc_project.LED.GroupManager",
                "/xyz/openbmc_project/led/groups/enclosure_identify",
                "xyz.openbmc_project.Led.Group", "Asserted",
                [asyncResp](const boost::system::error_code& ec2,
                            const bool ledOn) {
                    if (ec2 == boost::system::errc::invalid_argument)
                    {
                        BMCWEB_LOG_DEBUG(
                            "Get enclosure identity led failed, mismatch in property type");
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    if (ec2)
                    {
                        return;
                    }

                    asyncResp->res.jsonValue["LocationIndicatorActive"] = ledOn;
                });
        });
}

/**
 * @brief Sets identify system led group properties
 *
 * @param[in] asyncResp     Shared pointer for generating response message.
 * @param[in] ledState  LED state passed from request
 *
 * @return None.
 */
inline void setSystemLocationIndicatorActive(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const bool ledState)
{
    BMCWEB_LOG_DEBUG("Set LocationIndicatorActive");

    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, "xyz.openbmc_project.LED.GroupManager",
        "/xyz/openbmc_project/led/groups/enclosure_identify_blink",
        "xyz.openbmc_project.Led.Group", "Asserted", ledState,
        [asyncResp, ledState](const boost::system::error_code& ec) {
            if (ec)
            {
                // Some systems may not have enclosure_identify_blink object so
                // lets set enclosure_identify state also if
                // enclosure_identify_blink failed
                setDbusProperty(
                    asyncResp, "LocationIndicatorActive",
                    "xyz.openbmc_project.LED.GroupManager",
                    sdbusplus::message::object_path(
                        "/xyz/openbmc_project/led/groups/enclosure_identify"),
                    "xyz.openbmc_project.Led.Group", "Asserted", ledState);
            }
        });
}

inline void afterGetLedGroupPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& objPath, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree,
    const std::function<void(const std::string& ledGroupPath,
                             const std::string& service)>& callback)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR(
                "DBUS response error for getAssociatedSubTreePaths: {}",
                ec.value());
            messages::internalError(asyncResp->res);
        }
        return;
    }

    if (subtree.empty())
    {
        BMCWEB_LOG_DEBUG(
            "No LED group associated with the specified object path: {}",
            objPath);
        return;
    }

    if (subtree.size() > 1)
    {
        BMCWEB_LOG_ERROR(
            "More than one LED group associated with the object: {}",
            subtree.size());
        messages::internalError(asyncResp->res);
        return;
    }

    const auto& [ledGroupPath, serviceMap] = *subtree.begin();
    const auto& [service, interfaces] = *serviceMap.begin();
    callback(ledGroupPath, service);
}

inline void getLedGroupPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& objPath,
    const std::function<void(const std::string& ledGroupPath,
                             const std::string& service)>& callback)
{
    static constexpr const char* ledObjectPath =
        "/xyz/openbmc_project/led/groups";
    sdbusplus::message::object_path ledGroupAssociatedPath =
        objPath + "/identifying";

    dbus::utility::getAssociatedSubTree(
        ledGroupAssociatedPath, sdbusplus::message::object_path(ledObjectPath),
        0, ledGroupInterface,
        [asyncResp, objPath,
         callback](const boost::system::error_code& ec,
                   const dbus::utility::MapperGetSubTreeResponse& subtree) {
            afterGetLedGroupPath(asyncResp, objPath, ec, subtree, callback);
        });
}

inline void getLedState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& ledGroupPath,
                        const std::string& service)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, service, ledGroupPath,
        "xyz.openbmc_project.Led.Group", "Asserted",
        [asyncResp,
         ledGroupPath](const boost::system::error_code& ec, bool assert) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("DBUS response error for get ledState {}",
                                     ec.value());
                    messages::internalError(asyncResp->res);
                    return;
                }
                return;
            }

            asyncResp->res.jsonValue["LocationIndicatorActive"] = assert;
        });
}

/**
 * @brief Retrieves identify led group properties over dbus
 *
 * @param[in] asyncResp Shared pointer for generating response
 * message.
 * @param[in] objPath   Object path on PIM
 *
 * @return None.
 */
inline void getLocationIndicatorActive(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& objPath)
{
    BMCWEB_LOG_DEBUG("Get LocationIndicatorActive");
    getLedGroupPath(asyncResp, objPath,
                    std::bind_front(getLedState, asyncResp));
}

/**
 * @brief Sets identify led group properties
 *
 * @param[in] asyncResp     Shared pointer for generating response
 * message.
 * @param[in] objPath       Object path on PIM
 * @param[in] ledState      LED state passed from request
 *
 * @return None.
 */
inline void setLocationIndicatorActive(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& objPath, bool ledState)
{
    BMCWEB_LOG_DEBUG("Set LocationIndicatorActive");
    getLedGroupPath(asyncResp, objPath,
                    [asyncResp, ledState](const std::string& ledGroupPath,
                                          const std::string& service) {
                        setDbusProperty(asyncResp, "Asserted", service,
                                        ledGroupPath,
                                        "xyz.openbmc_project.Led.Group",
                                        "LedGroup", ledState);
                    });
}

} // namespace redfish
