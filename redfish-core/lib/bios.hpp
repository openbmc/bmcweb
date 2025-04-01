// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/sw_utils.hpp"

#include <boost/beast/http/verb.hpp>

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
    asyncResp->res.jsonValue["Actions"]["#Bios.ChangePassword"]["target"] =
        std::format("/redfish/v1/Systems/{}/Bios/Actions/Bios.ChangePassword",
                    BMCWEB_REDFISH_SYSTEM_URI_NAME);

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

    crow::connections::systemBus->async_method_call(
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

inline void handleBiosChangePasswordPost(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    [[maybe_unused]] const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    std::string passwordName;
    std::string oldPassword;
    std::string newPassword;
    if (!redfish::json_util::readJsonAction(
            req, asyncResp->res, "PasswordName", passwordName, "OldPassword",
            oldPassword, "NewPassword", newPassword))
    {
        return;
    }

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.BIOSConfig.Password"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project", 0, interfaces,
        [asyncResp, passwordName, oldPassword,
         newPassword](const boost::system::error_code& ec,
                      const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("Failed to find BIOS Password object: {}",
                                     ec);
                    messages::internalError(asyncResp->res);
                }
                return;
            }
            if (subtree.size() != 1)
            {
                BMCWEB_LOG_ERROR("Failed to find BIOS Password object: {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }
            const auto& [path, services] = subtree[0];

            if (services.size() != 1)
            {
                BMCWEB_LOG_ERROR("Failed to find BIOS Password object: {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }
            const auto& [service, interfaces] = services[0];

            crow::connections::systemBus->async_method_call(
                [asyncResp](boost::system::error_code& ec1,
                            sdbusplus::message_t& msg) {
                    if (ec1)
                    {
                        const auto* const error = msg.get_error();
                        if (sd_bus_error_has_name(
                                error,
                                "xyz.openbmc_project.BIOSConfig.Common.Error.InvalidCurrentPassword") !=
                            0)
                        {
                            BMCWEB_LOG_ERROR(
                                "Failed to change password message: {}",
                                error->name);
                            messages::actionParameterValueError(
                                asyncResp->res, "OldPassword",
                                "ChangePassword");
                            return;
                        }

                        messages::internalError(asyncResp->res);
                        return;
                    }
                    messages::success(asyncResp->res);
                    return;
                },
                service, path, "xyz.openbmc_project.BIOSConfig.Password",
                "ChangePassword", passwordName, oldPassword, newPassword);
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
