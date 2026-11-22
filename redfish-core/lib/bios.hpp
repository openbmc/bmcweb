// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/attribute_registry.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/bios_utils.hpp"
#include "utils/sw_utils.hpp"

#include <sys/types.h>

#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>

#include <array>
#include <cerrno>
#include <cstdint>
#include <format>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace redfish
{
using BaseTableOption =
    std::tuple<std::string, dbus::utility::DbusVariantType, std::string>;

using BaseTableAttribute =
    std::tuple<std::string, bool, std::string, std::string, std::string,
               dbus::utility::DbusVariantType, dbus::utility::DbusVariantType,
               std::vector<BaseTableOption>>;

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
    for (const auto& [name, baseTableAttribute] : baseTable)
    {
        bios_utils::addAttribute(
            attributes, name,
            std::get<uint(BaseTableAttributeIndex::Type)>(baseTableAttribute),
            std::get<uint(BaseTableAttributeIndex::CurrentValue)>(
                baseTableAttribute));
    }
}

inline void handleBiosManagerObjectForGetBiosAttributes(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& objectPath)
{
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
    asyncResp->res.jsonValue["@odata.id"] = std::format(
        "/redfish/v1/Systems/{}/Bios", BMCWEB_REDFISH_SYSTEM_URI_NAME);
    asyncResp->res.jsonValue["@odata.type"] = "#Bios.v1_2_0.Bios";
    asyncResp->res.jsonValue["Name"] = "BIOS Configuration";
    asyncResp->res.jsonValue["Description"] = "BIOS Configuration Service";
    asyncResp->res.jsonValue["Id"] = "BIOS";
    asyncResp->res.jsonValue["AttributeRegistry"] =
        bios_utils::biosAttributeRegistryName;
    asyncResp->res.jsonValue["Actions"]["#Bios.ResetBios"]["target"] =
        boost::urls::format(
            "/redfish/v1/Systems/{}/Bios/Actions/Bios.ResetBios",
            BMCWEB_REDFISH_SYSTEM_URI_NAME);
    dbus::utility::checkDbusPathExists(
        "/xyz/openbmc_project/bios_config/manager", [asyncResp](int rc) {
            if (rc > 0)
            {
                getBiosAttributes(asyncResp);
            }
        });
    // Get the ActiveSoftwareImage and SoftwareImages
    sw_util::populateSoftwareInformation(asyncResp, sw_util::biosPurpose, "",
                                         true);
}

inline void requestRoutesBiosService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/")
        .privileges(redfish::privileges::getBios)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleBiosServiceGet, std::ref(app)));
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
        "xyz.openbmc_project.Software.Host.Updater0",
        "/xyz/openbmc_project/software",
        "xyz.openbmc_project.Common.FactoryReset", "Reset");
}

inline void requestRoutesBiosReset(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/Actions/Bios.ResetBios/")
        .privileges(redfish::privileges::postBios)
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleBiosResetPost, std::ref(app)));
}

inline attribute_registry::AttributeType getBiosAttributeType(
    std::string_view dbusType)
{
    if (dbusType ==
        "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Enumeration")
    {
        return attribute_registry::AttributeType::Enumeration;
    }
    if (dbusType ==
        "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.String")
    {
        return attribute_registry::AttributeType::String;
    }
    if (dbusType ==
        "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Integer")
    {
        return attribute_registry::AttributeType::Integer;
    }
    if (dbusType ==
        "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Boolean")
    {
        return attribute_registry::AttributeType::Boolean;
    }
    if (dbusType ==
        "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Password")
    {
        return attribute_registry::AttributeType::Password;
    }
    return attribute_registry::AttributeType::Invalid;
}

inline void fillBiosAttributeBounds(nlohmann::json& attribute,
                                    attribute_registry::AttributeType type,
                                    const std::vector<BaseTableOption>& options)
{
    nlohmann::json::array_t values;
    for (const BaseTableOption& option : options)
    {
        const std::string& boundType = std::get<0>(option);
        const dbus::utility::DbusVariantType& boundValue = std::get<1>(option);
        const std::string& valueName = std::get<2>(option);

        if (boundType ==
            "xyz.openbmc_project.BIOSConfig.Manager.BoundType.OneOf")
        {
            const std::string* strValue = std::get_if<std::string>(&boundValue);
            if (strValue == nullptr)
            {
                continue;
            }
            nlohmann::json::object_t oneOf;
            oneOf["ValueName"] = *strValue;
            if (!valueName.empty())
            {
                oneOf["ValueDisplayName"] = valueName;
            }
            values.emplace_back(std::move(oneOf));
            continue;
        }

        const int64_t* intValue = std::get_if<int64_t>(&boundValue);
        if (intValue == nullptr)
        {
            continue;
        }
        if (boundType ==
            "xyz.openbmc_project.BIOSConfig.Manager.BoundType.LowerBound")
        {
            attribute["LowerBound"] = *intValue;
        }
        else if (boundType ==
                 "xyz.openbmc_project.BIOSConfig.Manager.BoundType.UpperBound")
        {
            attribute["UpperBound"] = *intValue;
        }
        else if (
            boundType ==
            "xyz.openbmc_project.BIOSConfig.Manager.BoundType.ScalarIncrement")
        {
            attribute["ScalarIncrement"] = *intValue;
        }
        else if (
            boundType ==
            "xyz.openbmc_project.BIOSConfig.Manager.BoundType.MinStringLength")
        {
            attribute["MinLength"] = *intValue;
        }
        else if (
            boundType ==
            "xyz.openbmc_project.BIOSConfig.Manager.BoundType.MaxStringLength")
        {
            attribute["MaxLength"] = *intValue;
        }
    }
    if (type == attribute_registry::AttributeType::Enumeration)
    {
        attribute["Value"] = std::move(values);
    }
}

inline nlohmann::json::array_t buildBiosAttributeRegistryEntries(
    const BaseTable& baseTable)
{
    nlohmann::json::array_t attributes;
    for (const auto& [name, entry] : baseTable)
    {
        const std::string& dbusType =
            std::get<uint(BaseTableAttributeIndex::Type)>(entry);
        attribute_registry::AttributeType type = getBiosAttributeType(dbusType);
        if (type == attribute_registry::AttributeType::Invalid)
        {
            BMCWEB_LOG_WARNING("Skipping BIOS attribute {} of unknown type",
                               name);
            continue;
        }

        nlohmann::json attribute = nlohmann::json::object();
        attribute["AttributeName"] = name;
        attribute["Type"] = type;
        attribute["ReadOnly"] =
            std::get<uint(BaseTableAttributeIndex::ReadOnly)>(entry);
        attribute["DisplayName"] =
            std::get<uint(BaseTableAttributeIndex::Name)>(entry);
        const std::string& helpText =
            std::get<uint(BaseTableAttributeIndex::Description)>(entry);
        if (!helpText.empty())
        {
            attribute["HelpText"] = helpText;
        }
        const std::string& menuPath =
            std::get<uint(BaseTableAttributeIndex::Path)>(entry);
        if (!menuPath.empty())
        {
            attribute["MenuPath"] = menuPath;
        }
        if (type != attribute_registry::AttributeType::Password)
        {
            // Same conversion as the Attributes of the Bios resource
            bios_utils::addAttribute(
                attribute, "CurrentValue", dbusType,
                std::get<uint(BaseTableAttributeIndex::CurrentValue)>(entry));
            bios_utils::addAttribute(
                attribute, "DefaultValue", dbusType,
                std::get<uint(BaseTableAttributeIndex::DefaultValue)>(entry));
        }
        fillBiosAttributeBounds(
            attribute, type,
            std::get<uint(BaseTableAttributeIndex::Options)>(entry));
        attributes.emplace_back(std::move(attribute));
    }
    return attributes;
}

inline void afterFindBiosManagerForRegistry(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::string_view resourceType, std::string_view resourceId,
    const std::function<void(const std::string&)>& callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetObject& object)
{
    if (ec.value() == EBADR || ec.value() == boost::system::errc::io_error ||
        (!ec && object.empty()))
    {
        messages::resourceNotFound(asyncResp->res, resourceType, resourceId);
        return;
    }
    if (ec || object.size() > 1)
    {
        BMCWEB_LOG_ERROR("Error finding BIOS Manager object {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }
    callback(object.begin()->first);
}

/**
 * @brief Find the BIOS configuration manager for the attribute registry.
 *
 * Not every system has a BIOS configuration manager, so a missing object
 * is reported as ResourceNotFound rather than as an internal error.
 */
inline void getBiosManagerForRegistry(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::string_view resourceType, std::string_view resourceId,
    std::function<void(const std::string&)> callback)
{
    dbus::utility::getDbusObject(
        std::string(bios_utils::biosConfigManagerPath),
        std::array<std::string_view, 1>{bios_utils::biosConfigManagerInterface},
        std::bind_front(afterFindBiosManagerForRegistry, asyncResp,
                        resourceType, resourceId, std::move(callback)));
}

inline void afterGetBaseBiosTableForRegistry(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const BaseTable& baseTable)
{
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Registries/{}/{}", bios_utils::biosAttributeRegistryId,
        bios_utils::biosAttributeRegistryId);
    asyncResp->res.jsonValue["@odata.type"] =
        "#AttributeRegistry.v1_5_0.AttributeRegistry";
    asyncResp->res.jsonValue["Id"] = bios_utils::biosAttributeRegistryName;
    asyncResp->res.jsonValue["Name"] = "BIOS Attribute Registry";
    asyncResp->res.jsonValue["Description"] =
        "Attribute registry for the BIOS configuration of this system";
    asyncResp->res.jsonValue["Language"] = "en";
    asyncResp->res.jsonValue["OwningEntity"] = "OpenBMC";
    asyncResp->res.jsonValue["RegistryVersion"] =
        bios_utils::biosAttributeRegistryVersion;
    asyncResp->res.jsonValue["RegistryEntries"]["Attributes"] =
        buildBiosAttributeRegistryEntries(baseTable);
}

inline void afterGetBiosManagerForRegistry(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& objectPath)
{
    bios_utils::getBIOSManagerProperty<BaseTable>(
        asyncResp, "BaseBIOSTable", objectPath,
        std::bind_front(afterGetBaseBiosTableForRegistry, asyncResp));
}

inline void handleBiosAttributeRegistryGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    getBiosManagerForRegistry(
        asyncResp, "AttributeRegistry", bios_utils::biosAttributeRegistryId,
        std::bind_front(afterGetBiosManagerForRegistry, asyncResp));
}

inline void fillBiosAttributeRegistryFile(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& /*objectPath*/)
{
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Registries/{}", bios_utils::biosAttributeRegistryId);
    asyncResp->res.jsonValue["@odata.type"] =
        "#MessageRegistryFile.v1_1_0.MessageRegistryFile";
    asyncResp->res.jsonValue["Id"] = bios_utils::biosAttributeRegistryId;
    asyncResp->res.jsonValue["Name"] = "BIOS Attribute Registry File";
    asyncResp->res.jsonValue["Description"] =
        "BIOS Attribute Registry File Location";
    asyncResp->res.jsonValue["Registry"] = "BiosAttributeRegistry.1.0";
    nlohmann::json::array_t languages;
    languages.emplace_back("en");
    asyncResp->res.jsonValue["Languages@odata.count"] = languages.size();
    asyncResp->res.jsonValue["Languages"] = std::move(languages);
    nlohmann::json::array_t locationMembers;
    nlohmann::json::object_t location;
    location["Language"] = "en";
    location["Uri"] = boost::urls::format(
        "/redfish/v1/Registries/{}/{}", bios_utils::biosAttributeRegistryId,
        bios_utils::biosAttributeRegistryId);
    locationMembers.emplace_back(std::move(location));
    asyncResp->res.jsonValue["Location@odata.count"] = locationMembers.size();
    asyncResp->res.jsonValue["Location"] = std::move(locationMembers);
}

inline void handleBiosAttributeRegistryFileGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    getBiosManagerForRegistry(
        asyncResp, "MessageRegistryFile", bios_utils::biosAttributeRegistryId,
        std::bind_front(fillBiosAttributeRegistryFile, asyncResp));
}

} // namespace redfish
