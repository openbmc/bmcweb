// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/json_utils.hpp"
#include "utils/sw_utils.hpp"

#include <systemd/sd-bus.h>

#include <boost/beast/http/verb.hpp>
#include <sdbusplus/message.hpp>

#include <array>
#include <format>
#include <functional>
#include <memory>
#include <string>

namespace redfish
{
/**
 * BiosService class supports handle get method for bios.
 */
inline void handleBiosServiceGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    asyncResp->res.jsonValue["@odata.id"] = std::format(
        "/redfish/v1/Systems/{}/Bios", BMCWEB_REDFISH_SYSTEM_URI_NAME);
    asyncResp->res.jsonValue["@odata.type"] = "#Bios.v1_1_0.Bios";
    asyncResp->res.jsonValue["Name"] = "BIOS Configuration";
    asyncResp->res.jsonValue["Description"] = "BIOS Configuration Service";
    asyncResp->res.jsonValue["Id"] = "BIOS";
    asyncResp->res.jsonValue["Actions"]["#Bios.ResetBios"]["target"] =
        std::format("/redfish/v1/Systems/{}/Bios/Actions/Bios.ResetBios",
                    BMCWEB_REDFISH_SYSTEM_URI_NAME);
    dbus::utility::checkDbusPathExists(
        "/xyz/openbmc_project/bios_config/password", [asyncResp](int rc) {
            if (rc > 0)
            {
                asyncResp->res.jsonValue["Actions"]["#Bios.ChangePassword"]
                                        ["target"] = std::format(
                    "/redfish/v1/Systems/{}/Bios/Actions/Bios.ChangePassword",
                    BMCWEB_REDFISH_SYSTEM_URI_NAME);
            }
        });

    // Get the ActiveSoftwareImage and SoftwareImages
    sw_util::populateSoftwareInformation(asyncResp, sw_util::biosPurpose, "",
                                         true);
}

inline void requestRoutesBiosService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/")
        .privileges(redfish::privileges::getBios)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleBiosServiceGet, std::ref(app)));
}

/**
 * BiosReset class supports handle POST method for Reset bios.
 * The class retrieves and sends data directly to D-Bus.
 *
 * Function handles POST method request.
 * Analyzes POST body message before sends Reset request data to D-Bus.
 */
inline void handleBiosResetPost(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    dbus::utility::async_method_call(
        asyncResp,
        [asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("Failed to reset bios: {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }
        },
        "org.open_power.Software.Host.Updater", "/xyz/openbmc_project/software",
        "xyz.openbmc_project.Common.FactoryReset", "Reset");
}

inline void afterBiosPasswordChangeObjectResponse(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& service, const std::string& passwordName,
    const std::string& oldPassword, const std::string& newPassword)
{
    dbus::utility::async_method_call(
        asyncResp,
        [asyncResp](const boost::system::error_code& ec,
                    sdbusplus::message_t& msg) {
            if (ec)
            {
                const sd_bus_error* dbusError = msg.get_error();
                if (dbusError != nullptr)
                {
                    if (std::string_view(
                            "xyz.openbmc_project.BIOSConfig.Common.Error.InvalidCurrentPassword") ==
                        dbusError->name)
                    {
                        messages::actionParameterValueError(
                            asyncResp->res, "OldPassword", "ChangePassword");
                        return;
                    }
                }
                BMCWEB_LOG_ERROR("DBUS response error: {}", ec.message());
                messages::internalError(asyncResp->res);
                return;
            }
            messages::success(asyncResp->res);
            return;
        },
        service, "/xyz/openbmc_project/bios_config/password",
        "xyz.openbmc_project.BIOSConfig.Password", "ChangePassword",
        passwordName, oldPassword, newPassword);
}

inline void handleBiosChangePasswordPost(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    [[maybe_unused]] const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    std::string passwordName;
    std::string oldPassword;
    std::string newPassword;
    if (!json_util::readJsonAction(req, asyncResp->res, "PasswordName",
                                   passwordName, "OldPassword", oldPassword,
                                   "NewPassword", newPassword))
    {
        return;
    }

    constexpr std::array<std::string_view, 1> biosPasswordInterfaces = {
        "xyz.openbmc_project.BIOSConfig.Password"};
    dbus::utility::getDbusObject(
        "/xyz/openbmc_project/bios_config/password", biosPasswordInterfaces,
        [asyncResp, passwordName, oldPassword,
         newPassword](const boost::system::error_code& ec,
                      const dbus::utility::MapperGetObject& objType) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("Failed to get BIOS Password object: {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }
            if (objType.empty())
            {
                BMCWEB_LOG_ERROR("BIOS Password object not found");
                messages::internalError(asyncResp->res);
                return;
            }
            const std::string& objectPath = objType.begin()->first;
            afterBiosPasswordChangeObjectResponse(
                asyncResp, objectPath, passwordName, oldPassword, newPassword);
        });
}

inline void requestRoutesBiosReset(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/Actions/Bios.ResetBios/")
        .privileges(redfish::privileges::postBios)
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleBiosResetPost, std::ref(app)));
}

inline void requestRoutesBiosChangePassword(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/Bios/Actions/Bios.ChangePassword/")
        .privileges(redfish::privileges::postBios)
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleBiosChangePasswordPost, std::ref(app)));
}

} // namespace redfish
