#pragma once

#include <app.hpp>
#include <query.hpp>
#include <registries/privilege_registry.hpp>
#include <utils/sw_utils.hpp>

using void* (*memset_t)(void*, int, size_t);

static volatile memset_t memsetFunc = memset;

void cleanse(void* ptr, size_t len)
{
    memsetFunc(ptr, 0, len);
}

namespace redfish
{
/**
 * BiosService class supports handle get method for bios.
 */
inline void
    handleBiosServiceGet(crow::App& app, const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Systems/system/Bios";
    asyncResp->res.jsonValue["@odata.type"] = "#Bios.v1_1_0.Bios";
    asyncResp->res.jsonValue["Name"] = "BIOS Configuration";
    asyncResp->res.jsonValue["Description"] = "BIOS Configuration Service";
    asyncResp->res.jsonValue["Id"] = "BIOS";
    asyncResp->res.jsonValue["Actions"]["#Bios.ResetBios"] = {
        {"target", "/redfish/v1/Systems/system/Bios/Actions/Bios.ResetBios"}};

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
inline void
    handleBiosResetPost(crow::App& app, const crow::Request& req,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Failed to reset bios: " << ec;
            messages::internalError(asyncResp->res);
            return;
        }
        },
        "org.open_power.Software.Host.Updater", "/xyz/openbmc_project/software",
        "xyz.openbmc_project.Common.FactoryReset", "Reset");
}

inline void requestRoutesBiosReset(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/Actions/Bios.ResetBios/")
        .privileges(redfish::privileges::postBios)
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleBiosResetPost, std::ref(app)));
}

inline void handleBiosChangePasswordPost(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    std::string currentPassword;
    std::string newPassword;
    std::string userName;

    if (!json_util::readJsonPatch(req, asyncResp->res, "NewPassword",
                                  newPassword, "OldPassword", currentPassword,
                                  "PasswordName", userName))
    {
        return;
    }
    if (currentPassword.empty())
    {
        messages::actionParameterUnknown(asyncResp->res, "ChangePassword",
                                         "OldPassword");
        return;
    }
    if (newPassword.empty())
    {
        messages::actionParameterUnknown(asyncResp->res, "ChangePassword",
                                         "NewPassword");
        return;
    }
    if (userName.empty())
    {
        messages::actionParameterUnknown(asyncResp->res, "ChangePassword",
                                         "PasswordName");
        return;
    }

    // In Intel BIOS, we are not supporting user password in BIOS
    // setup
    if (userName == "UserPassword")
    {
        messages::actionParameterUnknown(asyncResp->res, "ChangePassword",
                                         "PasswordName");

        /* Clear contents of password once used */
        cleanse(&currentPassword[0], currentPassword.size());
        cleanse(&newPassword[0], newPassword.size());

        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec) {
        if (ec)
        {
            BMCWEB_LOG_CRITICAL << "Failed in doPost(BiosChangePassword) "
                                << ec;
            messages::internalError(asyncResp->res);
            return;
        }
        },
        "xyz.openbmc_project.BIOSConfigPassword",
        "/xyz/openbmc_project/bios_config/password",
        "xyz.openbmc_project.BIOSConfig.Password", "ChangePassword", userName,
        currentPassword, newPassword);

    /* Clear contents of password once used */
    cleanse(&currentPassword[0], currentPassword.size());
    cleanse(&newPassword[0], newPassword.size());
}

/**
 * BiosChangePassword class supports handle POST method for change bios
 * password. The class retrieves and sends data directly to D-Bus.
 */
inline void requestRoutesBiosChangePassword(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/<str>/system/Bios/Actions/Bios.ChangePassword/")
        .privileges(redfish::privileges::postBios)
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleBiosChangePasswordPost, std::ref(app)));
}

} // namespace redfish
