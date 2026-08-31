// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/json_utils.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace redfish
{
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

    std::string mode = "Unknown";
    static const std::unordered_map<std::string, std::string> modeMap = {
        {"xyz.openbmc_project.BIOSConfig.SecureBoot.ModeType.Setup",
         "SetupMode"},
        {"xyz.openbmc_project.BIOSConfig.SecureBoot.ModeType.User", "UserMode"},
        {"xyz.openbmc_project.BIOSConfig.SecureBoot.ModeType.Audit",
         "AuditMode"},
        {"xyz.openbmc_project.BIOSConfig.SecureBoot.ModeType.Deployed",
         "DeployedMode"}};
    if (secureBootMode != nullptr)
    {
        auto it = modeMap.find(*secureBootMode);
        if (it != modeMap.end())
        {
            mode = it->second;
        }
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

/**
 * Handle PATCH of the SecureBoot resource.
 *
 * SecureBootEnable is the administrator's request to change the setting;
 * it is staged in the BIOSConfig.SecureBoot PendingEnable property for
 * the host firmware to apply at next boot. SecureBootCurrentBoot and
 * SecureBootMode are read-only facts about the current boot and are
 * rejected here.
 */
inline void handleSecureBootPatch(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    [[maybe_unused]] const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, aResp))
    {
        return;
    }

    std::optional<std::string> secureBootCurrentBoot;
    std::optional<std::string> secureBootMode;
    std::optional<bool> secureBootEnable;
    if (!json_util::readJsonPatch(                          //
            req, aResp->res,                                //
            "SecureBootCurrentBoot", secureBootCurrentBoot, //
            "SecureBootEnable", secureBootEnable,           //
            "SecureBootMode", secureBootMode))
    {
        return;
    }

    constexpr const char* sbService = "xyz.openbmc_project.BIOSConfigManager";
    constexpr const char* sbPath =
        "/xyz/openbmc_project/bios_config/secure_boot";
    constexpr const char* sbIface = "xyz.openbmc_project.BIOSConfig.SecureBoot";

    // SecureBootCurrentBoot and SecureBootMode are read-only per the
    // Redfish SecureBoot schema: they describe the state of the current
    // boot, which only the host firmware can know. Host-firmware state
    // reporting over the Redfish Host Interface will enable writing them
    // for the host identity once bootstrap account support is available.
    if (secureBootCurrentBoot)
    {
        messages::propertyNotWritable(aResp->res, "SecureBootCurrentBoot");
        return;
    }
    if (secureBootMode)
    {
        messages::propertyNotWritable(aResp->res, "SecureBootMode");
        return;
    }

    if (secureBootEnable)
    {
        sdbusplus::asio::setProperty(
            *crow::connections::systemBus, sbService, sbPath, sbIface,
            "PendingEnable", *secureBootEnable,
            [aResp](const boost::system::error_code& ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR("Failed to set PendingEnable: {}", ec);
                    messages::internalError(aResp->res);
                }
            });
    }
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
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/SecureBoot/")
        .privileges(redfish::privileges::patchSecureBoot)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handleSecureBootPatch, std::ref(app)));
}
} // namespace redfish
