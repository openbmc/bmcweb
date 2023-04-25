#pragma once

#include "app.hpp"
#include "fabric_adapters.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace redfish
{

inline void handlePortError(const boost::system::error_code& ec,
                            crow::Response& res, const std::string& portId)
{
    if (ec.value() == boost::system::errc::io_error)
    {
        messages::resourceNotFound(res, "#Port.v1_7_0.Port", portId);
        return;
    }

    BMCWEB_LOG_ERROR << "DBus method call failed with error " << ec.value();
    messages::internalError(res);
}

inline bool checkPortId(const std::string& portPath, const std::string& portId)
{
    std::string portName = sdbusplus::message::object_path(portPath).filename();

    return !(portName.empty() || portName != portId);
}

inline void getPortLocation(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                            const std::string& serviceName,
                            const std::string& portPath)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, serviceName, portPath,
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode",
        [aResp](const boost::system::error_code& ec, const std::string& value) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error";
            messages::internalError(aResp->res);
            return;
        }

        aResp->res.jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
            value;
        });
}

inline void getPortState(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                         const std::string& serviceName,
                         const std::string& portPath)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, serviceName, portPath,
        "xyz.openbmc_project.Inventory.Item", "Present",
        [aResp](const boost::system::error_code ec, const bool present) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR << "DBUS response error for State";
                messages::internalError(aResp->res);
            }
            return;
        }

        if (!present)
        {
            aResp->res.jsonValue["Status"]["State"] = "Absent";
        }
        });
}

inline void getPortHealth(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                          const std::string& serviceName,
                          const std::string& portPath)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, serviceName, portPath,
        "xyz.openbmc_project.State.Decorator.OperationalStatus", "Functional",
        [aResp](const boost::system::error_code& ec, const bool functional) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR << "DBUS response error for Health";
                messages::internalError(aResp->res);
            }
            return;
        }

        if (!functional)
        {
            aResp->res.jsonValue["Status"]["Health"] = "Critical";
        }
        });
}
/**
 * @brief Api to get Port properties.
 * @param[in,out]   aResp       Async HTTP response.
 * @param[in]       portPath    Object path of the Port.
 * @param[in]       service     Service and corresponding
 *                              interface list for the given port.
 */
inline void getPortProperties(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                              const std::string& systemName,
                              const std::string& adapterId,
                              const std::string& portId,
                              const std::string& portPath,
                              const std::string& serviceName)
{
    aResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/Port/Port.json>; rel=describedby");

    aResp->res.jsonValue["@odata.type"] = "#Port.v1_7_0.Port";
    aResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Systems", systemName, "FabricAdapters", adapterId,
        "Ports", portId);
    aResp->res.jsonValue["Id"] = portId;
    aResp->res.jsonValue["Name"] = portId;

    aResp->res.jsonValue["Status"]["State"] = "Enabled";
    aResp->res.jsonValue["Status"]["Health"] = "OK";

    getPortLocation(aResp, serviceName, portPath);
    getPortState(aResp, serviceName, portPath);
    getPortHealth(aResp, serviceName, portPath);
}

inline void getValidPortPath(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const std::string& adapterPath, const std::string& portId,
    std::function<void(const std::string& portPath,
                       const std::string& serviceName)>&& callback)
{
    constexpr std::array<std::string_view, 1> interfaces{
        "xyz.openbmc_project.Inventory.Item.Connector"};

    dbus::utility::getSubTree(
        adapterPath, 0, interfaces,
        [portId, aResp,
         callback](const boost::system::error_code& ec,
                   const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            handlePortError(ec, aResp->res, portId);
            return;
        }
        for (const auto& [portPath, serviceMap] : subtree)
        {
            if (checkPortId(portPath, portId))
            {
                callback(portPath, serviceMap.begin()->first);
                return;
            }
        }
        BMCWEB_LOG_WARNING << "Port not found";
        messages::resourceNotFound(aResp->res, "Port", portId);
        });
}

inline void handlePortHead(crow::App& app, const crow::Request& req,
                           const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                           const std::string& systemName,
                           const std::string& adapterId,
                           const std::string& portId)
{
    if (!redfish::setUpRedfishRoute(app, req, aResp))
    {
        return;
    }

    getValidFabricAdapterPath(
        adapterId, systemName, aResp,
        [aResp, portId](const std::string& adapterPath, const std::string&) {
        getValidPortPath(aResp, adapterPath, portId,
                         [aResp](const std::string&, const std::string&) {
            aResp->res.addHeader(
                boost::beast::http::field::link,
                "</redfish/v1/JsonSchemas/Port/Port.json>; rel=describedby");
        });
        });
}

inline void handlePortGet(App& app, const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                          const std::string& systemName,
                          const std::string& adapterId,
                          const std::string& portId)
{
    if (!redfish::setUpRedfishRoute(app, req, aResp))
    {
        return;
    }

    getValidFabricAdapterPath(
        adapterId, systemName, aResp,
        [aResp, portId, adapterId, systemName](const std::string& adapterPath,
                                               const std::string&) {
        getValidPortPath(aResp, adapterPath, portId,
                         [aResp, adapterId, systemName,
                          portId](const std::string& portPath,
                                  const std::string& serviceName) {
            getPortProperties(aResp, systemName, adapterId, portId, portPath,
                              serviceName);
        });
        });
}

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
        [aResp, systemName,
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

            std::string portId =
                sdbusplus::message::object_path(path).filename();
            nlohmann::json item;
            item["@odata.id"] = crow::utility::urlFromPieces(
                "redfish", "v1", "Systems", systemName, "FabricAdapters",
                adapterId, "Ports", portId);
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
        [aResp, systemName, adapterId](const std::string& fabricAdapterPath,
                                       const std::string&) {
        doPortCollectionGet(aResp, systemName, fabricAdapterPath, adapterId);
        });
}

/**
 * Systems derived class for delivering port Schema.
 */
inline void requestRoutesPort(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/FabricAdapters/<str>/Ports/<str>/")
        .privileges(redfish::privileges::headPort)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handlePortHead, std::ref(app)));

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/FabricAdapters/<str>/Ports/<str>/")
        .privileges(redfish::privileges::getPort)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePortGet, std::ref(app)));
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
