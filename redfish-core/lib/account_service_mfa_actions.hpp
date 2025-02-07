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
#include "privileges.hpp"
#include "query.hpp"

#include <boost/beast/http/verb.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <functional>
#include <memory>
#include <string>

namespace redfish
{

inline void createSecretKeyUtil(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& userPath)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code& ec,
                    const std::string& secretKey,
                    sdbusplus::message_t& m) {
            if (ec)
            {
                const sd_bus_error* dbusError = m.get_error();
                if (dbusError == nullptr)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                BMCWEB_LOG_ERROR("DBus error: {}", dbusError->name);

                if (std::string_view(
                        "xyz.openbmc_project.Common.Error.ResourceNotFound") ==
                    dbusError->name)
                {
                    messages::resourceNotFound(asyncResp->res, "", "CreateSecretKey");
                    return;
                }
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

    createSecretKeyUtil(asyncResp, userPath);
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
}
} // namespace redfish
