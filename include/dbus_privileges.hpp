// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "privileges.hpp"
#include "routing/baserule.hpp"
#include "sessions.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/beast/http/status.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace crow
{
// Populate session with user information.
inline bool populateUserInfo(
    persistent_data::UserSession& session,
    const dbus::utility::DBusPropertiesMap& userInfoMap)
{
    std::string userRole;
    bool remoteUser = false;
    std::optional<bool> passwordExpired;
    std::optional<bool> secretKeyRequired;
    std::optional<std::vector<std::string>> userGroups;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        redfish::dbus_utils::UnpackErrorPrinter(), userInfoMap, "UserPrivilege",
        userRole, "RemoteUser", remoteUser, "UserPasswordExpired",
        passwordExpired, "TOTPSecretkeyRequired", secretKeyRequired,
        "UserGroups", userGroups);

    if (!success)
    {
        BMCWEB_LOG_ERROR("Failed to unpack user properties.");
        return false;
    }

    if (!remoteUser && (!passwordExpired || !userGroups))
    {
        BMCWEB_LOG_ERROR(
            "Missing UserPasswordExpired or UserGroups property for local user");
        return false;
    }

    session.userRole = userRole;
    BMCWEB_LOG_DEBUG("userName = {} userRole = {}", session.username, userRole);

    session.isGenerateSecretKeyRequired = secretKeyRequired.value_or(false);

    // Set isConfigureSelfOnly based on D-Bus results.  This
    // ignores the results from both pamAuthenticateUser and the
    // value from any previous use of this session.
    session.isConfigureSelfOnly =
        passwordExpired.value_or(false) || secretKeyRequired.value_or(false);

    session.userGroups.clear();
    if (userGroups)
    {
        session.userGroups.swap(*userGroups);
    }

    return true;
}

inline bool isUserPrivileged(
    Request& req, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
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
            if (!req.session->isGenerateSecretKeyRequired)
            {
                redfish::messages::passwordChangeRequired(
                    asyncResp->res,
                    boost::urls::format(
                        "/redfish/v1/AccountService/Accounts/{}",
                        req.session->username));
            }
            else
            {
                redfish::messages::generateSecretKeyRequired(
                    asyncResp->res,
                    boost::urls::format(
                        "/redfish/v1/AccountService/Accounts/{}",
                        req.session->username));
            }
        }
        return false;
    }

    req.userRole = req.session->userRole;
    return true;
}

inline bool afterGetUserInfoValidate(
    Request& req, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    BaseRule& rule, const dbus::utility::DBusPropertiesMap& userInfoMap)
{
    if (req.session == nullptr || !populateUserInfo(*req.session, userInfoMap))
    {
        BMCWEB_LOG_ERROR("Failed to populate user information");
        asyncResp->res.result(
            boost::beast::http::status::internal_server_error);
        return false;
    }

    if (!isUserPrivileged(req, asyncResp, rule))
    {
        // User is not privileged
        BMCWEB_LOG_ERROR("Insufficient Privilege");
        asyncResp->res.result(boost::beast::http::status::forbidden);
        return false;
    }

    return true;
}

inline void requestUserInfo(
    const std::string& username,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::move_only_function<void(const dbus::utility::DBusPropertiesMap&)>&&
        callback)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, callback = std::move(callback)](
            const boost::system::error_code& ec,
            const dbus::utility::DBusPropertiesMap& userInfoMap) mutable {
            if (ec)
            {
                BMCWEB_LOG_ERROR("GetUserInfo failed...");
                asyncResp->res.result(
                    boost::beast::http::status::internal_server_error);
                return;
            }
            callback(userInfoMap);
        },
        "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
        "xyz.openbmc_project.User.Manager", "GetUserInfo", username);
}

inline void validatePrivilege(
    const std::shared_ptr<Request>& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, BaseRule& rule,
    std::move_only_function<void()>&& callback)
{
    if (req->session == nullptr)
    {
        return;
    }

    requestUserInfo(
        req->session->username, asyncResp,
        [req, asyncResp, &rule, callback = std::move(callback)](
            const dbus::utility::DBusPropertiesMap& userInfoMap) mutable {
            if (afterGetUserInfoValidate(*req, asyncResp, rule, userInfoMap))
            {
                callback();
            }
        });
}

inline void getUserInfo(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& username,
                        std::shared_ptr<persistent_data::UserSession>& session,
                        std::move_only_function<void()>&& callback)
{
    requestUserInfo(
        username, asyncResp,
        [asyncResp, session, callback = std::move(callback)](
            const dbus::utility::DBusPropertiesMap& userInfoMap) mutable {
            if (!populateUserInfo(*session, userInfoMap))
            {
                BMCWEB_LOG_ERROR("Failed to populate user information");
                asyncResp->res.result(
                    boost::beast::http::status::internal_server_error);
                return;
            }
            callback();
        });
}

} // namespace crow
