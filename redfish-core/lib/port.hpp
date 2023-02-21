#pragma once

#include "app.hpp"
#include "fabric_adapters.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"

#include <array>
#include <memory>
#include <string>
#include <string_view>

namespace redfish
{

inline void getPortCollection(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                              const std::string& systemName,
                              const std::string& fabricAdapterPath,
                              const std::string& adapterId)
{
    BMCWEB_LOG_DEBUG << "Get Port collection members for: " << adapterId;

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Connector"};

    dbus::utility::getSubTreePaths(
        fabricAdapterPath, 0, interfaces,
        [aResp{std::move(aResp)}, systemName,
         adapterId](const boost::system::error_code& ec,
                    const dbus::utility::MapperGetSubTreePathsResponse& paths) {
        if (ec == boost::system::errc::io_error)
        {
            aResp->res.jsonValue["Members"] = nlohmann::json::array();
            aResp->res.jsonValue["Members@odata.count"] = 0;
            return;
        }

        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error " << ec.value();
            messages::internalError(aResp->res);
            return;
        }
        nlohmann::json& members = aResp->res.jsonValue["Members"];
        members = nlohmann::json::array();

        for (const auto& path : paths)
        {
            std::string adapterPath =
                sdbusplus::message::object_path(path).parent_path();
            if (!checkFabricAdapterId(adapterPath, adapterId))
            {
                continue;
            }

            std::string connectorId =
                sdbusplus::message::object_path(path).filename();
            nlohmann::json item;
            item["@odata.id"] = crow::utility::urlFromPieces(
                "redfish", "v1", "Systems", systemName, "FabricAdapters",
                adapterId, "Ports", connectorId);
            members.emplace_back(std::move(item));
        }
        aResp->res.jsonValue["Members@odata.count"] = members.size();
        });
}

inline void
    handlePortCollectionHead(crow::App& app, const crow::Request& req,
                             const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             const std::string& systemName,
                             const std::string& adapterId)
{
    if (!redfish::setUpRedfishRoute(app, req, aResp))
    {
        return;
    }

    getValidFabricAdapterPath(adapterId, systemName, aResp,
                              [aResp](const std::string&, const std::string&) {
        aResp->res.addHeader(
            boost::beast::http::field::link,
            "</redfish/v1/JsonSchemas/PortCollection/PortCollection.json>; rel=describedby");
    });
}

inline void doPortCollectionGet(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                                const std::string& systemName,
                                const std::string& fabricAdapterPath,
                                const std::string& adapterId)
{
    aResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/PortCollection/PortCollection.json>; rel=describedby");
    aResp->res.jsonValue["@odata.type"] = "#PortCollection.PortCollection";
    aResp->res.jsonValue["Name"] = "Port Collection";
    aResp->res.jsonValue["@odata.id"] =
        crow::utility::urlFromPieces("redfish", "v1", "Systems", systemName,
                                     "FabricAdapters", adapterId, "Ports");

    getPortCollection(aResp, systemName, fabricAdapterPath, adapterId);
}

inline void
    handlePortCollectionGet(App& app, const crow::Request& req,
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
        [aResp{std::move(aResp)}, systemName,
         adapterId](const std::string& fabricAdapterPath, const std::string&) {
        doPortCollectionGet(aResp, systemName, fabricAdapterPath, adapterId);
        });
}

inline void requestRoutesPortCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/FabricAdapters/<str>/Ports/")
        .privileges(redfish::privileges::headPortCollection)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handlePortCollectionHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/FabricAdapters/<str>/Ports/")
        .privileges(redfish::privileges::getPortCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePortCollectionGet, std::ref(app)));
}

} // namespace redfish
