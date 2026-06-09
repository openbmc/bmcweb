// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/bios_utils.hpp"
#include "utils/json_utils.hpp"
#include "utils/sw_utils.hpp"

#include <sys/types.h>

#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace redfish
{
using BaseTableOption =
    std::tuple<std::string, bios_utils::BiosAttributeValue, std::string>;

using BaseTableAttribute =
    std::tuple<bios_utils::BiosAttributeType, bool, std::string, std::string,
               std::string, bios_utils::BiosAttributeValue,
               bios_utils::BiosAttributeValue, std::vector<BaseTableOption>>;

enum class BaseTableAttributeIndex
{
    Type = 0,
    ReadOnly,
    Name,
    Description,
    Path,
    CurrentValue,
    DefaultValue,
    Options
};

using BaseTable = std::map<std::string, BaseTableAttribute>;

inline void populateRedfishFromBaseTable(crow::Response& response,
                                         const BaseTable& baseTable)
{
    nlohmann::json& attributes = response.jsonValue["Attributes"];
    if (!attributes.is_object())
    {
        attributes = nlohmann::json::object();
    }
    for (const auto& [name, baseTableAttribute] : baseTable)
    {
        bios_utils::addAttribute(
            attributes, name,
            std::get<uint(BaseTableAttributeIndex::Type)>(baseTableAttribute),
            std::get<uint(BaseTableAttributeIndex::CurrentValue)>(
                baseTableAttribute));
    }
}

inline void populateSettings(crow::Response& response)
{
    nlohmann::json& redfishSettings = response.jsonValue["@Redfish.Settings"];
    redfishSettings["@odata.type"] = "#Settings.v1_3_5.Settings";
    redfishSettings["SupportedApplyTimes"] = nlohmann::json::array_t{"OnReset"};
    redfishSettings["SettingsObject"]["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/Bios/Settings", BMCWEB_REDFISH_SYSTEM_URI_NAME);
}

inline void populateRedfishFromPending(
    crow::Response& response,
    const bios_utils::PendingAttributes& pendingAttributes)
{
    nlohmann::json& attributes = response.jsonValue["Attributes"];
    if (!attributes.is_object())
    {
        attributes = nlohmann::json::object();
    }
    for (const auto& [name, pendingAttribute] : pendingAttributes)
    {
        bios_utils::addAttribute(
            attributes, name,
            std::get<uint(bios_utils::PendingAttributeValueIndex::Type)>(
                pendingAttribute),
            std::get<uint(bios_utils::PendingAttributeValueIndex::Value)>(
                pendingAttribute));
    }
}

inline void updatePendingAttribute(
    bios_utils::PendingAttributes& pendingAttributes, const std::string& name,
    bios_utils::PendingAttributeValue attributeValue)
{
    pendingAttributes[name] = std::move(attributeValue);
}

inline bool populatePendingFromRedfish(
    bios_utils::PendingAttributes& pendingAttributes,
    const nlohmann::json::object_t& jsonAttributes, crow::Response& response)
{
    for (const auto& [name, value] : jsonAttributes)
    {
        const std::string* strValue = value.get_ptr<const std::string*>();
        if (strValue != nullptr)
        {
            updatePendingAttribute(
                pendingAttributes, name,
                std::make_tuple(
                    std::string(bios_utils::biosAttributeTypeString),
                    *strValue));
            continue;
        }
        const bool* boolValue = value.get_ptr<const bool*>();
        if (boolValue != nullptr)
        {
            updatePendingAttribute(
                pendingAttributes, name,
                std::make_tuple(
                    std::string(bios_utils::biosAttributeTypeBoolean),
                    *boolValue));
            continue;
        }
        const int64_t* intValue = value.get_ptr<const int64_t*>();
        if (intValue != nullptr)
        {
            updatePendingAttribute(
                pendingAttributes, name,
                std::make_tuple(
                    std::string(bios_utils::biosAttributeTypeInteger),
                    *intValue));
            continue;
        }

        BMCWEB_LOG_ERROR("Invalid type for attribute {} in request", name);
        messages::propertyValueTypeError(response, value, name);
        return false;
    }
    return true;
}

inline void handleBiosManagerObjectForGetBiosAttributes(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& objectPath)
{
    populateSettings(asyncResp->res);
    bios_utils::getBIOSManagerProperty<BaseTable>(
        asyncResp, "BaseBIOSTable", objectPath,
        std::bind_front(populateRedfishFromBaseTable,
                        std::ref(asyncResp->res)));
}

inline void getBiosAttributes(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    bios_utils::getBIOSManagerObject(
        asyncResp, std::bind_front(handleBiosManagerObjectForGetBiosAttributes,
                                   asyncResp));
}

/**
 * BiosService class supports handle get method for bios.
 */
inline void handleBiosServiceGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/Bios", BMCWEB_REDFISH_SYSTEM_URI_NAME);
    asyncResp->res.jsonValue["@odata.type"] = "#Bios.v1_1_0.Bios";
    asyncResp->res.jsonValue["Name"] = "BIOS Configuration";
    asyncResp->res.jsonValue["Description"] = "BIOS Configuration Service";
    asyncResp->res.jsonValue["Id"] = "BIOS";
    asyncResp->res.jsonValue["Actions"]["#Bios.ResetBios"]["target"] =
        boost::urls::format(
            "/redfish/v1/Systems/{}/Bios/Actions/Bios.ResetBios",
            BMCWEB_REDFISH_SYSTEM_URI_NAME);
    dbus::utility::checkDbusPathExists(
        std::string(bios_utils::biosConfigManagerPath), [asyncResp](int rc) {
            if (rc > 0)
            {
                getBiosAttributes(asyncResp);
            }
        });
    // Get the ActiveSoftwareImage and SoftwareImages
    sw_util::populateSoftwareInformation(asyncResp, sw_util::biosPurpose, "",
                                         true);
}

inline void handlePendingBiosManagerObjectForGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& objectPath)
{
    bios_utils::getBIOSManagerProperty<bios_utils::PendingAttributes>(
        asyncResp, "PendingAttributes", objectPath,
        std::bind_front(populateRedfishFromPending, std::ref(asyncResp->res)));
}

inline void handlePendingBiosPathForGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, int rc)
{
    if (rc > 0)
    {
        bios_utils::getBIOSManagerObject(
            asyncResp,
            std::bind_front(handlePendingBiosManagerObjectForGet, asyncResp));
        return;
    }
    messages::resourceNotFound(asyncResp->res, "Bios", "Settings");
}

inline void handlePendingBiosGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/Bios/Settings", BMCWEB_REDFISH_SYSTEM_URI_NAME);
    asyncResp->res.jsonValue["@odata.type"] = "#Bios.v1_1_0.Bios";
    asyncResp->res.jsonValue["Name"] = "Pending BIOS Configuration";
    asyncResp->res.jsonValue["Description"] =
        "Pending BIOS Configuration Service";
    asyncResp->res.jsonValue["Id"] = "Pending";

    dbus::utility::checkDbusPathExists(
        std::string(bios_utils::biosConfigManagerPath),
        std::bind_front(handlePendingBiosPathForGet, asyncResp));
}

inline void handlePendingBiosPatchAttributes(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::object_t& jsonAttributes,
    const bios_utils::PendingAttributes& currentPendingAttributes)
{
    bios_utils::PendingAttributes pendingAttributes = currentPendingAttributes;
    if (!populatePendingFromRedfish(pendingAttributes, jsonAttributes,
                                    asyncResp->res))
    {
        return;
    }
    bios_utils::getBIOSManagerObject(
        asyncResp,
        std::bind_front(bios_utils::setBIOSManagerProperty, asyncResp,
                        "PendingAttributes", pendingAttributes));
}

inline void handlePendingBiosManagerObjectForPatch(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::object_t& jsonAttributes,
    const std::string& objectPath)
{
    bios_utils::getBIOSManagerProperty<bios_utils::PendingAttributes>(
        asyncResp, "PendingAttributes", objectPath,
        std::bind_front(handlePendingBiosPatchAttributes, asyncResp,
                        jsonAttributes));
}

inline void handlePendingBiosPathForPatch(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::object_t& jsonAttributes, int rc)
{
    if (rc > 0)
    {
        bios_utils::getBIOSManagerObject(
            asyncResp, std::bind_front(handlePendingBiosManagerObjectForPatch,
                                       asyncResp, jsonAttributes));
        return;
    }
    messages::resourceNotFound(asyncResp->res, "Bios", "Settings");
}

inline void handlePendingBiosPatch(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    nlohmann::json::object_t jsonAttributes;
    if (!json_util::readJsonPatch(req, asyncResp->res, "Attributes",
                                  jsonAttributes))
    {
        BMCWEB_LOG_ERROR("Invalid JSON request.");
        return;
    }

    dbus::utility::checkDbusPathExists(
        std::string(bios_utils::biosConfigManagerPath),
        std::bind_front(handlePendingBiosPathForPatch, asyncResp,
                        std::move(jsonAttributes)));
}

inline void requestRoutesBiosService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/")
        .privileges(redfish::privileges::getBios)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleBiosServiceGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/Settings/")
        .privileges(redfish::privileges::getBios)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePendingBiosGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/Settings/")
        .privileges(redfish::privileges::patchBios)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handlePendingBiosPatch, std::ref(app)));
}

/**
 * BiosReset class supports handle POST method for Reset bios.
 * The class retrieves and sends data directly to D-Bus.
 *
 * Function handles POST method request.
 * Analyzes POST body message before sends Reset request data to D-Bus.
 */
inline void handleBiosResetPost(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    dbus::utility::async_method_call(
        asyncResp,
        [asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("Failed to reset bios: {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }
        },
        "org.open_power.Software.Host.Updater", "/xyz/openbmc_project/software",
        "xyz.openbmc_project.Common.FactoryReset", "Reset");
}

inline void requestRoutesBiosReset(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/Actions/Bios.ResetBios/")
        .privileges(redfish::privileges::postBios)
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleBiosResetPost, std::ref(app)));
}

} // namespace redfish
