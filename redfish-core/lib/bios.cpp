#include "../../http/http_request_class_decl.hpp"
#include "../../http/http_response_class_decl.hpp"
#include <sdbusplus/message.hpp>
#include "bios.hpp"

template<>
auto& crow::App::route<0>(std::string&& rule) 
{
    return router.newRuleTagged<0>(std::move(rule));
}

namespace redfish {

/**
 * BiosReset class supports handle POST method for Reset bios.
 * The class retrieves and sends data directly to D-Bus.
 *
 * Function handles POST method request.
 * Analyzes POST body message before sends Reset request data to D-Bus.
 */
inline void
    handleBiosResetPost(const crow::Request&,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Failed to reset bios: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
        },
        "org.open_power.Software.Host.Updater", "/xyz/openbmc_project/software",
        "xyz.openbmc_project.Common.FactoryReset", "Reset");
}

void requestRoutesBiosService(crow::App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Bios/")
        .privileges(redfish::privileges::getBios)
        .methods(boost::beast::http::verb::get)(handleBiosServiceGet);
}

void requestRoutesBiosReset(crow::App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Bios/Actions/Bios.ResetBios/")
        .privileges(redfish::privileges::postBios)
        .methods(boost::beast::http::verb::post)(handleBiosResetPost);
}

} // namespace redfish