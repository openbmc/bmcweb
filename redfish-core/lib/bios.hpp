#pragma once

#include "app.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/sw_utils.hpp"

namespace redfish
{

inline void isBiosObjectPresent(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetObject& mapOfServiceAndInterfaces)
{
    if (ec || mapOfServiceAndInterfaces.empty())
    {
        BMCWEB_LOG_WARNING(
            "Failed to GetObject for path /xyz/openbmc_project/bios_config/password. Error : {}",
            ec.what());
        asyncResp->res.result(boost::beast::http::status::not_found);

        return;
    }

    asyncResp->res.jsonValue["Actions"]["#Bios.ChangePassword"]["target"] =
        "/redfish/v1/Systems/system/Bios/Actions/Bios.ChangePassword";
}

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
    if constexpr (bmcwebEnableMultiHost)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
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

    crow::connections::systemBus->async_method_call(
        std::bind_front(isBiosObjectPresent, asyncResp),
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject",
        "/xyz/openbmc_project/bios_config/password",
        std::array<const std::string, 1>{
            "xyz.openbmc_project.BIOSConfig.Password"});

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

    if constexpr (bmcwebEnableMultiHost)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    if (systemName != "system")
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

inline void requestRoutesBiosReset(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/Actions/Bios.ResetBios/")
        .privileges(redfish::privileges::postBios)
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleBiosResetPost, std::ref(app)));
}

inline void
    changePasswordHandler(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const boost::system::error_code& ec)
{
    if (ec)
    {
        BMCWEB_LOG_WARNING("Failed in doPost(BiosChangePassword) {}",
                           ec.what());
        asyncResp->res.result(boost::beast::http::status::not_found);
    }
}

inline void getBiosService(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           std::string& currentPassword,
                           std::string& newPassword, std::string& passwordName,
                           const boost::system::error_code& ec,
                           const dbus::utility::MapperGetObject& resp)
{
    if (ec || resp.empty())
    {
        BMCWEB_LOG_WARNING(
            "DBUS response error during getting of service name: {}",
            ec.what());
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }
    std::string service = resp.begin()->first;

    crow::connections::systemBus->async_method_call(
        std::bind_front(changePasswordHandler, asyncResp), service,
        "/xyz/openbmc_project/bios_config/password",
        "xyz.openbmc_project.BIOSConfig.Password", "ChangePassword",
        passwordName, currentPassword, newPassword);
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
    std::string passwordName;

    if (!json_util::readJsonAction(req, asyncResp->res, "NewPassword",
                                   newPassword, "OldPassword", currentPassword,
                                   "PasswordName", passwordName))
    {
        return;
    }

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.BIOSConfig.Password"};

    dbus::utility::getDbusObject(
        "/xyz/openbmc_project/bios_config/password", interfaces,
        std::bind_front(getBiosService, asyncResp, currentPassword, newPassword,
                        passwordName));
}

/**
 * BiosChangePassword class supports handle POST method for change bios
 * password. The class retrieves and sends data directly to D-Bus.
 */
inline void requestRoutesBiosChangePassword(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/Bios/Actions/Bios.ChangePassword/")
        .privileges(redfish::privileges::postBios)
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleBiosChangePasswordPost, std::ref(app)));
}

} // namespace redfish
