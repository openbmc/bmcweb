#pragma once

#include <app.hpp>
#include <query.hpp>
#include <registries/privilege_registry.hpp>
#include <utils/chassis_utils.hpp>

namespace redfish
{

inline bool checkPowerSupplyId(const std::string& powerSupplyPath,
                               const std::string& powerSupplyId)
{
    std::string powerSupplyName =
        sdbusplus::message::object_path(powerSupplyPath).filename();

    return !(powerSupplyName.empty() || powerSupplyName != powerSupplyId);
}

template <typename Callback>
inline void
    getValidPowerSupplyPath(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& chassisPath,
                            const std::string& powerSupplyId,
                            Callback&& callback)
{
    auto respHandler =
        [callback{std::forward<Callback>(callback)}, asyncResp, powerSupplyId](
            const boost::system::error_code ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
        BMCWEB_LOG_DEBUG << "getValidPowerSupplyPath respHandler enter";

        if (ec)
        {
            BMCWEB_LOG_ERROR << "espHandler DBUS error: " << ec.message();
            messages::internalError(asyncResp->res);
            return;
        }

        for (const auto& [powerSupplyPath, serviceMap] : subtree)
        {
            if (serviceMap.empty())
            {
                continue;
            }

            for (const auto& [service, interfaces] : serviceMap)
            {
                if (checkPowerSupplyId(powerSupplyPath, powerSupplyId))
                {
                    callback(service, powerSupplyPath, interfaces);
                    return;
                }
            }
        }

        messages::resourceNotFound(asyncResp->res, "PowerSupply",
                                   powerSupplyId);
    };

    // Get the PowerSupply Collection
    crow::connections::systemBus->async_method_call(
        respHandler, "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", chassisPath, 0,
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
        [asyncResp,
         chassisId](const boost::system::error_code ec,
                    const dbus::utility::MapperGetSubTreePathsResponse&
                        powerSupplyPaths) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "D-Bus response error on GetSubTree " << ec;
            messages::internalError(asyncResp->res);
            return;
        }

        asyncResp->res.jsonValue["@odata.type"] =
            "#PowerSupplyCollection.PowerSupplyCollection";
        asyncResp->res.jsonValue["Name"] = "Power Supply Collection";
        asyncResp->res.jsonValue["@odata.id"] =
            crow::utility::urlFromPieces("redfish", "v1", "Chassis", chassisId,
                                         "PowerSubsystem", "PowerSupplies");
        asyncResp->res.jsonValue["Description"] =
            "The collection of PowerSupply resource instances " + chassisId;

        for (const auto& powerSupplyPath : powerSupplyPaths)
        {
            updatePowerSupplyList(asyncResp, chassisId, powerSupplyPath);
        }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        *validChassisPath, 0,
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
    getPowerSupplyState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& service, const std::string& path,
                        const std::string& intf)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, service, path, intf, "Present",
        [asyncResp](const boost::system::error_code ec, const bool value) {
        if (ec)
        {
            return;
        }

        asyncResp->res.jsonValue["Status"]["State"] =
            value ? "Enabled" : "Absent";
        });
}

inline void
    getPowerSupplyHealth(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& service, const std::string& path,
                         const std::string& intf)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, service, path, intf, "Functional",
        [asyncResp](const boost::system::error_code ec, const bool value) {
        if (ec)
        {
            return;
        }

        asyncResp->res.jsonValue["Status"]["Health"] =
            value ? "OK" : "Critical";
        });
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

    auto getPowerSupplyIdFunc =
        [asyncResp, chassisId, powerSupplyId](
            const std::string& service, const std::string& powerSupplyPath,
            const std::vector<std::string>& interfaces) {
        asyncResp->res.jsonValue["@odata.type"] =
            "#PowerSupply.v1_5_0.PowerSupply";
        asyncResp->res.jsonValue["Name"] = powerSupplyId;
        asyncResp->res.jsonValue["Id"] = powerSupplyId;
        asyncResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
            "redfish", "v1", "Chassis", chassisId, "PowerSubsystem",
            "PowerSupplies", powerSupplyId);

        for (const auto& intf : interfaces)
        {
            if (intf == "xyz.openbmc_project.Inventory.Item")
            {
                getPowerSupplyState(asyncResp, service, powerSupplyPath, intf);
            }
            if (intf == "xyz.openbmc_project.State.Decorator.OperationalStatus")
            {
                getPowerSupplyHealth(asyncResp, service, powerSupplyPath, intf);
            }
        }
    };
    // Get the correct Path and Service that match the input parameters
    getValidPowerSupplyPath(asyncResp, *validChassisPath, powerSupplyId,
                            std::move(getPowerSupplyIdFunc));
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
