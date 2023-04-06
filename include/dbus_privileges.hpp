#pragma once

#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"

inline bool isUserPrivileged(
    Request& req, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    BaseRule& rule, const dbus::utility::DBusPropertiesMap& userInfoMap)
{
    std::string userRole{};
    const std::string* userRolePtr = nullptr;
    const bool* remoteUser = nullptr;
    const bool* passwordExpired = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        redfish::dbus_utils::UnpackErrorPrinter(), userInfoMap, "UserPrivilege",
        userRolePtr, "RemoteUser", remoteUser, "UserPasswordExpired",
        passwordExpired);

    if (!success)
    {
        asyncResp->res.result(
            boost::beast::http::status::internal_server_error);
        return false;
    }

    if (userRolePtr != nullptr)
    {
        userRole = *userRolePtr;
        BMCWEB_LOG_DEBUG << "userName = " << req.session->username
                         << " userRole = " << *userRolePtr;
    }

    if (remoteUser == nullptr)
    {
        BMCWEB_LOG_ERROR << "RemoteUser property missing or wrong type";
        asyncResp->res.result(
            boost::beast::http::status::internal_server_error);
        return false;
    }
    bool expired = false;
    if (passwordExpired == nullptr)
    {
        if (!*remoteUser)
        {
            BMCWEB_LOG_ERROR << "UserPasswordExpired property is expected for"
                                " local user but is missing or wrong type";
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            return false;
        }
    }
    else
    {
        expired = *passwordExpired;
    }

    // Get the user's privileges from the role
    redfish::Privileges userPrivileges = redfish::getUserPrivileges(userRole);

    // Set isConfigureSelfOnly based on D-Bus results.  This
    // ignores the results from both pamAuthenticateUser and the
    // value from any previous use of this session.
    req.session->isConfigureSelfOnly = expired;

    // Modify privileges if isConfigureSelfOnly.
    if (req.session->isConfigureSelfOnly)
    {
        // Remove all privileges except ConfigureSelf
        userPrivileges =
            userPrivileges.intersection(redfish::Privileges{"ConfigureSelf"});
        BMCWEB_LOG_DEBUG << "Operation limited to ConfigureSelf";
    }

    if (!rule.checkPrivileges(userPrivileges))
    {
        asyncResp->res.result(boost::beast::http::status::forbidden);
        if (req.session->isConfigureSelfOnly)
        {
            redfish::messages::passwordChangeRequired(
                asyncResp->res, crow::utility::urlFromPieces(
                                    "redfish", "v1", "AccountService",
                                    "Accounts", req.session->username));
        }
        return false;
    }

    req.userRole = userRole;

    return true;
}

template <typename CallbackFn>
void afterGetUserInfo(Request& req,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      BaseRule& rule, CallbackFn&& callback,
                      const boost::system::error_code& ec,
                      const dbus::utility::DBusPropertiesMap& userInfoMap)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR << "GetUserInfo failed...";
        asyncResp->res.result(
            boost::beast::http::status::internal_server_error);
        return;
    }

    if (!Router::isUserPrivileged(req, asyncResp, rule, userInfoMap))
    {
        // User is not privileged
        BMCWEB_LOG_ERROR << "Insufficient Privilege";
        asyncResp->res.result(boost::beast::http::status::forbidden);
        return;
    }
    callback(req);
}

template <typename CallbackFn>
void validatePrivilege(Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       BaseRule& rule, CallbackFn&& callback)
{
    if (req.session == nullptr)
    {
        return;
    }
    std::string username = req.session->username;
    crow::connections::systemBus->async_method_call(
        [this, &req, asyncResp, &rule,
         callback(std::forward<CallbackFn>(callback))](
            const boost::system::error_code& ec,
            const dbus::utility::DBusPropertiesMap& userInfoMap) mutable {
        afterGetUserInfo(req, asyncResp, rule,
                         std::forward<CallbackFn>(callback), ec, userInfoMap);
        },
        "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
        "xyz.openbmc_project.User.Manager", "GetUserInfo", username);
}
