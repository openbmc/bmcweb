#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "privileges.hpp"
#include "query.hpp"
#include "utils/json_utils.hpp"

#include <boost/beast/http/verb.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace redfish
{

inline void createSecretKeyUtil(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& username, const std::string& userPath)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, username](const boost::system::error_code& ec,
                              const std::string& secretKey) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("D-Bus response error: {}", ec.value());
                messages::internalError(asyncResp->res);
                return;
            }
            asyncResp->res.jsonValue["SecretKey"] = secretKey;
        },
        "xyz.openbmc_project.User.Manager", userPath,
        "xyz.openbmc_project.User.TOTPAuthenticator", "CreateSecretKey");
}

inline void handleGenerateSecretKey(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& username)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if constexpr (BMCWEB_INSECURE_DISABLE_AUTH)
    {
        // If authentication is disabled, there are no user accounts
        messages::resourceNotFound(asyncResp->res, "ManagerAccount", username);
        return;
    }

    if (req.session == nullptr)
    {
        messages::internalError(asyncResp->res);
        return;
    }
    bool userSelf = (username == req.session->username);

    Privileges effectiveUserPrivileges =
        redfish::getUserPrivileges(*req.session);
    Privileges configureUsers = {"ConfigureUsers"};
    bool userHasConfigureUsers =
        effectiveUserPrivileges.isSupersetOf(configureUsers);

    if (!userHasConfigureUsers && !userSelf)
    {
        BMCWEB_LOG_WARNING("Insufficient Privilege");
        messages::insufficientPrivilege(asyncResp->res);
        return;
    }
    constexpr const char* userDbusPath = "/xyz/openbmc_project/user/";
    sdbusplus::message::object_path tempObjPath(userDbusPath);
    tempObjPath /= username;
    const std::string userPath(tempObjPath);

    createSecretKeyUtil(asyncResp, username, userPath);
}

inline void verifyTotpDbusUtil(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const bool googleAuthEnabled, const std::string& totp,
    const std::string& userPath, std::function<void(bool)>&& callback)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, callback = std::move(callback),
         googleAuthEnabled](const boost::system::error_code& ec, bool status) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("D-Bus response error: {}", ec.value());
                messages::internalError(asyncResp->res);
                callback(false);
                return;
            }
            if (!status)
            {
                messages::actionParameterValueError(
                    asyncResp->res,
                    "ManagerAccount.VerifyTimeBasedOneTimePassword",
                    "TimeBasedOneTimePassword");
                callback(false);
                return;
            }
            if (!googleAuthEnabled)
            {
                callback(false);
                return;
            }
            callback(true);
        },
        "xyz.openbmc_project.User.Manager", userPath,
        "xyz.openbmc_project.User.TOTPAuthenticator", "VerifyOTP", totp);
}

inline void handleVerifyTotpAction(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& totp, const std::string& userPath,
    std::function<void(bool)>&& callback)
{
    dbus::utility::getProperty<std::string>(
        "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
        "xyz.openbmc_project.User.MultiFactorAuthConfiguration", "Enabled",
        [asyncResp, totp, userPath, callback = std::move(callback)](
            const boost::system::error_code& ec,
            const std::string& multiFactorAuthEnabledVal) mutable {
            if (ec)
            {
                BMCWEB_LOG_ERROR(
                    "DBUS response error while fetching MultiFactorAuth property. Error: {}",
                    ec);
                messages::internalError(asyncResp->res);
                callback(false);
                return;
            }

            constexpr std::string_view mfaGoogleAuthDbusVal =
                "xyz.openbmc_project.User.MultiFactorAuthConfiguration.Type.GoogleAuthenticator";
            bool googleAuthEnabled = false;
            if (multiFactorAuthEnabledVal == mfaGoogleAuthDbusVal)
            {
                googleAuthEnabled = true;
            }
            verifyTotpDbusUtil(asyncResp, googleAuthEnabled, totp, userPath,
                               std::move(callback));
        });
}

inline void handleManagerAccountVerifyTotpAction(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& username)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if constexpr (BMCWEB_INSECURE_DISABLE_AUTH)
    {
        // If authentication is disabled, there are no user accounts
        messages::resourceNotFound(asyncResp->res, "ManagerAccount", username);
        return;
    }

    if (req.session == nullptr)
    {
        messages::internalError(asyncResp->res);
        return;
    }
    bool userSelf = (username == req.session->username);

    Privileges effectiveUserPrivileges =
        redfish::getUserPrivileges(*req.session);
    Privileges configureUsers = {"ConfigureUsers"};
    bool userHasConfigureUsers =
        effectiveUserPrivileges.isSupersetOf(configureUsers);

    if (!userHasConfigureUsers && !userSelf)
    {
        messages::insufficientPrivilege(asyncResp->res);
        return;
    }

    std::string totp;
    if (!json_util::readJsonAction(req, asyncResp->res,             //
                                   "TimeBasedOneTimePassword", totp //
                                   ))
    {
        messages::actionParameterMissing(
            asyncResp->res, "ManagerAccount.VerifyTimeBasedOneTimePassword",
            "TimeBasedOneTimePassword");
        return;
    }
    sdbusplus::message::object_path tempObjPath("/xyz/openbmc_project/user/");
    tempObjPath /= username;
    const std::string userPath(tempObjPath);

    handleVerifyTotpAction(
        asyncResp, totp, userPath, [username, session = session](bool success) {
            if (success)
            {
                // Remove existing sessions of the user
                persistent_data::SessionStore::getInstance()
                    .removeSessionsByUsernameExceptSession(username, session);
            }
        });
}

inline void handleClearSecretKeyResponse(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, const sdbusplus::message_t& msg)
{
    if (ec)
    {
        const sd_bus_error* dbusError = msg.get_error();
        if (dbusError != nullptr)
        {
            if (dbusError->name ==
                std::string_view(
                    "xyz.openbmc_project.Common.Error.UnsupportedRequest"))
            {
                BMCWEB_LOG_WARNING("DBUS response error: {}", ec);
                messages::actionNotSupported(asyncResp->res, "ClearSecretKey");
                return;
            }
            if (dbusError->name ==
                std::string_view("xyz.openbmc_project.Common.Error.NotAllowed"))
            {
                BMCWEB_LOG_WARNING("DBUS response error: {}", ec);
                messages::operationNotAllowed(asyncResp->res);
                return;
            }
        }
        BMCWEB_LOG_ERROR("D-Bus response error: {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }
    messages::success(asyncResp->res);
}

inline void clearSecretKeyDbusUtil(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& userPath)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code& ec,
                    const sdbusplus::message_t& msg) {
            handleClearSecretKeyResponse(asyncResp, ec, msg);
        },
        "xyz.openbmc_project.User.Manager", userPath,
        "xyz.openbmc_project.User.TOTPAuthenticator", "ClearSecretKey");
}

inline void handleManagerAccountClearSecretKey(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& username)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if constexpr (BMCWEB_INSECURE_DISABLE_AUTH)
    {
        // If authentication is disabled, there are no user accounts
        messages::resourceNotFound(asyncResp->res, "ManagerAccount", username);
        return;
    }

    if (req.session == nullptr)
    {
        messages::internalError(asyncResp->res);
        return;
    }
    bool userSelf = (username == req.session->username);

    Privileges effectiveUserPrivileges =
        redfish::getUserPrivileges(*req.session);
    Privileges configureUsers = {"ConfigureUsers"};
    bool userHasConfigureUsers =
        effectiveUserPrivileges.isSupersetOf(configureUsers);

    if (!userHasConfigureUsers && !userSelf)
    {
        messages::insufficientPrivilege(asyncResp->res);
        return;
    }

    sdbusplus::message::object_path tempObjPath("/xyz/openbmc_project/user/");
    tempObjPath /= username;
    const std::string userPath(tempObjPath);
    clearSecretKeyDbusUtil(asyncResp, userPath);
}

inline void requestAccountServiceMFARoutes(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/AccountService/Accounts/<str>/Actions/ManagerAccount.GenerateSecretKey")
        // TODO this privilege should be using the generated endpoints, but
        // because of the special handling of ConfigureSelf, it's not able to
        // yet
        .privileges({{"ConfigureUsers"}, {"ConfigureSelf"}})
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleGenerateSecretKey, std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/AccountService/Accounts/<str>/Actions/ManagerAccount.VerifyTimeBasedOneTimePassword")
        // TODO this privilege should be using the generated endpoints, but
        // because of the special handling of ConfigureSelf, it's not able to
        // yet
        .privileges({{"ConfigureUsers"}, {"ConfigureSelf"}})
        .methods(boost::beast::http::verb::post)(std::bind_front(
            handleManagerAccountVerifyTotpAction, std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/AccountService/Accounts/<str>/Actions/ManagerAccount.ClearSecretKey")
        // TODO this privilege should be using the generated endpoints, but
        // because of the special handling of ConfigureSelf, it's not able to
        // yet
        .privileges({{"ConfigureUsers"}, {"ConfigureSelf"}})
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleManagerAccountClearSecretKey, std::ref(app)));
}
} // namespace redfish
