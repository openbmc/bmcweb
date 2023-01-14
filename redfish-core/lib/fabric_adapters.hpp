#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/collection.hpp"
#include "utils/json_utils.hpp"

#include <boost/system/error_code.hpp>

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace redfish
{

inline void handleAdapterError(const boost::system::error_code& ec,
                               crow::Response& res,
                               const std::string& adapterId)
{

    if (ec.value() == boost::system::errc::io_error)
    {
        messages::resourceNotFound(res, "#FabricAdapter.v1_0_0.FabricAdapter",
                                   adapterId);
        return;
    }

    BMCWEB_LOG_ERROR << "DBus method call failed with error " << ec.value();
    messages::internalError(res);
}

inline void doAdapterGet(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                         const std::string& systemName,
                         const std::string& adapterId)
{
    aResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/FabricAdapter/FabricAdapter.json>; rel=describedby");
    aResp->res.jsonValue["@odata.type"] = "#FabricAdapter.v1_0_0.FabricAdapter";
    aResp->res.jsonValue["Name"] = "Fabric Adapter";
    aResp->res.jsonValue["Id"] = adapterId;
    aResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Systems", systemName, "FabricAdapters", adapterId);
}

inline bool checkFabricAdapterId(const std::string& adapterPath,
                                 const std::string& adapterId)
{
    std::string fabricAdapterName =
        sdbusplus::message::object_path(adapterPath).filename();

    return !(fabricAdapterName.empty() || fabricAdapterName != adapterId);
}

inline void getValidFabricAdapterPath(
    const std::string& adapterId, const std::string& systemName,
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    std::function<void(const std::string& fabricAdapterPath,
                       const std::string& serviceName)>&& callback)
{
    if (systemName != "system")
    {
        messages::resourceNotFound(aResp->res, "ComputerSystem", systemName);
        return;
    }
    constexpr std::array<std::string_view, 1> interfaces{
        "xyz.openbmc_project.Inventory.Item.FabricAdapter"};

    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [adapterId, aResp,
         callback](const boost::system::error_code& ec,
                   const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            handleAdapterError(ec, aResp->res, adapterId);
            return;
        }
        for (const auto& [adapterPath, serviceMap] : subtree)
        {
            if (checkFabricAdapterId(adapterPath, adapterId))
            {
                callback(adapterPath, serviceMap.begin()->first);
                return;
            }
        }
        BMCWEB_LOG_WARNING << "Adapter not found";
        messages::resourceNotFound(aResp->res, "FabricAdapter", adapterId);
        });
}

inline void
    handleFabricAdapterGet(App& app, const crow::Request& req,
                           const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                           const std::string& systemName,
                           const std::string& adapterId)
{
    if (!redfish::setUpRedfishRoute(app, req, aResp))
    {
        return;
    }

    getValidFabricAdapterPath(
        adapterId, systemName, aResp,
        [aResp, systemName, adapterId](const std::string&, const std::string&) {
        doAdapterGet(aResp, systemName, adapterId);
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

    aResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/FabricAdapterCollection/FabricAdapterCollection.json>; rel=describedby");
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

inline void handleFabricAdapterCollectionHead(
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
    aResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/FabricAdapterCollection/FabricAdapterCollection.json>; rel=describedby");
}

inline void
    handleFabricAdapterHead(crow::App& app, const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                            const std::string& systemName,
                            const std::string& adapterId)
{
    if (!redfish::setUpRedfishRoute(app, req, aResp))
    {
        return;
    }

    getValidFabricAdapterPath(
        adapterId, systemName, aResp,
        [aResp, systemName, adapterId](const std::string&, const std::string&) {
        aResp->res.addHeader(
            boost::beast::http::field::link,
            "</redfish/v1/JsonSchemas/FabricAdapter/FabricAdapter.json>; rel=describedby");
        });
}

inline void requestRoutesFabricAdapterCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/FabricAdapters/")
        .privileges(redfish::privileges::getFabricAdapterCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFabricAdapterCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/FabricAdapters/")
        .privileges(redfish::privileges::headFabricAdapterCollection)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleFabricAdapterCollectionHead, std::ref(app)));
}

inline void requestRoutesFabricAdapters(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/FabricAdapters/<str>/")
        .privileges(redfish::privileges::getFabricAdapter)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFabricAdapterGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/FabricAdapters/<str>/")
        .privileges(redfish::privileges::headFabricAdapter)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleFabricAdapterHead, std::ref(app)));
}
} // namespace redfish
