#pragma once

#include <app.hpp>
#include <query.hpp>
#include <registries/privilege_registry.hpp>
#include <utils/chassis_utils.hpp>

namespace redfish
{
using MapperGetAssociationResponse =
    std::vector<std::tuple<std::string, std::string, std::string>>;

template <typename Callback>
inline void
    getChassisAssoction(const std::string& service, const std::string& objPath,
                        const std::string& chassisPath, Callback&& callback)
{
    const static std::string assIntf =
        "xyz.openbmc_project.Association.Definitions";
    sdbusplus::asio::getProperty<MapperGetAssociationResponse>(
        *crow::connections::systemBus, service, objPath, assIntf,
        "Associations",
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

inline bool checkPowerSupplyId(const std::string& powerSupplyPath,
                               const std::string& powerSupplyId)
{
    std::string powerSupplyName =
        sdbusplus::message::object_path(powerSupplyPath).filename();

    return !(powerSupplyName.empty() || powerSupplyName != powerSupplyId);
}

template <typename Callback>
inline void getValidPowerSupplyPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& validChassisPath, const std::string& chassisId,
    const std::string& powerSupplyId, Callback&& callback)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, validChassisPath, chassisId, powerSupplyId,
         callback](const boost::system::error_code ec,
                   const dbus::utility::MapperGetSubTreeResponse& subtree) {
        messages::resourceNotFound(asyncResp->res, "PowerSupply",
                                   powerSupplyId);
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

        for (const auto& [powerSupplyPath, serviceMap] : subtree)
        {
            for (const auto& [service, interfaces] : serviceMap)
            {
                auto respHandler = [asyncResp, powerSupplyPath{powerSupplyPath},
                                    powerSupplyId, callback]() {
                    if (checkPowerSupplyId(powerSupplyPath, powerSupplyId))
                    {
                        asyncResp->res.clear();
                        callback();
                        return;
                    }
                };
                getChassisAssoction(service, powerSupplyPath, validChassisPath,
                                    std::move(respHandler));
            }
        }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project", 0,
        std::array<const char*, 1>{
            "xyz.openbmc_project.Inventory.Item.PowerSupply"});
}

inline void
    updatePowerSupplyList(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& chassisId,
                          const std::string& powerSupplyPath)
{
    std::string powerSupplyName =
        sdbusplus::message::object_path(powerSupplyPath).filename();
    if (powerSupplyName.empty())
    {
        return;
    }

    nlohmann::json item;
    item["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Chassis", chassisId, "PowerSubsystem",
        "PowerSupplies", powerSupplyName);

    nlohmann::json& powerSupplyList = asyncResp->res.jsonValue["Members"];
    powerSupplyList.emplace_back(std::move(item));
    asyncResp->res.jsonValue["Members@odata.count"] = powerSupplyList.size();
}

inline void powerSupplyCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& validChassisPath,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    asyncResp->res.jsonValue["@odata.type"] =
        "#PowerSupplyCollection.PowerSupplyCollection";
    asyncResp->res.jsonValue["Name"] = "Power Supply Collection";
    asyncResp->res.jsonValue["@odata.id"] =
        crow::utility::urlFromPieces("redfish", "v1", "Chassis", chassisId,
                                     "PowerSubsystem", "PowerSupplies");
    asyncResp->res.jsonValue["Description"] =
        "The collection of PowerSupply resource instances.";
    asyncResp->res.jsonValue["Members@odata.count"] = 0;

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/PowerSupplyCollection/PowerSupplyCollection.json>; rel=describedby");

    for (const auto& [powerSupplyPath, serviceMap] : subtree)
    {
        for (const auto& [service, interfaces] : serviceMap)
        {
            auto respHandler =
                [asyncResp, powerSupplyPath{powerSupplyPath}, chassisId]() {
                updatePowerSupplyList(asyncResp, chassisId, powerSupplyPath);
            };
            getChassisAssoction(service, powerSupplyPath, validChassisPath,
                                std::move(respHandler));
        }
    }
}

inline void
    doPowerSupplyCollection(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
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

        powerSupplyCollection(asyncResp, chassisId, validChassisPath, subtree);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project", 0,
        std::array<const char*, 1>{
            "xyz.openbmc_project.Inventory.Item.PowerSupply"});
}

inline void handlePowerSupplyCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        std::bind_front(doPowerSupplyCollection, asyncResp, chassisId));
}

inline void requestRoutesPowerSupplyCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/PowerSubsystem/PowerSupplies/")
        .privileges(redfish::privileges::getPowerSupplyCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePowerSupplyCollectionGet, std::ref(app)));
}

inline void
    doPowerSupplyGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& chassisId,
                     const std::string& powerSupplyId,
                     const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        BMCWEB_LOG_ERROR << "Not a valid chassis ID" << chassisId;
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    auto getPowerSupplyIdFunc = [asyncResp, chassisId, powerSupplyId]() {
        asyncResp->res.jsonValue["@odata.type"] =
            "#PowerSupply.v1_5_0.PowerSupply";
        asyncResp->res.jsonValue["Name"] = powerSupplyId;
        asyncResp->res.jsonValue["Id"] = powerSupplyId;
        asyncResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
            "redfish", "v1", "Chassis", chassisId, "PowerSubsystem",
            "PowerSupplies", powerSupplyId);

        asyncResp->res.addHeader(
            boost::beast::http::field::link,
            "</redfish/v1/JsonSchemas/PowerSupply/PowerSupply.json>; rel=describedby");
    };
    // Get the correct Path and Service that match the input parameters
    getValidPowerSupplyPath(asyncResp, *validChassisPath, chassisId,
                            powerSupplyId, std::move(getPowerSupplyIdFunc));
}

inline void
    handlePowerSupplyGet(App& app, const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& chassisId,
                         const std::string& powerSupplyId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        std::bind_front(doPowerSupplyGet, asyncResp, chassisId, powerSupplyId));
}

inline void requestRoutesPowerSupply(App& app)
{
    BMCWEB_ROUTE(
        app, "/redfish/v1/Chassis/<str>/PowerSubsystem/PowerSupplies/<str>/")
        .privileges(redfish::privileges::getPowerSupply)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePowerSupplyGet, std::ref(app)));
}

} // namespace redfish
