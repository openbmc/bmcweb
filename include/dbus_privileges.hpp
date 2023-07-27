#pragma once

#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "routing/baserule.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/url/format.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <memory>
#include <vector>

namespace crow
{
// Populate session with user information.
inline bool
    populateUserInfo(Request& req,
                     const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const dbus::utility::DBusPropertiesMap& userInfoMap)
{
    const std::string* userRolePtr = nullptr;
    const bool* remoteUser = nullptr;
    const bool* passwordExpired = nullptr;
    const std::vector<std::string>* userGroups = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        redfish::dbus_utils::UnpackErrorPrinter(), userInfoMap, "UserPrivilege",
        userRolePtr, "RemoteUser", remoteUser, "UserPasswordExpired",
        passwordExpired, "UserGroups", userGroups);

    if (!success)
    {
        BMCWEB_LOG_ERROR("Failed to unpack user properties.");
        asyncResp->res.result(
            boost::beast::http::status::internal_server_error);
        return false;
    }

    if (req.session == nullptr)
    {
        return false;
    }

    if (userRolePtr != nullptr)
    {
        req.session->userRole = *userRolePtr;
        BMCWEB_LOG_DEBUG("userName = {} userRole = {}", req.session->username,
                         *userRolePtr);
    }

    if (remoteUser == nullptr)
    {
        BMCWEB_LOG_ERROR("RemoteUser property missing or wrong type");
        asyncResp->res.result(
            boost::beast::http::status::internal_server_error);
        return false;
    }
    bool expired = false;
    if (passwordExpired == nullptr)
    {
        if (!*remoteUser)
        {
            BMCWEB_LOG_ERROR("UserPasswordExpired property is expected for"
                             " local user but is missing or wrong type");
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            return false;
        }
    }
    else
    {
        expired = *passwordExpired;
    }

    // Set isConfigureSelfOnly based on D-Bus results.  This
    // ignores the results from both pamAuthenticateUser and the
    // value from any previous use of this session.
    req.session->isConfigureSelfOnly = expired;

    if (userGroups != nullptr)
    {
        // Populate session with user groups.
        for (const auto& userGroup : *userGroups)
        {
            req.session->userGroups.emplace_back(userGroup);
        }
    }

    return true;
}

inline bool
    isUserPrivileged(Request& req,
                     const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     BaseRule& rule)
{
    if (req.session == nullptr)
    {
        return false;
    }
    // Get the user's privileges from the role
    redfish::Privileges userPrivileges =
        redfish::getUserPrivileges(*req.session);

    // Modify privileges if isConfigureSelfOnly.
    if (req.session->isConfigureSelfOnly)
    {
        // Remove all privileges except ConfigureSelf
        userPrivileges =
            userPrivileges.intersection(redfish::Privileges{"ConfigureSelf"});
        BMCWEB_LOG_DEBUG("Operation limited to ConfigureSelf");
    }

    if (!rule.checkPrivileges(userPrivileges))
    {
        asyncResp->res.result(boost::beast::http::status::forbidden);
        if (req.session->isConfigureSelfOnly)
        {
            redfish::messages::passwordChangeRequired(
                asyncResp->res,
                boost::urls::format("/redfish/v1/AccountService/Accounts/{}",
                                    req.session->username));
        }
        return false;
    }

    req.userRole = req.session->userRole;
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
        BMCWEB_LOG_ERROR("GetUserInfo failed...");
        asyncResp->res.result(
            boost::beast::http::status::internal_server_error);
        return;
    }

    if (!populateUserInfo(req, asyncResp, userInfoMap))
    {
        BMCWEB_LOG_ERROR("Failed to populate user information");
        asyncResp->res.result(
            boost::beast::http::status::internal_server_error);
        return;
    }

    if (!isUserPrivileged(req, asyncResp, rule))
    {
        // User is not privileged
        BMCWEB_LOG_ERROR("Insufficient Privilege");
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
        [&req, asyncResp, &rule, callback(std::forward<CallbackFn>(callback))](
            const boost::system::error_code& ec,
            const dbus::utility::DBusPropertiesMap& userInfoMap) mutable {
        afterGetUserInfo(req, asyncResp, rule,
                         std::forward<CallbackFn>(callback), ec, userInfoMap);
        },
        "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
        "xyz.openbmc_project.User.Manager", "GetUserInfo", username);
}

} // namespace crow
