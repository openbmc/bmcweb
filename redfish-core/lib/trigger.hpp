
#include "utils/telemetry_utils.hpp"

#include <app.hpp>
#include <error_messages.hpp>
#include <registries/privilege_registry.hpp>

namespace redfish
{

inline void requestRoutesTriggers(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/Triggers/<str>/")
        .privileges(redfish::privileges::deleteTriggers)
        .methods(boost::beast::http::verb::delete_)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& id) {
                const std::string triggerPath =
                    telemetry::getDbusTriggerPath(id);

                crow::connections::systemBus->async_method_call(
                    [asyncResp, id](const boost::system::error_code ec) {
                        /*
                         * boost::system::errc and std::errc are missing value
                         * for EBADR error that is defined in Linux.
                         */
                        if (ec.value() == EBADR)
                        {
                            messages::resourceNotFound(asyncResp->res,
                                                       "Triggers", id);
                            return;
                        }

                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        asyncResp->res.result(
                            boost::beast::http::status::no_content);
                    },
                    telemetry::service, triggerPath,
                    "xyz.openbmc_project.Object.Delete", "Delete");
            });
}

} // namespace redfish