#pragma once

#include "app.hpp"
#include "utils/chassis_utils.hpp"

#include <memory>
#include <optional>
#include <string>

namespace redfish
{
using MapperGetAssociationResponse =
    std::vector<std::tuple<std::string, std::string, std::string>>;

inline void getChassisAssoction(const std::string& service,
                                const std::string& objPath,
                                const std::string& chassisPath,
                                std::function<void()>&& callback)
{
    sdbusplus::asio::getProperty<MapperGetAssociationResponse>(
        *crow::connections::systemBus, service, objPath,
        "xyz.openbmc_project.Association.Definitions", "Associations",
        [callback,
         chassisPath](const boost::system::error_code ec,
                      const MapperGetAssociationResponse& associations) {
        if (ec)
        {
            return;
        }

        for (const auto& assoc : associations)
        {
            const auto& [rType, tType, endpoint] = assoc;
            if (rType == "chassis" && endpoint == chassisPath)
            {
                callback();
                return;
            }
        }
        });
}

inline void
    getSensorsAssoction(const std::string& service, const std::string& objPath,
                        std::function<void(const std::string& path)>&& callback)
{
    sdbusplus::asio::getProperty<MapperGetAssociationResponse>(
        *crow::connections::systemBus, service, objPath,
        "xyz.openbmc_project.Association.Definitions", "Associations",
        [callback](const boost::system::error_code ec,
                   const MapperGetAssociationResponse& associations) {
        if (ec)
        {
            callback("");
            return;
        }

        for (const auto& assoc : associations)
        {
            const auto& [rType, tType, endpoint] = assoc;
            if (rType == "sensors")
            {
                callback(endpoint);
            }
        }
        });
}

inline void
    updateFanList(const std::shared_ptr<bmcweb::AsyncResp>& /* asyncResp */,
                  const std::string& /* chassisId */,
                  const std::string& fanPath)
{
    std::string fanName = sdbusplus::message::object_path(fanPath).filename();
    if (fanName.empty())
    {
        return;
    }

    // TODO In order for the validator to pass, the Members property will be
    // implemented on the next commit
}

inline void
    fanCollection(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                  const std::string& chassisId,
                  const std::string& validChassisPath,
                  const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    asyncResp->res.jsonValue["@odata.type"] = "#FanCollection.FanCollection";
    asyncResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Chassis", chassisId, "ThermalSubsystem", "Fans");
    asyncResp->res.jsonValue["Name"] = "Fan Collection";
    asyncResp->res.jsonValue["Description"] =
        "The collection of Fan resource instances " + chassisId;
    asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
    asyncResp->res.jsonValue["Members@odata.count"] = 0;

    for (const auto& [fanPath, serviceMap] : subtree)
    {
        for (const auto& [service, interfaces] : serviceMap)
        {
            auto respHandler =
                [asyncResp, service{service}, fanPath{fanPath}, chassisId]() {
                getSensorsAssoction(service, fanPath,
                                    [asyncResp, fanPath, chassisId](
                                        const std::string& fanSensorPath) {
                    if (fanSensorPath.empty())
                    {
                        updateFanList(asyncResp, chassisId, fanPath);
                    }
                    else
                    {
                        updateFanList(asyncResp, chassisId, fanSensorPath);
                    }
                });
            };
            getChassisAssoction(service, fanPath, validChassisPath,
                                std::move(respHandler));
        }
    }
}

inline void doFanCollection(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& chassisId,
                            const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        BMCWEB_LOG_ERROR << "Not a valid chassis ID" << chassisId;
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp, chassisId, validChassisPath{*validChassisPath}](
            const boost::system::error_code ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "D-Bus response error on GetSubTree " << ec;
            if (ec.value() ==
                boost::system::linux_error::bad_request_descriptor)
            {
                messages::resourceNotFound(asyncResp->res, "Chassis",
                                           chassisId);
                return;
            }
            messages::internalError(asyncResp->res);
            return;
        }

        fanCollection(asyncResp, chassisId, validChassisPath, subtree);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project", 0,
        std::array<const char*, 1>{"xyz.openbmc_project.Inventory.Item.Fan"});
}

inline void
    handleFanCollectionHead(App& app, const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& /* chassisId */)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/FanCollection/FanCollection.json>; rel=describedby");
}

inline void
    handleFanCollectionGet(App& app, const crow::Request& req,
                           const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& chassisId)
{
    handleFanCollectionHead(app, req, asyncResp, chassisId);

    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        std::bind_front(doFanCollection, asyncResp, chassisId));
}

inline void requestRoutesFanCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/Fans/")
        .privileges(redfish::privileges::headFanCollection)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleFanCollectionHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/Fans/")
        .privileges(redfish::privileges::getFanCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFanCollectionGet, std::ref(app)));
}

} // namespace redfish
