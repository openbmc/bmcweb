#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "registries/privilege_registry.hpp"
#include "utility.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"

#include <boost/system/error_code.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <string_view>

namespace redfish
{

inline void handleComponentIntegrityCollectionGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.jsonValue["@odata.type"] =
        "#ComponentIntegrityCollection.ComponentIntegrityCollection";
    asyncResp->res.jsonValue["Name"] = "Component Integrity Collection";
    asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/ComponentIntegrity";

    // TODO Get into PDI
    constexpr std::array<std::string_view, 1> interfaces{
        "xyz.openbmc_project.Inventory.Item.RootOfTrust"};
    collection_util::getCollectionMembers(
        asyncResp, boost::urls::url("/redfish/v1/ComponentIntegrity"),
        interfaces);
}
inline void handleComponentIntegrityGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& componentId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.jsonValue["@odata.type"] =
        "#ComponentIntegrity.ComponentIntegrity";
    asyncResp->res.jsonValue["Name"] = "Component Integrity";
    asyncResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "ComponentIntegrity", componentId);
}
inline void requestRoutesComponentIntegrity(App& app)
{
    // TODO(ed) wait until dmtf privileges are finished

    BMCWEB_ROUTE(app, "/redfish/v1/ComponentIntegrity/")
        .privileges(redfish::privileges::getServiceRoot)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleComponentIntegrityCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/ComponentIntegrity/<str>/")
        .privileges(redfish::privileges::getServiceRoot)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleComponentIntegrityGet, std::ref(app)));
}

} // namespace redfish
