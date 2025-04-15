// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/attribute_registry.hpp"
#include "logging.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

namespace redfish
{
namespace bios_utils
{
static constexpr std::string_view biosConfigManagerPath =
    "/xyz/openbmc_project/bios_config/manager";
static constexpr std::string_view biosConfigManagerInterface =
    "xyz.openbmc_project.BIOSConfig.Manager";

inline void addAttribute(nlohmann::json& attributes, const std::string& name,
                         const dbus::utility::DbusVariantType& type,
                         const dbus::utility::DbusVariantType& value)
{
    static const std::unordered_map<std::string,
                                    attribute_registry::AttributeType>
        typeMap = {
            {"xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Enumeration",
             attribute_registry::AttributeType::Enumeration},
            {"xyz.openbmc_project.BIOSConfig.Manager.AttributeType.String",
             attribute_registry::AttributeType::String},
            {"xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Password",
             attribute_registry::AttributeType::Password},
            {"xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Integer",
             attribute_registry::AttributeType::Integer},
            {"xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Boolean",
             attribute_registry::AttributeType::Boolean}};

    attribute_registry::AttributeType attributeType =
        attribute_registry::AttributeType::Invalid;
    const std::string* typeStr = std::get_if<std::string>(&type);
    if (typeStr != nullptr)
    {
        auto it = typeMap.find(*typeStr);
        if (it != typeMap.end())
        {
            attributeType = it->second;
        }
    }

    if (attributeType == attribute_registry::AttributeType::Boolean)
    {
        const int64_t* boolValue = std::get_if<int64_t>(&value);
        if (boolValue != nullptr)
        {
            attributes[name] = (*boolValue != 0);
            return;
        }
        attributes[name] = false;
        BMCWEB_LOG_ERROR("Invalid Boolean value for attribute {}", name);
        return;
    }

    if (attributeType == attribute_registry::AttributeType::Integer)
    {
        const int64_t* intValue = std::get_if<int64_t>(&value);
        if (intValue != nullptr)
        {
            attributes[name] = *intValue;
            return;
        }
        attributes[name] = 0;
        BMCWEB_LOG_ERROR("Invalid Integer value for attribute {}", name);
        return;
    }

    if (attributeType == attribute_registry::AttributeType::String ||
        attributeType == attribute_registry::AttributeType::Enumeration)
    {
        const std::string* strValue = std::get_if<std::string>(&value);
        if (strValue != nullptr)
        {
            attributes[name] = *strValue;
            return;
        }
        attributes[name] = "";
        BMCWEB_LOG_ERROR("Invalid String value for attribute {}", name);
        return;
    }
    BMCWEB_LOG_ERROR("Invalid type for attribute {} in base table", name);
}

template <typename T>
inline void getBIOSManagerProperty(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& property, std::function<void(const T&)> handler,
    const std::string& objectPath)
{
    sdbusplus::asio::getProperty<T>(
        *crow::connections::systemBus, objectPath,
        std::string(biosConfigManagerPath),
        std::string(biosConfigManagerInterface), property,
        [asyncResp, property, handler{std::move(handler)}](
            const boost::system::error_code& ec, const T& value) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("DBus response error for {}: {}", property,
                                 ec);
                messages::internalError(asyncResp->res);
                return;
            }
            handler(value);
        });
}

template <typename CallbackFunc>
inline void getBIOSManagerObject(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    CallbackFunc&& callback)
{
    dbus::utility::getDbusObject(
        std::string(biosConfigManagerPath),
        std::array<std::string_view, 1>{biosConfigManagerInterface},
        [asyncResp, callback = std::forward<CallbackFunc>(callback)](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetObject& object) {
            if (ec || object.empty())
            {
                BMCWEB_LOG_ERROR("Error finding BIOS Manager object {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }
            if (object.size() > 1)
            {
                BMCWEB_LOG_ERROR("More than one BIOS Manager object found");
                messages::internalError(asyncResp->res);
                return;
            }
            callback(object.begin()->first);
        });
}

} // namespace bios_utils
} // namespace redfish
