
#pragma once

#include "bmcweb_config.h"

#include <app.hpp>

#include <string>

namespace redfish
{

inline void
    getHandleOemOpenBmc(const crow::Request& /*req*/,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& /*managerId*/)
{
    // Default OEM data
    nlohmann::json& oemOpenbmc = asyncResp->res.jsonValue;
    oemOpenbmc["@odata.type"] = "#OpenBMCManager.v1_0_0.Manager";
    oemOpenbmc["@odata.id"] =
        boost::urls::format("/redfish/v1/Managers/{}#/Oem/OpenBmc",
                            BMCWEB_REDFISH_MANAGER_URI_NAME);

    nlohmann::json::object_t certificates;
    certificates["@odata.id"] =
        boost::urls::format("/redfish/v1/Managers/{}/Truststore/Certificates",
                            BMCWEB_REDFISH_MANAGER_URI_NAME);
    oemOpenbmc["Certificates"] = std::move(certificates);
}

inline void requestRoutesOpenBmcManager(App& app)
{
    BMCWEB_OEM_ROUTE(app, "/redfish/v1/Managers/<str>#Oem/OpenBmc")
        .privileges(redfish::privileges::getManager)
        .setGetHandler(boost::beast::http::verb::get)(getHandleOemOpenBmc);
}

} // namespace redfish
