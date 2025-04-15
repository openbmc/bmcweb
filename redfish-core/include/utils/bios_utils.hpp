// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
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

template <typename Type>
inline void extractValue(nlohmann::json& attributes, const std::string& name,
                         const dbus::utility::DbusVariantType& value)
{
    const Type* tValue = std::get_if<Type>(&value);
    if (tValue != nullptr)
    {
        attributes[name] = *tValue;
        return;
    }
    attributes[name] = Type{};
}

template <>
inline void extractValue<bool>(nlohmann::json& attributes,
                               const std::string& name,
                               const dbus::utility::DbusVariantType& value)
{
    const int64_t* tValue = std::get_if<int64_t>(&value);
    if (tValue != nullptr)
    {
        attributes[name] = (*tValue != 0);
        return;
    }
    attributes[name] = false;
}

using HandlerType = std::function<void(nlohmann::json&, const std::string&,
                                       const dbus::utility::DbusVariantType&)>;

inline void addAttribute(nlohmann::json& attributes, const std::string& name,
                         const dbus::utility::DbusVariantType& type,
                         const dbus::utility::DbusVariantType& value)
{
    static const std::unordered_map<std::string, HandlerType> typeMap = {
        {"xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Enumeration",
         extractValue<std::string>},
        {"xyz.openbmc_project.BIOSConfig.Manager.AttributeType.String",
         extractValue<std::string>},
        {"xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Password",
         extractValue<std::string>},
        {"xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Integer",
         extractValue<int64_t>},
        {"xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Boolean",
         extractValue<bool>}};

    const std::string* typeStr = std::get_if<std::string>(&type);
    if (typeStr != nullptr)
    {
        auto it = typeMap.find(*typeStr);
        if (it != typeMap.end())
        {
            it->second(attributes, name, value);
        }
    }
}

template <typename T>
inline void getBIOSManagerProperty(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& property, const std::string& objectPath,
    std::function<void(const T&)> handler)
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
