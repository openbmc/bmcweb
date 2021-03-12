#pragma once

#include <utils/collection.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

using ServiceMap =
    std::vector<std::pair<std::string, std::vector<std::string>>>;

inline void handleAdapterError(const boost::system::error_code& ec,
                               const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                               const std::string& adapter)
{
    BMCWEB_LOG_ERROR << "DBus method call failed with error " << ec.value();

    if (ec.value() == boost::system::errc::io_error)
    {
        messages::resourceNotFound(
            aResp->res, "#FabricAdapter.v1_0_0.FabricAdapter", adapter);
        return;
    }

    BMCWEB_LOG_DEBUG << "DBUS response error";
    messages::internalError(aResp->res);
    return;
}

/**
 * @brief Api to look for specific fabric adapter among
 * all available Fabric adapters on a system.
 *
 * @param[in,out]   aResp       Async HTTP response.
 * @param[in]       adapter     Fabric adapter to look for.
 */
inline void getAdapter(crow::App& app, const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                       const std::string& adapter)
{
    if (!redfish::setUpRedfishRoute(app, req, aResp))
    {
        return;
    }

    auto respHandler =
        [adapter,
         aResp](const boost::system::error_code ec,
                const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            handleAdapterError(ec, aResp, adapter);
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
            aResp->res.jsonValue["@odata.type"] =
                "#FabricAdapter.v1_0_0.FabricAdapter";
            aResp->res.jsonValue["@odata.id"] =
                "/redfish/v1/Systems/system/FabricAdapters/" + adapter;
            aResp->res.jsonValue["Id"] = adapterId;
            aResp->res.jsonValue["Name"] = "FabricAdapter";

            return;
        }
        BMCWEB_LOG_ERROR << "Adapter not found";
        messages::resourceNotFound(aResp->res, "FabricAdapter", adapter);
    };

    std::vector<std::string> interfaces = {
        "xyz.openbmc_project.Inventory.Item.FabricAdapter"};
    dbus::utility::getSubTree("/xyz/openbmc_project/inventory", interfaces,
                              std::move(respHandler));
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
    aResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Systems/system/FabricAdapters";

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
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/FabricAdapters/<str>/")
        .privileges(redfish::privileges::getFabricAdapter)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(getAdapter, std::ref(app)));
}
} // namespace redfish
