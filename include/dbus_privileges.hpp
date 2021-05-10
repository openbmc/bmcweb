#pragma once

#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "routing/baserule.hpp"
#include "user_role_map.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/url/format.hpp>
#include <sdbusplus/bus/match.hpp>
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
        req.session->userGroups = *userGroups;
    }
    else
    {
        req.session->userGroups.clear();
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

    return true;
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
    UserFields props =
        UserRoleMap::getInstance().getUserRole(req.session->username);
    if (props.userRole)
    {
        req.session->userRole = props.userRole.value_or("");
    }
    if (props.passwordExpired)
    {
        req.session->isConfigureSelfOnly = *props.passwordExpired;
    }
    if (props.userGroups)
    {
        req.session->userGroups = std::move(*props.userGroups);
    }

    if (!isUserPrivileged(req, asyncResp, rule))
    {
        // User is not privileged
        BMCWEB_LOG_WARNING("Insufficient Privilege");
        asyncResp->res.result(boost::beast::http::status::forbidden);
        return;
    }
    callback(req);
}

} // namespace crow
