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
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>

namespace redfish
{
namespace bios_utils
{
static constexpr std::string_view biosConfigManagerPath =
    "/xyz/openbmc_project/bios_config/manager";
static constexpr std::string_view biosConfigManagerInterface =
    "xyz.openbmc_project.BIOSConfig.Manager";

using BiosAttributeType = std::string;
using BiosAttributeValue = std::variant<int64_t, std::string, bool>;

static constexpr std::string_view biosAttributeTypeEnumeration =
    "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Enumeration";
static constexpr std::string_view biosAttributeTypeString =
    "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.String";
static constexpr std::string_view biosAttributeTypePassword =
    "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Password";
static constexpr std::string_view biosAttributeTypeInteger =
    "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Integer";
static constexpr std::string_view biosAttributeTypeBoolean =
    "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Boolean";

inline void addAttribute(nlohmann::json& attributes, const std::string& name,
                         const BiosAttributeType& type,
                         const BiosAttributeValue& value)
{
    if (type == biosAttributeTypeEnumeration ||
        type == biosAttributeTypeString || type == biosAttributeTypePassword)
    {
        const std::string* stringValue = std::get_if<std::string>(&value);
        attributes[name] =
            stringValue == nullptr ? std::string{} : *stringValue;
        return;
    }
    if (type == biosAttributeTypeInteger)
    {
        const int64_t* intValue = std::get_if<int64_t>(&value);
        attributes[name] = intValue == nullptr ? int64_t{} : *intValue;
        return;
    }
    if (type == biosAttributeTypeBoolean)
    {
        const bool* boolValue = std::get_if<bool>(&value);
        attributes[name] = boolValue == nullptr ? false : *boolValue;
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
inline void handleBIOSManagerObject(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, CallbackFunc& callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetObject& object)
{
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
            const dbus::utility::MapperGetObject& object) mutable {
            handleBIOSManagerObject(asyncResp, callback, ec, object);
        });
}

using PendingAttributeValue = std::tuple<BiosAttributeType, BiosAttributeValue>;
enum class PendingAttributeValueIndex
{
    Type = 0,
    Value
};

using PendingAttributes = std::map<std::string, PendingAttributeValue>;

inline void setBIOSManagerProperty(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& propertyName, const PendingAttributes& propertyValue,
    const std::string& objectPath)
{
    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, objectPath,
        std::string(biosConfigManagerPath),
        std::string(biosConfigManagerInterface), propertyName, propertyValue,
        [asyncResp, propertyName](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("DBus response error for setting {}: {}",
                                 propertyName, ec);
                messages::internalError(asyncResp->res);
                return;
            }
        });
}

} // namespace bios_utils
} // namespace redfish
