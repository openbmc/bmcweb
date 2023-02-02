#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "led.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"

#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace redfish
{

static constexpr std::array<std::string_view, 1> powerSupplyInterface = {
    "xyz.openbmc_project.Inventory.Item.PowerSupply"};

inline void updatePowerSupplyList(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId,
    const dbus::utility::MapperGetSubTreePathsResponse& powerSupplyPaths)
{
    nlohmann::json& powerSupplyList = asyncResp->res.jsonValue["Members"];
    for (const std::string& powerSupplyPath : powerSupplyPaths)
    {
        std::string powerSupplyName =
            sdbusplus::message::object_path(powerSupplyPath).filename();
        if (powerSupplyName.empty())
        {
            continue;
        }

        nlohmann::json item = nlohmann::json::object();
        item["@odata.id"] = boost::urls::format(
            "/redfish/v1/Chassis/{}/PowerSubsystem/PowerSupplies/{}", chassisId,
            powerSupplyName);

        powerSupplyList.emplace_back(std::move(item));
    }
    asyncResp->res.jsonValue["Members@odata.count"] = powerSupplyList.size();
}

inline void
    doPowerSupplyCollection(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& chassisId,
                            const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/PowerSupplyCollection/PowerSupplyCollection.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] =
        "#PowerSupplyCollection.PowerSupplyCollection";
    asyncResp->res.jsonValue["Name"] = "Power Supply Collection";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/PowerSubsystem/PowerSupplies", chassisId);
    asyncResp->res.jsonValue["Description"] =
        "The collection of PowerSupply resource instances.";
    asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
    asyncResp->res.jsonValue["Members@odata.count"] = 0;

    std::string powerPath = *validChassisPath + "/powered_by";
    dbus::utility::getAssociatedSubTreePaths(
        powerPath,
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        powerSupplyInterface,
        [asyncResp, chassisId](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& subtreePaths) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR("DBUS response error{}", ec.value());
                messages::internalError(asyncResp->res);
            }
            return;
        }

        updatePowerSupplyList(asyncResp, chassisId, subtreePaths);
    });
}

inline void handlePowerSupplyCollectionHead(
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
        [asyncResp,
         chassisId](const std::optional<std::string>& validChassisPath) {
        if (!validChassisPath)
        {
            messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
            return;
        }
        asyncResp->res.addHeader(
            boost::beast::http::field::link,
            "</redfish/v1/JsonSchemas/PowerSupplyCollection/PowerSupplyCollection.json>; rel=describedby");
    });
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
        .privileges(redfish::privileges::headPowerSupplyCollection)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handlePowerSupplyCollectionHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/PowerSubsystem/PowerSupplies/")
        .privileges(redfish::privileges::getPowerSupplyCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePowerSupplyCollectionGet, std::ref(app)));
}

inline bool checkPowerSupplyId(const std::string& powerSupplyPath,
                               const std::string& powerSupplyId)
{
    std::string powerSupplyName =
        sdbusplus::message::object_path(powerSupplyPath).filename();

    return !(powerSupplyName.empty() || powerSupplyName != powerSupplyId);
}

inline void getValidPowerSupplyPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& validChassisPath, const std::string& powerSupplyId,
    std::function<void(const std::string& powerSupplyPath)>&& callback)
{
    std::string powerPath = validChassisPath + "/powered_by";
    dbus::utility::getAssociatedSubTreePaths(
        powerPath,
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        powerSupplyInterface,
        [asyncResp, powerSupplyId, callback{std::move(callback)}](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& subtreePaths) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR(
                    "DBUS response error for getAssociatedSubTreePaths{}",
                    ec.value());
                messages::internalError(asyncResp->res);
                return;
            }
            messages::resourceNotFound(asyncResp->res, "PowerSupplies",
                                       powerSupplyId);
            return;
        }

        for (const std::string& path : subtreePaths)
        {
            if (checkPowerSupplyId(path, powerSupplyId))
            {
                callback(path);
                return;
            }
        }

        if (!subtreePaths.empty())
        {
            BMCWEB_LOG_WARNING("Power supply not found: {}", powerSupplyId);
            messages::resourceNotFound(asyncResp->res, "PowerSupplies",
                                       powerSupplyId);
            return;
        }
    });
}

inline void
    getPowerSupplyState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& service, const std::string& path,
                        bool available)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, service, path,
        "xyz.openbmc_project.Inventory.Item", "Present",
        [asyncResp, available](const boost::system::error_code& ec,
                               const bool value) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR("DBUS response error for State {}",
                                 ec.value());
                messages::internalError(asyncResp->res);
            }
            return;
        }

        if (!value)
        {
            asyncResp->res.jsonValue["Status"]["State"] = "Absent";
        }
        else if (!available)
        {
            asyncResp->res.jsonValue["Status"]["State"] = "UnavailableOffline";
        }
    });
}

inline void
    getPowerSupplyHealth(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& service, const std::string& path,
                         bool available)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, service, path,
        "xyz.openbmc_project.State.Decorator.OperationalStatus", "Functional",
        [asyncResp, available](const boost::system::error_code& ec,
                               const bool value) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR("DBUS response error for Health {}",
                                 ec.value());
                messages::internalError(asyncResp->res);
            }
            return;
        }

        if (!value || !available)
        {
            asyncResp->res.jsonValue["Status"]["Health"] = "Critical";
        }
    });
}

inline void getPowerSupplyStateAndHealth(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& service, const std::string& path)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, service, path,
        "xyz.openbmc_project.State.Decorator.Availability", "Available",
        [asyncResp, service, path](const boost::system::error_code& ec,
                                   const bool available) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR("DBUS response error for Available {}",
                                 ec.value());
                messages::internalError(asyncResp->res);
            }
            return;
        }

        getPowerSupplyState(asyncResp, service, path, available);

        getPowerSupplyHealth(asyncResp, service, path, available);
    });
}

inline void
    getPowerSupplyAsset(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& service, const std::string& path)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, path,
        "xyz.openbmc_project.Inventory.Decorator.Asset",
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::DBusPropertiesMap& propertiesList) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR("DBUS response error for Asset {}",
                                 ec.value());
                messages::internalError(asyncResp->res);
            }
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
    const std::string& service, const std::string& path)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, service, path,
        "xyz.openbmc_project.Software.Version", "Version",
        [asyncResp](const boost::system::error_code& ec,
                    const std::string& value) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR("DBUS response error for FirmwareVersion {}",
                                 ec.value());
                messages::internalError(asyncResp->res);
            }
            return;
        }
        asyncResp->res.jsonValue["FirmwareVersion"] = value;
    });
}

inline void
    getPowerSupplyLocation(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& service, const std::string& path)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, service, path,
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode",
        [asyncResp](const boost::system::error_code& ec,
                    const std::string& value) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR("DBUS response error for Location {}",
                                 ec.value());
                messages::internalError(asyncResp->res);
            }
            return;
        }
        asyncResp->res.jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
            value;
    });
}

inline void handleGetEfficiencyResponse(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, uint32_t value)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error for DeratingFactor {}",
                             ec.value());
            messages::internalError(asyncResp->res);
        }
        return;
    }
    // The PDI default value is 0, if it hasn't been set leave off
    if (value == 0)
    {
        return;
    }

    nlohmann::json::array_t efficiencyRatings;
    nlohmann::json::object_t efficiencyPercent;
    efficiencyPercent["EfficiencyPercent"] = value;
    efficiencyRatings.emplace_back(std::move(efficiencyPercent));
    asyncResp->res.jsonValue["EfficiencyRatings"] =
        std::move(efficiencyRatings);
}

inline void handlePowerSupplyAttributesSubTreeResponse(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error for EfficiencyPercent {}",
                             ec.value());
            messages::internalError(asyncResp->res);
        }
        return;
    }

    if (subtree.empty())
    {
        BMCWEB_LOG_DEBUG("Can't find Power Supply Attributes!");
        return;
    }

    if (subtree.size() != 1)
    {
        BMCWEB_LOG_ERROR(
            "Unexpected number of paths returned by getSubTree: {}",
            subtree.size());
        messages::internalError(asyncResp->res);
        return;
    }

    const auto& [path, serviceMap] = *subtree.begin();
    const auto& [service, interfaces] = *serviceMap.begin();
    sdbusplus::asio::getProperty<uint32_t>(
        *crow::connections::systemBus, service, path,
        "xyz.openbmc_project.Control.PowerSupplyAttributes", "DeratingFactor",
        [asyncResp](const boost::system::error_code& ec1, uint32_t value) {
        handleGetEfficiencyResponse(asyncResp, ec1, value);
    });
}

inline void
    getEfficiencyPercent(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    constexpr std::array<std::string_view, 1> efficiencyIntf = {
        "xyz.openbmc_project.Control.PowerSupplyAttributes"};

    dbus::utility::getSubTree(
        "/xyz/openbmc_project", 0, efficiencyIntf,
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
        handlePowerSupplyAttributesSubTreeResponse(asyncResp, ec, subtree);
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
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    // Get the correct Path and Service that match the input parameters
    getValidPowerSupplyPath(asyncResp, *validChassisPath, powerSupplyId,
                            [asyncResp, chassisId, powerSupplyId](
                                const std::string& powerSupplyPath) {
        asyncResp->res.addHeader(
            boost::beast::http::field::link,
            "</redfish/v1/JsonSchemas/PowerSupply/PowerSupply.json>; rel=describedby");
        asyncResp->res.jsonValue["@odata.type"] =
            "#PowerSupply.v1_5_0.PowerSupply";
        asyncResp->res.jsonValue["Name"] = "Power Supply";
        asyncResp->res.jsonValue["Id"] = powerSupplyId;
        asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
            "/redfish/v1/Chassis/{}/PowerSubsystem/PowerSupplies/{}", chassisId,
            powerSupplyId);

        asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
        asyncResp->res.jsonValue["Status"]["Health"] = "OK";

        dbus::utility::getDbusObject(
            powerSupplyPath, powerSupplyInterface,
            [asyncResp,
             powerSupplyPath](const boost::system::error_code& ec,
                              const dbus::utility::MapperGetObject& object) {
            if (ec || object.empty())
            {
                messages::internalError(asyncResp->res);
                return;
            }

            getPowerSupplyStateAndHealth(asyncResp, object.begin()->first,
                                         powerSupplyPath);
            getPowerSupplyAsset(asyncResp, object.begin()->first,
                                powerSupplyPath);
            getPowerSupplyFirmwareVersion(asyncResp, object.begin()->first,
                                          powerSupplyPath);
            getPowerSupplyLocation(asyncResp, object.begin()->first,
                                   powerSupplyPath);
        });

        getEfficiencyPercent(asyncResp);
        getLocationIndicatorActive(asyncResp, powerSupplyPath);
    });
}

inline void
    handlePowerSupplyHead(App& app, const crow::Request& req,
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
        [asyncResp, chassisId,
         powerSupplyId](const std::optional<std::string>& validChassisPath) {
        if (!validChassisPath)
        {
            messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
            return;
        }

        // Get the correct Path and Service that match the input parameters
        getValidPowerSupplyPath(asyncResp, *validChassisPath, powerSupplyId,
                                [asyncResp](const std::string&) {
            asyncResp->res.addHeader(
                boost::beast::http::field::link,
                "</redfish/v1/JsonSchemas/PowerSupply/PowerSupply.json>; rel=describedby");
        });
    });
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

inline void
    doPatchPowerSupply(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const bool locationIndicatorActive,
                       const std::string& powerSupplyPath)
{
    setLocationIndicatorActive(asyncResp, powerSupplyPath,
                               locationIndicatorActive);
}

inline void powerSupplyPatchAfterValidateChassis(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& powerSupplyId,
    const bool locationIndicatorActive,
    const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        BMCWEB_LOG_WARNING("Not a valid chassis ID: {}", chassisId);
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    // Get the correct power supply Path that match the input parameters
    getValidPowerSupplyPath(asyncResp, *validChassisPath, powerSupplyId,
                            std::bind_front(doPatchPowerSupply, asyncResp,
                                            locationIndicatorActive));
}

inline void
    handlePowerSupplyPatch(App& app, const crow::Request& req,
                           const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& chassisId,
                           const std::string& powerSupplyId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    std::optional<bool> locationIndicatorActive;
    if (!json_util::readJsonPatch(req, asyncResp->res,
                                  "LocationIndicatorActive",
                                  locationIndicatorActive))
    {
        return;
    }

    if (locationIndicatorActive)
    {
        redfish::chassis_utils::getValidChassisPath(
            asyncResp, chassisId,
            std::bind_front(powerSupplyPatchAfterValidateChassis, asyncResp,
                            chassisId, powerSupplyId,
                            *locationIndicatorActive));
    }
}

inline void requestRoutesPowerSupply(App& app)
{
    BMCWEB_ROUTE(
        app, "/redfish/v1/Chassis/<str>/PowerSubsystem/PowerSupplies/<str>/")
        .privileges(redfish::privileges::headPowerSupply)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handlePowerSupplyHead, std::ref(app)));

    BMCWEB_ROUTE(
        app, "/redfish/v1/Chassis/<str>/PowerSubsystem/PowerSupplies/<str>/")
        .privileges(redfish::privileges::getPowerSupply)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePowerSupplyGet, std::ref(app)));

    BMCWEB_ROUTE(
        app, "/redfish/v1/Chassis/<str>/PowerSubsystem/PowerSupplies/<str>/")
        .privileges(redfish::privileges::patchPowerSupply)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handlePowerSupplyPatch, std::ref(app)));
}

} // namespace redfish
