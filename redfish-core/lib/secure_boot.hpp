// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/secure_boot.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace redfish
{
constexpr const char* secureBootInterface =
    "xyz.openbmc_project.BIOSConfig.SecureBoot";

inline secure_boot::SecureBootModeType translateModeToRedfish(
    std::string_view dbusMode)
{
    if (dbusMode == "xyz.openbmc_project.BIOSConfig.SecureBoot.ModeType.Setup")
    {
        return secure_boot::SecureBootModeType::SetupMode;
    }
    if (dbusMode == "xyz.openbmc_project.BIOSConfig.SecureBoot.ModeType.User")
    {
        return secure_boot::SecureBootModeType::UserMode;
    }
    if (dbusMode == "xyz.openbmc_project.BIOSConfig.SecureBoot.ModeType.Audit")
    {
        return secure_boot::SecureBootModeType::AuditMode;
    }
    if (dbusMode ==
        "xyz.openbmc_project.BIOSConfig.SecureBoot.ModeType.Deployed")
    {
        return secure_boot::SecureBootModeType::DeployedMode;
    }
    return secure_boot::SecureBootModeType::Invalid;
}

inline secure_boot::SecureBootCurrentBootType translateCurrentBootToRedfish(
    std::string_view dbusCurrentBoot)
{
    if (dbusCurrentBoot ==
        "xyz.openbmc_project.BIOSConfig.SecureBoot.CurrentBootType.Enabled")
    {
        return secure_boot::SecureBootCurrentBootType::Enabled;
    }
    if (dbusCurrentBoot ==
        "xyz.openbmc_project.BIOSConfig.SecureBoot.CurrentBootType.Disabled")
    {
        return secure_boot::SecureBootCurrentBootType::Disabled;
    }
    return secure_boot::SecureBootCurrentBootType::Invalid;
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

    if (secureBootCurrentBoot != nullptr)
    {
        secure_boot::SecureBootCurrentBootType currentBoot =
            translateCurrentBootToRedfish(*secureBootCurrentBoot);
        if (currentBoot != secure_boot::SecureBootCurrentBootType::Invalid)
        {
            aResp->res.jsonValue["SecureBootCurrentBoot"] = currentBoot;
        }
    }
    if (secureBootMode != nullptr)
    {
        secure_boot::SecureBootModeType mode =
            translateModeToRedfish(*secureBootMode);
        if (mode != secure_boot::SecureBootModeType::Invalid)
        {
            aResp->res.jsonValue["SecureBootMode"] = mode;
        }
    }
    if (secureBootPendingEnable != nullptr)
    {
        aResp->res.jsonValue["SecureBootEnable"] = *secureBootPendingEnable;
    }
}

inline void afterGetSecureBootSubTree(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec || subtree.empty())
    {
        BMCWEB_LOG_DEBUG("SecureBoot backend not found: {}", ec.message());
        messages::resourceNotFound(aResp->res, "SecureBoot", "SecureBoot");
        return;
    }
    if (subtree.size() > 1 || subtree[0].second.empty())
    {
        BMCWEB_LOG_ERROR("Expected one SecureBoot object, found {}",
                         subtree.size());
        messages::internalError(aResp->res);
        return;
    }
    const std::string& objPath = subtree[0].first;
    const std::string& service = subtree[0].second.begin()->first;
    dbus::utility::getAllProperties(
        service, objPath, secureBootInterface,
        std::bind_front(afterGetSecureBootData, aResp));
}

inline void handleSecureBootGet(crow::App& app, const crow::Request& req,
                                const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                                const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, aResp))
    {
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(aResp->res, "ComputerSystem", systemName);
        return;
    }
    aResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/SecureBoot", BMCWEB_REDFISH_SYSTEM_URI_NAME);
    aResp->res.jsonValue["@odata.type"] = "#SecureBoot.v1_1_0.SecureBoot";
    aResp->res.jsonValue["Name"] = "UEFI Secure Boot";
    aResp->res.jsonValue["Description"] =
        "The UEFI Secure Boot associated with this system.";
    aResp->res.jsonValue["Id"] = "SecureBoot";

    constexpr std::array<std::string_view, 1> interfaces = {
        secureBootInterface};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project", 0, interfaces,
        std::bind_front(afterGetSecureBootSubTree, aResp));
}

inline void handleSecureBootHead(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, aResp))
    {
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(aResp->res, "ComputerSystem", systemName);
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
