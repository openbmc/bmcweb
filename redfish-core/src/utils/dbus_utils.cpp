#include "utils/dbus_utils.hpp"

#include "async_resp.hpp"

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

void afterSetProperty(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& redfishPropertyName,
                      const nlohmann::json& propertyValue,
                      const boost::system::error_code& ec,
                      const sdbusplus::message_t& msg)
{
    if (ec)
    {
        if (ec.value() == boost::system::errc::permission_denied)
        {
            messages::insufficientPrivilege(asyncResp->res);
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
        }
        BMCWEB_LOG_ERROR("D-Bus error setting Redfish Property {} ec={}",
                         redfishPropertyName, ec);
        messages::internalError(asyncResp->res);
        return;
    }
    // Only set 204 if another erro hasn't already happened.
    if (asyncResp->res.result() == boost::beast::http::status::ok)
    {
        asyncResp->res.result(boost::beast::http::status::no_content);
    }
};
} // namespace details
} // namespace redfish
