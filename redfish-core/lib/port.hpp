#pragma once

#include "app.hpp"
#include "fabric_adapters.hpp"
#include "human_sort.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"

#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>

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

/**
 * @brief Api to get Port properties.
 * @param[in,out]   aResp       Async HTTP response.
 * @param[in]       systemName  RSystem name Id.
 * @param[in]       adapterId   AdapterId
 * @param[in]       portId      Object path of the Port.
 * @param[in]       portPath    Object dbus path of the Port.
 */
inline void getPortProperties(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                              const std::string& systemName,
                              const std::string& adapterId,
                              const std::string& portId,
                              const std::string& /*portPath*/)
{
    aResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/Port/Port.json>; rel=describedby");

    aResp->res.jsonValue["@odata.type"] = "#Port.v1_7_0.Port";
    aResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/FabricAdapters/{}/Ports/{}",
                            systemName, adapterId, portId);
    aResp->res.jsonValue["Id"] = portId;
    aResp->res.jsonValue["Name"] = portId;
}

inline void getValidPortPath(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const std::string& adapterPath, const std::string& portId,
    std::function<void(const std::string& portPath)>&& callback)
{
    dbus::utility::getAssociationEndPoints(
        adapterPath + "/connecting",
        [aResp, portId,
         callback](const boost::system::error_code& ec,
                   const dbus::utility::MapperEndPoints& endpoints) {
        if (ec)
        {
            if (ec.value() == EBADR)
            {
                BMCWEB_LOG_DEBUG << "Port association not found";
                return;
            }
            BMCWEB_LOG_ERROR << "DBUS response error " << ec.message();
            messages::internalError(aResp->res);
            return;
        }

        for (const auto& portPath : endpoints)
        {
            if (checkPortId(portPath, portId))
            {
                callback(portPath);
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
                         [aResp](const std::string&) {
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
                          portId](const std::string& portPath) {
            getPortProperties(aResp, systemName, adapterId, portId, portPath);
        });
        });
}

inline void getPortCollection(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                              const std::string& systemName,
                              const std::string& fabricAdapterPath,
                              const std::string& adapterId)
{
    BMCWEB_LOG_DEBUG << "Get Port collection members for: " << adapterId;

    dbus::utility::getAssociationEndPoints(
        fabricAdapterPath + "/connecting",
        [aResp, systemName,
         adapterId](const boost::system::error_code& ec,
                    const dbus::utility::MapperEndPoints& endpoints) {
        if (ec)
        {
            if (ec.value() == EBADR)
            {
                BMCWEB_LOG_DEBUG << "Port association not found";
                return;
            }
            BMCWEB_LOG_ERROR << "DBUS response error " << ec.message();
            messages::internalError(aResp->res);
            return;
        }

        // Use the only leaf part of the portId names
        std::vector<std::string> portIdNames;
        for (const auto& path : endpoints)
        {
            std::string portId =
                sdbusplus::message::object_path(path).filename();
            portIdNames.emplace_back(std::move(portId));
        }

        std::sort(portIdNames.begin(), portIdNames.end(),
                  AlphanumLess<std::string>());

        nlohmann::json& members = aResp->res.jsonValue["Members"];
        members = nlohmann::json::array();
        for (const std::string& portId : portIdNames)
        {
            nlohmann::json item;
            item["@odata.id"] = boost::urls::format(
                "/redfish/v1/Systems/{}/FabricAdapters/{}/Ports/{}", systemName,
                adapterId, portId);
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
        boost::urls::format("/redfish/v1/Systems/{}/FabricAdapters/{}/Ports",
                            systemName, adapterId);

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
