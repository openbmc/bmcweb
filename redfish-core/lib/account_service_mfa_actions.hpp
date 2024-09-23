#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "privileges.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/json_utils.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/message.hpp>

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
            asyncResp->res.jsonValue["@odata.type"] =
                "#ManagerAccount.v1_13_0.ManagerAccount";
            asyncResp->res.jsonValue["Name"] = "User Account";
            asyncResp->res.jsonValue["Description"] = "User Account";
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
    sdbusplus::message::object_path tempObjPath(rootUserDbusPath);
    tempObjPath /= username;
    const std::string userPath(tempObjPath);

    createSecretKeyUtil(asyncResp, username, userPath);
}

inline void
    verifyTotpDbusUtil(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& totp, const std::string& userPath,
                       std::function<void(bool)>&& callback)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, "xyz.openbmc_project.User.Manager",
        "/xyz/openbmc_project/user",
        "xyz.openbmc_project.User.MultiFactorAuthConfiguration", "Enabled",
        [asyncResp, totp, userPath, callback = std::move(callback)](
            const boost::system::error_code& ec,
            const std::string& multiFactorAuthEnabledVal) {
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
            bool googleAuthEnabled =
                (multiFactorAuthEnabledVal == mfaGoogleAuthDbusVal);

            crow::connections::systemBus->async_method_call(
                [asyncResp, callback = std::move(callback), googleAuthEnabled](
                    const boost::system::error_code& ec1, bool status) {
                    if (ec1)
                    {
                        BMCWEB_LOG_ERROR("D-Bus response error: {}",
                                         ec1.value());
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
                        messages::success(asyncResp->res);
                        callback(false);
                        return;
                    }
                    messages::success(asyncResp->res);
                    callback(true);
                },
                "xyz.openbmc_project.User.Manager", userPath,
                "xyz.openbmc_project.User.TOTPAuthenticator", "VerifyOTP",
                totp);
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
    if (!json_util::readJsonAction(req, asyncResp->res,
                                   "TimeBasedOneTimePassword", totp))
    {
        messages::actionParameterMissing(
            asyncResp->res, "ManagerAccount.VerifyTimeBasedOneTimePassword",
            "TimeBasedOneTimePassword");
        return;
    }
    sdbusplus::message::object_path tempObjPath("/xyz/openbmc_project/user/");
    tempObjPath /= username;
    const std::string userPath(tempObjPath);
    auto verifyTotpCallback = [username, req](bool success) {
        if (success)
        {
            // Remove existing sessions of the user
            persistent_data::SessionStore::getInstance()
                .removeSessionsByUsernameExceptSession(username, req.session);
        }
    };
    verifyTotpDbusUtil(asyncResp, totp, userPath,
                       std::move(verifyTotpCallback));
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
}
} // namespace redfish
