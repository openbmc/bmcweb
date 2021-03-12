#pragma once

#include "boost/system/error_code.hpp"
#include "utils/collection.hpp"
#include "utils/json_utils.hpp"

#include "functional"
#include "memory"
#include "string"
#include "vector"

namespace redfish
{

inline void handleAdapterError(const boost::system::error_code& ec,
                               crow::Response& res, const std::string& adapter)
{

    if (ec.value() == boost::system::errc::io_error)
    {
        messages::resourceNotFound(res, "#FabricAdapter.v1_0_0.FabricAdapter",
                                   adapter);
        return;
    }

    BMCWEB_LOG_ERROR << "DBus method call failed with error " << ec.value();
    messages::internalError(res);
}

inline void handleFabricAdapterGet(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const dbus::utility::MapperGetSubTreeResponse& subtree,
    const std::string& adapter, const boost::system::error_code& ec)
{
    if (ec)
    {
        handleAdapterError(ec, aResp->res, adapter);
        return;
    }
    for (const auto& [objectPath, serviceMap] : subtree)
    {
        std::string adapterId =
            sdbusplus::message::object_path(objectPath).filename();

        if (adapterId.empty())
        {
            BMCWEB_LOG_ERROR << "Failed to find / in adapter path";
            messages::internalError(aResp->res);
            return;
        }

        if (adapterId != adapter)
        {
            // this is not the adapter we are interested in
            continue;
        }

        aResp->res.addHeader(
            boost::beast::http::field::link,
            "</redfish/v1/JsonSchemas/FabricAdapter/FabricAdapter.json>; rel=describedby");
        aResp->res.jsonValue["@odata.type"] =
            "#FabricAdapter.v1_0_0.FabricAdapter";
        aResp->res.jsonValue["Name"] = "FabricAdapter";
        aResp->res.jsonValue["Id"] = adapterId;
        aResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
            "redfish", "v1", "Systems", "system", "FabricAdapters", adapterId);

        return;
    }
    BMCWEB_LOG_WARNING << "Adapter not found";
    messages::resourceNotFound(aResp->res, "FabricAdapter", adapter);
}

/**
 * @brief Api to look for specific fabric adapter among
 * all available Fabric adapters on a system.
 *
 */
inline void getAdapter(App& app, const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                       const std::string& systemName,
                       const std::string& adapter)
{
    if (!redfish::setUpRedfishRoute(app, req, aResp))
    {
        return;
    }

    if (systemName != "system")
    {
        messages::resourceNotFound(aResp->res, "ComputerSystem", systemName);
        return;
    }

    std::vector<std::string> interfaces = {
        "xyz.openbmc_project.Inventory.Item.FabricAdapter"};

    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", interfaces,
        [adapter,
         aResp](const boost::system::error_code& ec,
                const dbus::utility::MapperGetSubTreeResponse& subtree) {
        handleFabricAdapterGet(aResp, subtree, adapter, ec);
        });
}

inline void handleFabricAdapterCollectionGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, aResp))
    {
        return;
    }
    if (systemName != "system")
    {
        messages::resourceNotFound(aResp->res, "ComputerSystem", systemName);
        return;
    }
    aResp->res.jsonValue["@odata.type"] =
        "#FabricAdapterCollection.FabricAdapterCollection";
    aResp->res.jsonValue["Name"] = "Fabric Adapter Collection";
    aResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Systems", systemName, "FabricAdapters");

    constexpr std::array<std::string_view, 1> interfaces{
        "xyz.openbmc_project.Inventory.Item.FabricAdapter"};
    collection_util::getCollectionMembers(
        aResp, boost::urls::url("/redfish/v1/Systems/system/FabricAdapters"),
        interfaces);
}

inline void requestRoutesFabricAdapterCollection(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/FabricAdapters/")
        .privileges(redfish::privileges::getFabricAdapterCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFabricAdapterCollectionGet, std::ref(app)));
}

/**
 * Systems derived class for delivering Fabric adapter Schema.
 */
inline void requestRoutesFabricAdapters(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/FabricAdapters/<str>/")
        .privileges(redfish::privileges::getFabricAdapter)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(getAdapter, std::ref(app)));
}
} // namespace redfish
