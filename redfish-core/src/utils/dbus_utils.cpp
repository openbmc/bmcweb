// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "utils/dbus_utils.hpp"

#include "async_resp.hpp"
#include "boost_formatters.hpp"
#include "error_messages.hpp"
#include "logging.hpp"

#include <systemd/sd-bus.h>

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/error_code.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message.hpp>

#include <memory>
#include <string>
#include <string_view>

namespace redfish
{
namespace details
{

void afterSetProperty(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& redfishPropertyName, const nlohmann::json& propertyValue,
    const boost::system::error_code& ec, const sdbusplus::message_t& msg)
{
    if (ec)
    {
        if (ec.value() == boost::system::errc::permission_denied)
        {
            messages::insufficientPrivilege(asyncResp->res);
        }
        if (ec.value() == boost::asio::error::host_unreachable)
        {
            messages::resourceNotFound(asyncResp->res, "Set",
                                       redfishPropertyName);
            return;
        }
        const sd_bus_error* dbusError = msg.get_error();
        if (dbusError != nullptr)
        {
            std::string_view errorName(dbusError->name);

            if (errorName == "xyz.openbmc_project.Common.Error.InvalidArgument")
            {
                BMCWEB_LOG_WARNING("DBUS response error: {}", ec);
                messages::propertyValueIncorrect(
                    asyncResp->res, redfishPropertyName, propertyValue);
                return;
            }
            if (errorName ==
                "xyz.openbmc_project.State.Chassis.Error.BMCNotReady")
            {
                BMCWEB_LOG_WARNING(
                    "BMC not ready, operation not allowed right now");
                messages::serviceTemporarilyUnavailable(asyncResp->res, "10");
                return;
            }
            if (errorName == "xyz.openbmc_project.State.Host.Error.BMCNotReady")
            {
                BMCWEB_LOG_WARNING(
                    "BMC not ready, operation not allowed right now");
                messages::serviceTemporarilyUnavailable(asyncResp->res, "10");
                return;
            }
            if (errorName == "xyz.openbmc_project.Common.Error.NotAllowed")
            {
                messages::propertyNotWritable(asyncResp->res,
                                              redfishPropertyName);
                return;
            }
            if (errorName == "xyz.openbmc_project.Common.Error.Unavailable")
            {
                messages::propertyValueExternalConflict(
                    asyncResp->res, redfishPropertyName, propertyValue);
                return;
            }
        }
        BMCWEB_LOG_ERROR("D-Bus error setting Redfish Property {} ec={}",
                         redfishPropertyName, ec);
        messages::internalError(asyncResp->res);
        return;
    }
    // Only set 204 if another error hasn't already happened.
    if (asyncResp->res.result() == boost::beast::http::status::ok)
    {
        asyncResp->res.result(boost::beast::http::status::no_content);
    }
};

void afterSetPropertyAction(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& redfishActionName,
                            const std::string& redfishActionParameterName,
                            const boost::system::error_code& ec,
                            const sdbusplus::message_t& /*msg*/)
{
    if (ec)
    {
        if (ec.value() == boost::asio::error::invalid_argument)
        {
            BMCWEB_LOG_WARNING(
                "Resource {} is patched with invalid argument during action {}",
                redfishActionParameterName, redfishActionName);
            if (redfishActionParameterName.empty())
            {
                messages::operationFailed(asyncResp->res);
            }
            else
            {
                messages::actionParameterValueError(asyncResp->res,
                                                    redfishActionParameterName,
                                                    redfishActionName);
            }
            return;
        }
        if (ec.value() == boost::asio::error::host_unreachable)
        {
            BMCWEB_LOG_WARNING(
                "Resource {} is not found while performing action {}",
                redfishActionParameterName, redfishActionName);
            messages::resourceNotFound(asyncResp->res, "Actions",
                                       redfishActionName);
            return;
        }

        BMCWEB_LOG_ERROR("D-Bus error setting Redfish Property {} ec={}",
                         redfishActionParameterName, ec);
        messages::internalError(asyncResp->res);
        return;
    }
    // Only set success if another error hasn't already happened.
    if (asyncResp->res.result() == boost::beast::http::status::ok)
    {
        messages::success(asyncResp->res);
    }
};
} // namespace details
} // namespace redfish
