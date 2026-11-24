// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "boost_formatters.hpp"
#include "dbus_singleton.hpp"
#include "error_messages.hpp"
#include "logging.hpp"

#include <systemd/sd-bus.h>

#include <boost/system/error_code.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <functional>
#include <memory>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

namespace redfish
{
namespace dbus_utils
{

struct UnpackErrorPrinter
{
    void operator()(const sdbusplus::UnpackErrorReason reason,
                    const std::string& property) const noexcept
    {
        BMCWEB_LOG_ERROR(
            "DBUS property error in property: {}, reason: {}", property,
            static_cast<std::underlying_type_t<sdbusplus::UnpackErrorReason>>(
                reason));
    }
};

/**
 * @brief Callback that handles one D-Bus error code.
 *
 * @param[in] ec         Error code of the D-Bus reply.
 * @param[in] errorName  D-Bus error name of the reply, or empty when the
 *                       reply message is not available.
 */
using DbusErrorCallback = std::function<void(
    const boost::system::error_code& ec, std::string_view errorName)>;

/**
 * @brief Log a missing D-Bus object or interface without failing the
 * request.
 *
 * @param[in] ec  Error code of the D-Bus reply.
 */
inline void handleEBADR(const boost::system::error_code& ec,
                        std::string_view /*errorName*/)
{
    BMCWEB_LOG_DEBUG("D-Bus object not found: {}", ec);
}

/**
 * @brief Handle the error code of a D-Bus reply.
 *
 * An error code with an entry in @p callbacks runs that callback with the
 * D-Bus error name of @p msg, so the callback can tell apart D-Bus errors
 * that share an errno. The callback must set any error response itself.
 * Any other error is logged and reported as InternalError.
 *
 * @param[in,out] asyncResp  Async HTTP response.
 * @param[in]     ec         Error code of the D-Bus reply.
 * @param[in]     msg        D-Bus reply message, if available.
 * @param[in]     callbacks  Error codes mapped to the callback that handles
 *                           them.
 * @param[in]     loc        Location of the caller.
 *
 * @return true if there is no error; false if the error was handled here.
 */
inline bool handleDbusError(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, const sdbusplus::message_t& msg = {},
    const std::unordered_map<int, DbusErrorCallback>& callbacks = {},
    const std::source_location& loc = std::source_location::current())
{
    if (!ec)
    {
        return true;
    }
    auto callback = callbacks.find(ec.value());
    if (callback != callbacks.end())
    {
        std::string_view errorName;
        const sd_bus_error* dbusError = msg.get_error();
        if (dbusError != nullptr && dbusError->name != nullptr)
        {
            errorName = dbusError->name;
        }
        callback->second(ec, errorName);
        return false;
    }
    BMCWEB_LOG_ERROR("D-Bus response error {}", ec);
    messages::internalError(asyncResp->res, loc);
    return false;
}

} // namespace dbus_utils

namespace details
{
void afterSetProperty(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& redfishPropertyName, const nlohmann::json& propertyValue,
    const boost::system::error_code& ec, const sdbusplus::message_t& msg);

void afterSetPropertyAction(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& redfishActionName,
                            const std::string& redfishActionParameterName,
                            const boost::system::error_code& ec,
                            const sdbusplus::message_t& msg);
} // namespace details

template <typename PropertyType>
void setDbusProperty(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::string_view redfishPropertyName, std::string_view processName,
    const sdbusplus::object_path& path, std::string_view interface,
    std::string_view dbusProperty, const PropertyType& prop)
{
    std::string processNameStr(processName);
    std::string interfaceStr(interface);
    std::string dbusPropertyStr(dbusProperty);

    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, processNameStr, path.str, interfaceStr,
        dbusPropertyStr, prop,
        [asyncResp, redfishPropertyNameStr = std::string{redfishPropertyName},
         jsonProp = nlohmann::json(prop)](const boost::system::error_code& ec,
                                          const sdbusplus::message_t& msg) {
            details::afterSetProperty(asyncResp, redfishPropertyNameStr,
                                      jsonProp, ec, msg);
        });
}

template <typename DbusPropertyType>
void setDbusPropertyAction(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::string_view processName, const sdbusplus::object_path& path,
    std::string_view interface, std::string_view dbusProperty,
    std::string_view redfishActionParameterName,
    std::string_view redfishActionName, const DbusPropertyType& prop)
{
    std::string processNameStr(processName);
    std::string interfaceStr(interface);
    std::string dbusPropertyStr(dbusProperty);

    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, processNameStr, path.str, interfaceStr,
        dbusPropertyStr, prop,
        [asyncResp,
         redfishActionParameterName = std::string{redfishActionParameterName},
         jsonProp = nlohmann::json(prop),
         redfishActionNameStr = std::string{redfishActionName}](
            const boost::system::error_code& ec,
            const sdbusplus::message_t& msg) {
            details::afterSetPropertyAction(asyncResp, redfishActionNameStr,
                                            redfishActionParameterName, ec,
                                            msg);
        });
}

} // namespace redfish
