// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2019 Intel Corporation
#pragma once

#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/chassis.hpp"
#include "logging.hpp"
#include "utils/dbus_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/system/error_code.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <array>
#include <functional>
#include <memory>
#include <string_view>
#include <utility>

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
inline void getIndicatorLedState(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("Get led groups");
    dbus::utility::getProperty<bool>(
        "xyz.openbmc_project.LED.GroupManager",
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

            dbus::utility::getProperty<bool>(
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
inline void setIndicatorLedState(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
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
    dbus::utility::getProperty<bool>(
        "xyz.openbmc_project.LED.GroupManager",
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

            dbus::utility::getProperty<bool>(
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

inline void handleLedGroupSubtree(
    const std::string& objPath, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree,
    const std::function<void(const boost::system::error_code& ec,
                             const std::string& ledGroupPath,
                             const std::string& service)>& callback)
{
    if (ec)
    {
        // Callback will handle the error
        callback(ec, "", "");
        return;
    }

    if (subtree.empty())
    {
        // Callback will handle the error
        BMCWEB_LOG_DEBUG(
            "No LED group associated with the specified object path: {}",
            objPath);
        callback(ec, "", "");
        return;
    }

    if (subtree.size() > 1)
    {
        // Callback will handle the error
        BMCWEB_LOG_DEBUG(
            "More than one LED group associated with the object {}: {}",
            objPath, subtree.size());
        callback(ec, "", "");
        return;
    }

    const auto& [ledGroupPath, serviceMap] = *subtree.begin();
    const auto& [service, interfaces] = *serviceMap.begin();
    callback(ec, ledGroupPath, service);
}

inline void getLedGroupPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& objPath,
    std::function<void(const boost::system::error_code& ec,
                       const std::string& ledGroupPath,
                       const std::string& service)>&& callback)
{
    static constexpr const char* ledObjectPath =
        "/xyz/openbmc_project/led/groups";
    sdbusplus::message::object_path ledGroupAssociatedPath =
        objPath + "/identifying";

    dbus::utility::getAssociatedSubTree(
        ledGroupAssociatedPath, sdbusplus::message::object_path(ledObjectPath),
        0, ledGroupInterface,
        [asyncResp, objPath, callback{std::move(callback)}](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
            handleLedGroupSubtree(objPath, ec, subtree, callback);
        });
}

inline void afterGetLedState(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, bool assert)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error for get ledState {}",
                             ec.value());
            messages::internalError(asyncResp->res);
        }
        return;
    }

    asyncResp->res.jsonValue["LocationIndicatorActive"] = assert;
}

inline void getLedState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const boost::system::error_code& ec,
                        const std::string& ledGroupPath,
                        const std::string& service)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
            messages::internalError(asyncResp->res);
        }
        return;
    }

    if (ledGroupPath.empty() || service.empty())
    {
        // No LED group associated, not an error
        return;
    }

    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, service, ledGroupPath,
        "xyz.openbmc_project.Led.Group", "Asserted",
        std::bind_front(afterGetLedState, asyncResp));
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
    BMCWEB_LOG_DEBUG("Get LocationIndicatorActive for {}", objPath);
    getLedGroupPath(asyncResp, objPath,
                    [asyncResp](const boost::system::error_code& ec,
                                const std::string& ledGroupPath,
                                const std::string& service) {
                        getLedState(asyncResp, ec, ledGroupPath, service);
                    });
}

inline void setLedState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        bool ledState, const boost::system::error_code& ec,
                        const std::string& ledGroupPath,
                        const std::string& service)
{
    if (ec)
    {
        if (ec.value() == EBADR)
        {
            messages::propertyUnknown(asyncResp->res,
                                      "LocationIndicatorActive");
            return;
        }
        BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    if (ledGroupPath.empty() || service.empty())
    {
        messages::propertyUnknown(asyncResp->res, "LocationIndicatorActive");
        return;
    }

    setDbusProperty(asyncResp, "LocationIndicatorActive", service, ledGroupPath,
                    "xyz.openbmc_project.Led.Group", "Asserted", ledState);
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
    BMCWEB_LOG_DEBUG("Set LocationIndicatorActive for {}", objPath);
    getLedGroupPath(
        asyncResp, objPath,
        [asyncResp, ledState](const boost::system::error_code& ec,
                              const std::string& ledGroupPath,
                              const std::string& service) {
            setLedState(asyncResp, ledState, ec, ledGroupPath, service);
        });
}

} // namespace redfish
