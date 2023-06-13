#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"

#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>

#include <memory>
#include <optional>
#include <string>

namespace redfish
{

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

    nlohmann::json item = nlohmann::json::object();
    item["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/PowerSubsystem/PowerSupplies/{}", chassisId,
        powerSupplyName);

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
    dbus::utility::getAssociationEndPoints(
        powerPath, [asyncResp, chassisId](
                       const boost::system::error_code& ec,
                       const dbus::utility::MapperEndPoints& endpoints) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR << "DBUS response error" << ec.value();
                    messages::internalError(asyncResp->res);
                }
                return;
            }

            for (const auto& endpoint : endpoints)
            {
                updatePowerSupplyList(asyncResp, chassisId, endpoint);
            }
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
    dbus::utility::getAssociationEndPoints(
        powerPath, [asyncResp, powerSupplyId, callback{std::move(callback)}](
                       const boost::system::error_code& ec,
                       const dbus::utility::MapperEndPoints& endpoints) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR
                        << "DBUS response error for getAssociationEndPoints"
                        << ec.value();
                    messages::internalError(asyncResp->res);
                    return;
                }
                messages::resourceNotFound(asyncResp->res, "PowerSupplies",
                                           powerSupplyId);
                return;
            }

            for (const auto& endpoint : endpoints)
            {
                if (checkPowerSupplyId(endpoint, powerSupplyId))
                {
                    callback(endpoint);
                    return;
                }
            }

            if (!endpoints.empty())
            {
                BMCWEB_LOG_WARNING << "Power supply not found: "
                                   << powerSupplyId;
                messages::resourceNotFound(asyncResp->res, "PowerSupplies",
                                           powerSupplyId);
                return;
            }
        });
}

inline void
    getPowerSupplyState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& service, const std::string& path)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, service, path,
        "xyz.openbmc_project.Inventory.Item", "Present",
        [asyncResp](const boost::system::error_code& ec, const bool value) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR << "DBUS response error for State "
                                 << ec.value();
                messages::internalError(asyncResp->res);
            }
            return;
        }

        if (!value)
        {
            asyncResp->res.jsonValue["Status"]["State"] = "Absent";
        }
        });
}

inline void
    getPowerSupplyHealth(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& service, const std::string& path)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, service, path,
        "xyz.openbmc_project.State.Decorator.OperationalStatus", "Functional",
        [asyncResp](const boost::system::error_code& ec, const bool value) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR << "DBUS response error for Health "
                                 << ec.value();
                messages::internalError(asyncResp->res);
            }
            return;
        }

        if (!value)
        {
            asyncResp->res.jsonValue["Status"]["Health"] = "Critical";
        }
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
                BMCWEB_LOG_ERROR << "DBUS response error for Asset "
                                 << ec.value();
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
                BMCWEB_LOG_ERROR << "DBUS response error for FirmwareVersion "
                                 << ec.value();
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
                BMCWEB_LOG_ERROR << "DBUS response error for Location "
                                 << ec.value();
                messages::internalError(asyncResp->res);
            }
            return;
        }
        asyncResp->res.jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
            value;
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

        constexpr std::array<std::string_view, 1> interfaces = {
            "xyz.openbmc_project.Inventory.Item.PowerSupply"};
        dbus::utility::getDbusObject(
            powerSupplyPath, interfaces,
            [asyncResp,
             powerSupplyPath](const boost::system::error_code& ec,
                              const dbus::utility::MapperGetObject& object) {
            if (ec || object.empty())
            {
                messages::internalError(asyncResp->res);
                return;
            }

            getPowerSupplyState(asyncResp, object.begin()->first,
                                powerSupplyPath);
            getPowerSupplyHealth(asyncResp, object.begin()->first,
                                 powerSupplyPath);
            getPowerSupplyAsset(asyncResp, object.begin()->first,
                                powerSupplyPath);
            getPowerSupplyFirmwareVersion(asyncResp, object.begin()->first,
                                          powerSupplyPath);
            getPowerSupplyLocation(asyncResp, object.begin()->first,
                                   powerSupplyPath);
            });
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
}

} // namespace redfish
