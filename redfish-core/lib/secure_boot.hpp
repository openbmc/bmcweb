// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"

#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>

namespace redfish
{
static std::string getSecureBootMode(const std::string& mode)
{
    if (mode == "xyz.openbmc_project.BIOSConfig.SecureBoot.ModeType.Setup")
    {
        return "SetupMode";
    }
    else if (mode == "xyz.openbmc_project.BIOSConfig.SecureBoot.ModeType.User")
    {
        return "UserMode";
    }
    else if (mode == "xyz.openbmc_project.BIOSConfig.SecureBoot.ModeType.Audit")
    {
        return "AuditMode";
    }
    else if (mode ==
             "xyz.openbmc_project.BIOSConfig.SecureBoot.ModeType.Deployed")
    {
        return "DeployedMode";
    }
    return "Unknown";
}

inline void afterGetSecureBootData(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error on SecureBoot GetAll: {}",
                         ec.message());
        messages::internalError(aResp->res);
        return;
    }

    const std::string* secureBootCurrentBoot = nullptr;
    const bool* secureBootPendingEnable = nullptr;
    const std::string* secureBootMode = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "CurrentBoot",
        secureBootCurrentBoot, "PendingEnable", secureBootPendingEnable, "Mode",
        secureBootMode);

    if (!success)
    {
        messages::internalError(aResp->res);
        return;
    }

    std::string mode;
    if (secureBootMode != nullptr)
    {
        mode = getSecureBootMode(*secureBootMode);
    }

    if (secureBootCurrentBoot != nullptr &&
        *secureBootCurrentBoot ==
            "xyz.openbmc_project.BIOSConfig.SecureBoot.CurrentBootType.Unknown" &&
        mode == "Unknown")
    {
        // BMC has not yet recevied data.
        return;
    }

    if (secureBootCurrentBoot != nullptr)
    {
        if (*secureBootCurrentBoot ==
            "xyz.openbmc_project.BIOSConfig.SecureBoot.CurrentBootType.Enabled")
        {
            aResp->res.jsonValue["SecureBootCurrentBoot"] = "Enabled";
        }
        else
        {
            aResp->res.jsonValue["SecureBootCurrentBoot"] = "Disabled";
        }
    }
    if (secureBootPendingEnable != nullptr)
    {
        aResp->res.jsonValue["SecureBootEnable"] = *secureBootPendingEnable;
    }
    aResp->res.jsonValue["SecureBootMode"] = mode;
}

inline void handleSecureBootGet(crow::App& app, const crow::Request& req,
                                const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                                [[maybe_unused]] const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, aResp))
    {
        return;
    }
    aResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/SecureBoot", BMCWEB_REDFISH_SYSTEM_URI_NAME);
    aResp->res.jsonValue["@odata.type"] = "#SecureBoot.v1_1_0.SecureBoot";
    aResp->res.jsonValue["Name"] = "UEFI Secure Boot";
    aResp->res.jsonValue["Description"] =
        "The UEFI Secure Boot associated with this system.";
    aResp->res.jsonValue["Id"] = "SecureBoot";

    dbus::utility::getAllProperties(
        "xyz.openbmc_project.BIOSConfigManager",
        "/xyz/openbmc_project/bios_config/secure_boot",
        "xyz.openbmc_project.BIOSConfig.SecureBoot",
        [aResp](const boost::system::error_code& ec,
                const dbus::utility::DBusPropertiesMap& properties) {
            afterGetSecureBootData(aResp, ec, properties);
        });
}

inline void handleSecureBootHead(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    [[maybe_unused]] const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, aResp))
    {
        return;
    }
    aResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/SecureBoot/SecureBoot.json>; rel=describedby");
}

inline void requestRoutesSecureBoot(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/SecureBoot/")
        .privileges(redfish::privileges::headSecureBoot)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleSecureBootHead, std::ref(app)));
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/SecureBoot/")
        .privileges(redfish::privileges::getSecureBoot)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleSecureBootGet, std::ref(app)));
}
} // namespace redfish
