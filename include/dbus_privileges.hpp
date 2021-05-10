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

inline bool
    isUserPrivileged(Request& req,
                     const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     BaseRule& rule)
{
    // Get the user's privileges from the role
    redfish::Privileges userPrivileges =
        redfish::getUserPrivileges(*req.session);

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
        BMCWEB_LOG_WARNING << "Insufficient Privilege";
        asyncResp->res.result(boost::beast::http::status::forbidden);
        return;
    }
    callback(req);
}

} // namespace crow
