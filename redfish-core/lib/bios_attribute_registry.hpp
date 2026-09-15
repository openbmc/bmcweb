// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "bios.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "logging.hpp"
#include "utils/bios_utils.hpp"

#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>

namespace redfish
{

inline std::string_view getBiosAttributeType(std::string_view dbusType)
{
    constexpr std::string_view prefix =
        "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.";
    if (!dbusType.starts_with(prefix))
    {
        return "";
    }
    std::string_view type = dbusType.substr(prefix.size());
    if (type == "Enumeration" || type == "String" || type == "Integer" ||
        type == "Boolean" || type == "Password")
    {
        return type;
    }
    return "";
}

inline std::string_view getBiosBoundType(std::string_view dbusType)
{
    constexpr std::string_view prefix =
        "xyz.openbmc_project.BIOSConfig.Manager.BoundType.";
    if (!dbusType.starts_with(prefix))
    {
        return "";
    }
    return dbusType.substr(prefix.size());
}

inline void fillBiosAttributeValue(
    nlohmann::json::object_t& attribute, const std::string& key,
    std::string_view type, const dbus::utility::DbusVariantType& value)
{
    if (type == "Integer")
    {
        const int64_t* intValue = std::get_if<int64_t>(&value);
        if (intValue != nullptr)
        {
            attribute[key] = *intValue;
        }
        return;
    }
    if (type == "Boolean")
    {
        const int64_t* intValue = std::get_if<int64_t>(&value);
        if (intValue != nullptr)
        {
            attribute[key] = (*intValue != 0);
            return;
        }
        const bool* boolValue = std::get_if<bool>(&value);
        if (boolValue != nullptr)
        {
            attribute[key] = *boolValue;
        }
        return;
    }
    const std::string* strValue = std::get_if<std::string>(&value);
    if (strValue != nullptr)
    {
        attribute[key] = *strValue;
    }
}

inline void fillBiosAttributeBounds(nlohmann::json::object_t& attribute,
                                    std::string_view type,
                                    const std::vector<BaseTableOption>& options)
{
    nlohmann::json::array_t values;
    for (const BaseTableOption& option : options)
    {
        std::string_view boundType = getBiosBoundType(std::get<0>(option));
        const dbus::utility::DbusVariantType& boundValue = std::get<1>(option);
        const std::string& valueName = std::get<2>(option);

        if (boundType == "OneOf")
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
        if (boundType == "LowerBound")
        {
            attribute["LowerBound"] = *intValue;
        }
        else if (boundType == "UpperBound")
        {
            attribute["UpperBound"] = *intValue;
        }
        else if (boundType == "ScalarIncrement")
        {
            attribute["ScalarIncrement"] = *intValue;
        }
        else if (boundType == "MinStringLength")
        {
            attribute["MinLength"] = *intValue;
        }
        else if (boundType == "MaxStringLength")
        {
            attribute["MaxLength"] = *intValue;
        }
    }
    if (type == "Enumeration")
    {
        attribute["Value"] = std::move(values);
    }
}

inline void fillBiosAttributeRegistryEntries(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const BaseTable& baseTable)
{
    nlohmann::json::array_t attributes;
    for (const auto& [name, entry] : baseTable)
    {
        std::string_view type = getBiosAttributeType(
            std::get<uint(BaseTableAttributeIndex::Type)>(entry));
        if (type.empty())
        {
            BMCWEB_LOG_WARNING("Skipping BIOS attribute {} of unknown type",
                               name);
            continue;
        }

        nlohmann::json::object_t attribute;
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
        if (type != "Password")
        {
            fillBiosAttributeValue(
                attribute, "CurrentValue", type,
                std::get<uint(BaseTableAttributeIndex::CurrentValue)>(entry));
            fillBiosAttributeValue(
                attribute, "DefaultValue", type,
                std::get<uint(BaseTableAttributeIndex::DefaultValue)>(entry));
        }
        fillBiosAttributeBounds(
            attribute, type,
            std::get<uint(BaseTableAttributeIndex::Options)>(entry));
        attributes.emplace_back(std::move(attribute));
    }
    asyncResp->res.jsonValue["RegistryEntries"]["Attributes"] =
        std::move(attributes);
}

inline void handleBiosAttributeRegistryManagerObject(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& objectPath)
{
    bios_utils::getBIOSManagerProperty<BaseTable>(
        asyncResp, "BaseBIOSTable", objectPath,
        std::bind_front(fillBiosAttributeRegistryEntries, asyncResp));
}

inline void handleBiosAttributeRegistryGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Registries/{}/{}", bios_utils::biosAttributeRegistryId,
        bios_utils::biosAttributeRegistryId);
    asyncResp->res.jsonValue["@odata.type"] =
        "#AttributeRegistry.v1_5_0.AttributeRegistry";
    asyncResp->res.jsonValue["Id"] = bios_utils::biosAttributeRegistryId;
    asyncResp->res.jsonValue["Name"] = "BIOS Attribute Registry";
    asyncResp->res.jsonValue["Description"] =
        "Attribute registry for the BIOS configuration of this system";
    asyncResp->res.jsonValue["Language"] = "en";
    asyncResp->res.jsonValue["OwningEntity"] = "OpenBMC";
    asyncResp->res.jsonValue["RegistryVersion"] =
        bios_utils::biosAttributeRegistryVersion;
    asyncResp->res.jsonValue["SupportedSystems"] = nlohmann::json::array_t{};
    asyncResp->res.jsonValue["RegistryEntries"]["Attributes"] =
        nlohmann::json::array_t{};

    bios_utils::getBIOSManagerObject(
        asyncResp,
        std::bind_front(handleBiosAttributeRegistryManagerObject, asyncResp));
}

inline void handleBiosAttributeRegistryFileGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Registries/{}", bios_utils::biosAttributeRegistryId);
    asyncResp->res.jsonValue["@odata.type"] =
        "#MessageRegistryFile.v1_1_0.MessageRegistryFile";
    asyncResp->res.jsonValue["Id"] = bios_utils::biosAttributeRegistryId;
    asyncResp->res.jsonValue["Name"] = "BIOS Attribute Registry File";
    asyncResp->res.jsonValue["Description"] =
        "BIOS Attribute Registry File Location";
    asyncResp->res.jsonValue["Registry"] =
        bios_utils::biosAttributeRegistryName;
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

} // namespace redfish
