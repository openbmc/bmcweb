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
inline void
    getValidPowerSupplyPath(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& validChassisPath,
                            const std::string& powerSupplyId,
                            Callback&& callback)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, powerSupplyId, validChassisPath,
         callback](const boost::system::error_code ec,
                   const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "D-Bus response error on GetSubTree " << ec;
            messages::internalError(asyncResp->res);
            return;
        }

        messages::resourceNotFound(asyncResp->res, "PowerSupply",
                                   powerSupplyId);
        for (const auto& [powerSupplyPath, serviceMap] : subtree)
        {
            for (const auto& [service, interfaces] : serviceMap)
            {
                auto respHandler = [asyncResp, service, powerSupplyPath,
                                    powerSupplyId, interfaces, callback]() {
                    if (checkPowerSupplyId(powerSupplyPath, powerSupplyId))
                    {
                        asyncResp->res.clear();
                        callback(service, powerSupplyPath, interfaces);
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

        for (const auto& [powerSupplyPath, serviceMap] : subtree)
        {
            for (const auto& [service, interfaces] : serviceMap)
            {
                auto respHandler =
                    [asyncResp, service, powerSupplyPath, chassisId]() {
                    updatePowerSupplyList(asyncResp, chassisId,
                                          powerSupplyPath);
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
    getPowerSupplyAsset(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& service, const std::string& path,
                        const std::string& intf)
{

    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, path, intf,
        [asyncResp](const boost::system::error_code ec,
                    const dbus::utility::DBusPropertiesMap& propertiesList) {
        if (ec)
        {
            return;
        }

        const std::string* partNumber = nullptr;
        const std::string* serialNumber = nullptr;
        const std::string* manufacturer = nullptr;
        const std::string* model = nullptr;
        const std::string* sparePartNumber = nullptr;

        const bool success = sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), propertiesList, "PartNumber",
            partNumber, "SerialNumber", serialNumber, "Manufacturer",
            manufacturer, "Model", model, "SparePartNumber", sparePartNumber);

        if (!success)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        if (partNumber != nullptr)
        {
            asyncResp->res.jsonValue["PartNumber"] = *partNumber;
        }

        if (serialNumber != nullptr)
        {
            asyncResp->res.jsonValue["SerialNumber"] = *serialNumber;
        }

        if (manufacturer != nullptr)
        {
            asyncResp->res.jsonValue["Manufacturer"] = *manufacturer;
        }

        if (model != nullptr)
        {
            asyncResp->res.jsonValue["Model"] = *model;
        }

        // SparePartNumber is optional on D-Bus so skip if it is empty
        if (sparePartNumber != nullptr && !sparePartNumber->empty())
        {
            asyncResp->res.jsonValue["SparePartNumber"] = *sparePartNumber;
        }
        });
}

inline void getPowerSupplyFirmwareVersion(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& service, const std::string& path,
    const std::string& intf)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, service, path, intf, "Version",
        [asyncResp](const boost::system::error_code ec,
                    const std::string& value) {
        if (ec)
        {
            return;
        }

        asyncResp->res.jsonValue["FirmwareVersion"] = value;
        });
}

inline void
    getPowerSupplyLocation(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& service, const std::string& path,
                           const std::string& intf)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, service, path, intf, "LocationCode",
        [asyncResp](const boost::system::error_code ec,
                    const std::string& value) {
        if (ec)
        {
            return;
        }

        asyncResp->res.jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
            value;
        });
}

inline void
    getEfficiencyPercent(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    static const std::string efficiencyIntf =
        "xyz.openbmc_project.Control.PowerSupplyAttributes";
    // Gets the Power Supply Attributes such as EfficiencyPercent.
    // Currently we only support one power supply EfficiencyPercent, use this
    // for all the power supplies.
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            if (ec.value() == EBADR)
            {
                return;
            }
            BMCWEB_LOG_ERROR << "respHandler DBus error " << ec.message();
            messages::internalError(asyncResp->res);
            return;
        }

        for (const auto& [path, serviceMap] : subtree)
        {
            if (serviceMap.empty())
            {
                continue;
            }

            for (const auto& [service, interfaces] : serviceMap)
            {
                sdbusplus::asio::getProperty<uint32_t>(
                    *crow::connections::systemBus, service, path,
                    efficiencyIntf, "DeratingFactor",
                    [asyncResp](const boost::system::error_code ec1,
                                const uint32_t value) {
                    if (ec1)
                    {
                        return;
                    }

                    nlohmann::json item;
                    item["EfficiencyPercent"] = value;
                    nlohmann::json& efficiencyList =
                        asyncResp->res.jsonValue["EfficiencyRatings"];
                    efficiencyList.emplace_back(std::move(item));
                    });
            }
        }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project", 0,
        std::array<const char*, 1>{efficiencyIntf.c_str()});
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
            if (intf == "xyz.openbmc_project.Inventory.Decorator.Asset")
            {
                getPowerSupplyAsset(asyncResp, service, powerSupplyPath, intf);
            }
            if (intf == "xyz.openbmc_project.Software.Version")
            {
                getPowerSupplyFirmwareVersion(asyncResp, service,
                                              powerSupplyPath, intf);
            }
            if (intf == "xyz.openbmc_project.Inventory.Decorator.LocationCode")
            {
                getPowerSupplyLocation(asyncResp, service, powerSupplyPath,
                                       intf);
            }
        }

        getEfficiencyPercent(asyncResp);
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
