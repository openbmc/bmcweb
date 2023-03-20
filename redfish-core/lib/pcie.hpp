/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/pcie_util.hpp"

#include <boost/system/linux_error.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

namespace redfish
{

static constexpr char const* inventoryPath = "/xyz/openbmc_project/inventory";
static constexpr std::array<std::string_view, 1> pcieDeviceInterface = {
    "xyz.openbmc_project.Inventory.Item.PCIeDevice"};
static constexpr std::array<std::string_view, 1> pcieSlotInterface = {
    "xyz.openbmc_project.Inventory.Item.PCIeSlot"};

static inline void handlePCIeDevicePath(
    const std::string& pcieDeviceId,
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const dbus::utility::MapperGetSubTreePathsResponse& pcieDevicePaths,
    const std::function<void(const std::string& pcieDevicePath,
                             const std::string& service)>& callback)

{
    for (const std::string& pcieDevicePath : pcieDevicePaths)
    {
        std::string pciecDeviceName =
            sdbusplus::message::object_path(pcieDevicePath).filename();
        if (pciecDeviceName.empty() || pciecDeviceName != pcieDeviceId)
        {
            continue;
        }

        dbus::utility::getDbusObject(
            pcieDevicePath, {},
            [pcieDevicePath, aResp,
             callback](const boost::system::error_code& ec,
                       const dbus::utility::MapperGetObject& object) {
            if (ec || object.empty())
            {
                BMCWEB_LOG_ERROR << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }
            callback(pcieDevicePath, object.begin()->first);
            });
        return;
    }

    BMCWEB_LOG_WARNING << "PCIe Device not found";
    messages::resourceNotFound(aResp->res, "PCIeDevice", pcieDeviceId);
}

static inline void getValidPCIeDevicePath(
    const std::string& pcieDeviceId,
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const std::function<void(const std::string& pcieDevicePath,
                             const std::string& service)>& callback)
{
    dbus::utility::getSubTreePaths(
        inventoryPath, 0, pcieDeviceInterface,
        [pcieDeviceId, aResp,
         callback](const boost::system::error_code& ec,
                   const dbus::utility::MapperGetSubTreePathsResponse&
                       pcieDevicePaths) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "D-Bus response error on GetSubTree " << ec;
            messages::internalError(aResp->res);
            return;
        }
        handlePCIeDevicePath(pcieDeviceId, aResp, pcieDevicePaths, callback);
        return;
        });
}

static inline void handlePCIeDeviceCollectionGet(
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

    aResp->res.addHeader(boost::beast::http::field::link,
                         "</redfish/v1/JsonSchemas/PCIeDeviceCollection/"
                         "PCIeDeviceCollection.json>; rel=describedby");
    aResp->res.jsonValue["@odata.type"] =
        "#PCIeDeviceCollection.PCIeDeviceCollection";
    aResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Systems/system/PCIeDevices";
    aResp->res.jsonValue["Name"] = "PCIe Device Collection";
    aResp->res.jsonValue["Description"] = "Collection of PCIe Devices";
    aResp->res.jsonValue["Members"] = nlohmann::json::array();
    aResp->res.jsonValue["Members@odata.count"] = 0;

    pcie_util::getPCIeDeviceList(aResp, "Members");
}

inline void requestRoutesSystemPCIeDeviceCollection(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/PCIeDevices/")
        .privileges(redfish::privileges::getPCIeDeviceCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePCIeDeviceCollectionGet, std::ref(app)));
}

inline void addPCIeSlotProperties(
    crow::Response& resp,
    const dbus::utility::DBusPropertiesMap& pcieSlotProperties)
{
    const std::string* generation = nullptr;
    const size_t* lanes = nullptr;
    const std::string* slotType = nullptr;
    const size_t* busId = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), pcieSlotProperties, "Generation",
        generation, "Lanes", lanes, "SlotType", slotType, "BusId", busId);

    if (!success)
    {
        messages::internalError(resp);
        return;
    }

    if (generation != nullptr)
    {
        std::optional<pcie_device::PCIeTypes> pcieType =
            redfish::pcie_util::redfishPcieGenerationFromDbus(*generation);
        if (!pcieType)
        {
            BMCWEB_LOG_ERROR << "Invalid PCIeType";
            messages::internalError(resp);
            return;
        }
        if (*pcieType != pcie_device::PCIeTypes::Invalid)
        {
            resp.jsonValue["Slot"]["PCIeType"] = *pcieType;
        }
    }

    if (lanes != nullptr)
    {

        resp.jsonValue["Slot"]["Lanes"] = *lanes;
    }

    if (slotType != nullptr)
    {
        std::optional<pcie_slots::SlotTypes> redfishSlotType =
            redfish::pcie_util::dbusSlotTypeToRf(*slotType);
        if (!redfishSlotType)
        {
            BMCWEB_LOG_ERROR << "Invalid PCIeSlot type";
            messages::internalError(resp);
            return;
        }
        if (*redfishSlotType != pcie_slots::SlotTypes::Invalid)
        {
            resp.jsonValue["Slot"]["SlotType"] = *redfishSlotType;
        }
    }
}

inline void getPCIeDeviceSlotPath(
    const std::string& pcieDevicePath,
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    std::function<void(const std::string& pcieDeviceSlot)>&& callback)
{
    std::string associationPath = pcieDevicePath + "/contained_by";
    dbus::utility::getAssociatedSubTreePaths(
        associationPath, sdbusplus::message::object_path(inventoryPath), 0,
        pcieSlotInterface,
        [callback, aResp,
         pcieDevicePath](const boost::system::error_code& ec,
                         const dbus::utility::MapperEndPoints& endpoints) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR
                    << "DBUS response error for getAssociatedSubTreePaths"
                    << ec.message();
                messages::internalError(aResp->res);
                return;
            }
            return;
        }

        if (endpoints.size() == 1)
        {
            callback(endpoints[0]);
        }
        else
        {
            // If the device doesn't have an association, return without PCIe
            // Slot properties
            BMCWEB_LOG_DEBUG << "PCIeDevice is not associated with PCIeSlot";
        }
        });
}

inline void getPCIeSlotLocation(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                                const std::string& pcieDevicePath,
                                const std::string& service)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, service, pcieDevicePath,
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode",
        [aResp](const boost::system::error_code& ec,
                const std::string& property) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR << "DBUS response error for Location"
                                 << ec.value();
                messages::internalError(aResp->res);
            }
            return;
        }
        aResp->res
            .jsonValue["Slot"]["Location"]["PartLocation"]["ServiceLabel"] =
            property;
        });
}

inline void
    doSlotLocationAndProperties(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                                const std::string& pcieSlotPath,
                                const std::string& service)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, pcieSlotPath,
        "xyz.openbmc_project.Inventory.Item.PCIeSlot",
        [aResp, pcieSlotPath,
         service](const boost::system::error_code ec,
                  const dbus::utility::DBusPropertiesMap& properties) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "DBUS response error for getAllProperties"
                             << ec.value();
            messages::internalError(aResp->res);
            return;
        }
        addPCIeSlotProperties(aResp->res, properties);
        getPCIeSlotLocation(aResp, pcieSlotPath, service);
        });
}

inline void getSlotLocationAndProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const std::string& pciePath, const bool isPCIeDevice,
    const std::string& service)
{
    if (!isPCIeDevice)
    {
        doSlotLocationAndProperties(aResp, pciePath, service);
    }
    else
    {
        getPCIeDeviceSlotPath(
            pciePath, aResp,
            [aResp, pciePath](const std::string& pcieDeviceSlot) {
            dbus::utility::getDbusObject(
                pcieDeviceSlot, pcieDeviceInterface,
                [aResp,
                 pcieDeviceSlot](const boost::system::error_code& ec,
                                 const dbus::utility::MapperGetObject& object) {
                if (ec || object.empty())
                {
                    BMCWEB_LOG_ERROR << "DBUS response error for getDbusObject "
                                     << ec.message();
                    messages::internalError(aResp->res);
                    return;
                }
                doSlotLocationAndProperties(aResp, pcieDeviceSlot,
                                            object.begin()->first);
                });
            });
    }
}

inline void getPCIeDeviceHealth(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                                const std::string& pcieDevicePath,
                                const std::string& service)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, service, pcieDevicePath,
        "xyz.openbmc_project.State.Decorator.OperationalStatus", "Functional",
        [aResp](const boost::system::error_code ec, const bool value) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR << "DBUS response error for Health";
                messages::internalError(aResp->res);
            }
            return;
        }

        if (!value)
        {
            aResp->res.jsonValue["Status"]["Health"] = "Critical";
        }
        });
}

inline void getPCIeDeviceState(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                               const std::string& pcieDevicePath,
                               const std::string& service)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, service, pcieDevicePath,
        "xyz.openbmc_project.Inventory.Item", "Present",
        [aResp](const boost::system::error_code& ec, const bool value) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR << "DBUS response error for State";
                messages::internalError(aResp->res);
            }
            return;
        }

        if (!value)
        {
            aResp->res.jsonValue["Status"]["State"] = "Absent";
        }
        });
}

inline void getPCIeDeviceAsset(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                               const std::string& pcieDevicePath,
                               const std::string& service)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, pcieDevicePath,
        "xyz.openbmc_project.Inventory.Decorator.Asset",
        [pcieDevicePath,
         aResp{aResp}](const boost::system::error_code& ec,
                       const dbus::utility::DBusPropertiesMap& assetList) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR << "DBUS response error for Properties"
                                 << ec.value();
                messages::internalError(aResp->res);
            }
            return;
        }

        const std::string* manufacturer = nullptr;
        const std::string* model = nullptr;
        const std::string* partNumber = nullptr;
        const std::string* serialNumber = nullptr;
        const std::string* sparePartNumber = nullptr;

        const bool success = sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), assetList, "Manufacturer",
            manufacturer, "Model", model, "PartNumber", partNumber,
            "SerialNumber", serialNumber, "SparePartNumber", sparePartNumber);

        if (!success)
        {
            messages::internalError(aResp->res);
            return;
        }

        if (manufacturer != nullptr)
        {
            aResp->res.jsonValue["Manufacturer"] = *manufacturer;
        }
        if (model != nullptr)
        {
            aResp->res.jsonValue["Model"] = *model;
        }

        if (partNumber != nullptr)
        {
            aResp->res.jsonValue["PartNumber"] = *partNumber;
        }

        if (serialNumber != nullptr)
        {
            aResp->res.jsonValue["SerialNumber"] = *serialNumber;
        }

        if (sparePartNumber != nullptr && !sparePartNumber->empty())
        {
            aResp->res.jsonValue["SparePartNumber"] = *sparePartNumber;
        }
        });
}

inline void addPCIeDeviceProperties(
    crow::Response& resp, const std::string& pcieDeviceId,
    const dbus::utility::DBusPropertiesMap& pcieDevProperties)
{
    const std::string* deviceType = nullptr;
    const std::string* generationInUse = nullptr;
    const int64_t* lanesInUse = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), pcieDevProperties, "DeviceType",
        deviceType, "GenerationInUse", generationInUse, "LanesInUse",
        lanesInUse);

    if (!success)
    {
        messages::internalError(resp);
        return;
    }

    if (deviceType != nullptr && !deviceType->empty())
    {
        resp.jsonValue["PCIeInterface"]["DeviceType"] = *deviceType;
    }

    if (generationInUse != nullptr)
    {
        std::optional<pcie_device::PCIeTypes> redfishGenerationInUse =
            redfish::pcie_util::redfishPcieGenerationFromDbus(*generationInUse);

        if (!redfishGenerationInUse)
        {
            messages::internalError(resp);
            return;
        }
        if (*redfishGenerationInUse != pcie_device::PCIeTypes::Invalid)
        {
            resp.jsonValue["PCIeInterface"]["PCIeType"] =
                *redfishGenerationInUse;
        }
    }

    // The default value of LanesInUse is 0, and the field will be
    // left as off if it is a default value.
    if (lanesInUse != nullptr && *lanesInUse != 0)
    {
        resp.jsonValue["PCIeInterface"]["LanesInUse"] = *lanesInUse;
    }

    resp.jsonValue["PCIeFunctions"]["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Systems", "system", "PCIeDevices", pcieDeviceId,
        "PCIeFunctions");
}

inline void getPCIeDeviceProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const std::string& pcieDevicePath, const std::string& service,
    const std::function<void(
        const dbus::utility::DBusPropertiesMap& pcieDevProperties)>&& callback)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, pcieDevicePath,
        "xyz.openbmc_project.Inventory.Item.PCIeDevice",
        [aResp,
         callback](const boost::system::error_code& ec,
                   const dbus::utility::DBusPropertiesMap& pcieDevProperties) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR << "DBUS response error for Properties";
                messages::internalError(aResp->res);
            }
            return;
        }
        callback(pcieDevProperties);
        });
}

inline void addPCIeDeviceCommonProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const std::string& pcieDeviceId, const bool isPCIeDevice)
{
    aResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/PCIeDevice/PCIeDevice.json>; rel=describedby");
    aResp->res.jsonValue["@odata.type"] = "#PCIeDevice.v1_9_0.PCIeDevice";
    aResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Systems", "system", "PCIeDevices", pcieDeviceId);
    aResp->res.jsonValue["Id"] = pcieDeviceId;
    if (isPCIeDevice)
    {
        aResp->res.jsonValue["Name"] = "PCIe Device";
        aResp->res.jsonValue["Status"]["State"] = "Enabled";
    }
    else
    {
        aResp->res.jsonValue["Name"] = "Empty PCIe Slot";
        aResp->res.jsonValue["Status"]["State"] = "Absent";
    }
    aResp->res.jsonValue["Status"]["Health"] = "OK";
}

inline void getValidPCIePath(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp, const std::string& pcieId,
    const std::function<void(const std::string& pciePath,
                             const std::string& service,
                             const bool isPCIeDevice)>& callback)
{
    constexpr std::array<std::string_view, 2> pcieInterface{
        "xyz.openbmc_project.Inventory.Item.PCIeDevice",
        "xyz.openbmc_project.Inventory.Item.PCIeSlot"};

    dbus::utility::getSubTree(
        inventoryPath, 0, pcieInterface,
        [pcieId, aResp,
         callback](const boost::system::error_code& ec,
                   const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error";
            messages::internalError(aResp->res);
            return;
        }
        bool found = false;
        for (const auto& [rawPath, object] : subtree)
        {
            sdbusplus::message::object_path path(rawPath);
            if (path.filename() == pcieId)
            {
                for (const auto& [service, interfaces] : object)
                {
                    for (const auto& interface : interfaces)
                    {
                        if (interface ==
                            "xyz.openbmc_project.Inventory.Item.PCIeDevice")
                        {
                            found = true;
                            callback(path, service, true);
                        }
                        if (interface ==
                            "xyz.openbmc_project.Inventory.Item.PCIeSlot")
                        {
                            found = true;
                            callback(path, service, false);
                        }
                    }
                }
            }
        }
        // Object not found
        if (!found)
        {
            messages::resourceNotFound(aResp->res, "PCIe", pcieId);
            return;
        }
        return;
        });
}

inline void handlePCIeDeviceGet(App& app, const crow::Request& req,
                                const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                                const std::string& systemName,
                                const std::string& pcieDeviceId)
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

    getValidPCIePath(aResp, pcieDeviceId,
                     [aResp, pcieDeviceId](const std::string& pcieDevicePath,
                                           const std::string& service,
                                           const bool isPCIeDevice) {
        addPCIeDeviceCommonProperties(aResp, pcieDeviceId, isPCIeDevice);
        if (isPCIeDevice)
        {
            getPCIeDeviceAsset(aResp, pcieDevicePath, service);
            getPCIeDeviceState(aResp, pcieDevicePath, service);
            getPCIeDeviceHealth(aResp, pcieDevicePath, service);

            getPCIeDeviceProperties(
                aResp, pcieDevicePath, service,
                [aResp, pcieDeviceId](
                    const dbus::utility::DBusPropertiesMap& pcieDevProperties) {
                addPCIeDeviceProperties(aResp->res, pcieDeviceId,
                                        pcieDevProperties);
                });
        }
        getSlotLocationAndProperties(aResp, pcieDevicePath, isPCIeDevice,
                                     service);
    });
}

inline void requestRoutesSystemPCIeDevice(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/PCIeDevices/<str>/")
        .privileges(redfish::privileges::getPCIeDevice)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePCIeDeviceGet, std::ref(app)));
}

inline void addPCIeFunctionList(
    crow::Response& res, const std::string& pcieDeviceId,
    const dbus::utility::DBusPropertiesMap& pcieDevProperties)
{
    nlohmann::json& pcieFunctionList = res.jsonValue["Members"];
    pcieFunctionList = nlohmann::json::array();
    static constexpr const int maxPciFunctionNum = 8;

    for (int functionNum = 0; functionNum < maxPciFunctionNum; functionNum++)
    {
        // Check if this function exists by
        // looking for a device ID
        std::string devIDProperty =
            "Function" + std::to_string(functionNum) + "DeviceId";
        const std::string* property = nullptr;
        for (const auto& propEntry : pcieDevProperties)
        {
            if (propEntry.first == devIDProperty)
            {
                property = std::get_if<std::string>(&propEntry.second);
                break;
            }
        }
        if (property == nullptr || property->empty())
        {
            continue;
        }

        nlohmann::json::object_t pcieFunction;
        pcieFunction["@odata.id"] = crow::utility::urlFromPieces(
            "redfish", "v1", "Systems", "system", "PCIeDevices", pcieDeviceId,
            "PCIeFunctions", std::to_string(functionNum));
        pcieFunctionList.push_back(std::move(pcieFunction));
    }
    res.jsonValue["PCIeFunctions@odata.count"] = pcieFunctionList.size();
}

inline void handlePCIeFunctionCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const std::string& pcieDeviceId)
{
    if (!redfish::setUpRedfishRoute(app, req, aResp))
    {
        return;
    }

    getValidPCIeDevicePath(
        pcieDeviceId, aResp,
        [aResp, pcieDeviceId](const std::string& pcieDevicePath,
                              const std::string& service) {
        aResp->res.addHeader(
            boost::beast::http::field::link,
            "</redfish/v1/JsonSchemas/PCIeFunctionCollection/PCIeFunctionCollection.json>; rel=describedby");
        aResp->res.jsonValue["@odata.type"] =
            "#PCIeFunctionCollection.PCIeFunctionCollection";
        aResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
            "redfish", "v1", "Systems", "system", "PCIeDevices", pcieDeviceId,
            "PCIeFunctions");
        aResp->res.jsonValue["Name"] = "PCIe Function Collection";
        aResp->res.jsonValue["Description"] =
            "Collection of PCIe Functions for PCIe Device " + pcieDeviceId;
        getPCIeDeviceProperties(
            aResp, pcieDevicePath, service,
            [aResp, pcieDeviceId](
                const dbus::utility::DBusPropertiesMap& pcieDevProperties) {
            addPCIeFunctionList(aResp->res, pcieDeviceId, pcieDevProperties);
            });
        });
}

inline void requestRoutesSystemPCIeFunctionCollection(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/system/PCIeDevices/<str>/PCIeFunctions/")
        .privileges(redfish::privileges::getPCIeFunctionCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePCIeFunctionCollectionGet, std::ref(app)));
}

inline bool validatePCIeFunctionId(
    const std::string& pcieFunctionId,
    const dbus::utility::DBusPropertiesMap& pcieDevProperties)
{
    std::string functionName = "Function" + pcieFunctionId;
    std::string devIDProperty = functionName + "DeviceId";

    const std::string* devIdProperty = nullptr;
    for (const auto& property : pcieDevProperties)
    {
        if (property.first == devIDProperty)
        {
            devIdProperty = std::get_if<std::string>(&property.second);
            break;
        }
    }
    return (devIdProperty != nullptr && !devIdProperty->empty());
}

inline void addPCIeFunctionProperties(
    crow::Response& resp, const std::string& pcieFunctionId,
    const dbus::utility::DBusPropertiesMap& pcieDevProperties)
{
    std::string functionName = "Function" + pcieFunctionId;
    if (!validatePCIeFunctionId(pcieFunctionId, pcieDevProperties))
    {
        messages::resourceNotFound(resp, "PCIeFunction", pcieFunctionId);
        return;
    }
    for (const auto& property : pcieDevProperties)
    {
        const std::string* strProperty =
            std::get_if<std::string>(&property.second);

        if (property.first == functionName + "DeviceId")
        {
            resp.jsonValue["DeviceId"] = *strProperty;
        }
        if (property.first == functionName + "VendorId")
        {
            resp.jsonValue["VendorId"] = *strProperty;
        }
        // TODO: FunctionType and DeviceClass are Redfish enums. The D-Bus
        // property strings should be mapped correctly to ensure these
        // strings are Redfish enum values. For now just check for empty.
        if (property.first == functionName + "FunctionType")
        {
            if (!strProperty->empty())
            {
                resp.jsonValue["FunctionType"] = *strProperty;
            }
        }
        if (property.first == functionName + "DeviceClass")
        {
            if (!strProperty->empty())
            {
                resp.jsonValue["DeviceClass"] = *strProperty;
            }
        }
        if (property.first == functionName + "ClassCode")
        {
            resp.jsonValue["ClassCode"] = *strProperty;
        }
        if (property.first == functionName + "RevisionId")
        {
            resp.jsonValue["RevisionId"] = *strProperty;
        }
        if (property.first == functionName + "SubsystemId")
        {
            resp.jsonValue["SubsystemId"] = *strProperty;
        }
        if (property.first == functionName + "SubsystemVendorId")
        {
            resp.jsonValue["SubsystemVendorId"] = *strProperty;
        }
    }
}

inline void addPCIeFunctionCommonProperties(crow::Response& resp,
                                            const std::string& pcieDeviceId,
                                            const std::string& pcieFunctionId)
{
    resp.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/PCIeFunction/PCIeFunction.json>; rel=describedby");
    resp.jsonValue["@odata.type"] = "#PCIeFunction.v1_2_3.PCIeFunction";
    resp.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Systems", "system", "PCIeDevices", pcieDeviceId,
        "PCIeFunctions", pcieFunctionId);
    resp.jsonValue["Name"] = "PCIe Function";
    resp.jsonValue["Id"] = pcieFunctionId;
    resp.jsonValue["FunctionId"] = std::stoi(pcieFunctionId);
    resp.jsonValue["Links"]["PCIeDevice"]["@odata.id"] =
        crow::utility::urlFromPieces("redfish", "v1", "Systems", "system",
                                     "PCIeDevices", pcieDeviceId);
}

inline void
    handlePCIeFunctionGet(App& app, const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                          const std::string& pcieDeviceId,
                          const std::string& pcieFunctionId)
{
    if (!redfish::setUpRedfishRoute(app, req, aResp))
    {
        return;
    }

    getValidPCIeDevicePath(
        pcieDeviceId, aResp,
        [aResp, pcieDeviceId, pcieFunctionId](const std::string& pcieDevicePath,
                                              const std::string& service) {
        getPCIeDeviceProperties(
            aResp, pcieDevicePath, service,
            [aResp, pcieDeviceId, pcieFunctionId](
                const dbus::utility::DBusPropertiesMap& pcieDevProperties) {
            addPCIeFunctionCommonProperties(aResp->res, pcieDeviceId,
                                            pcieFunctionId);
            addPCIeFunctionProperties(aResp->res, pcieFunctionId,
                                      pcieDevProperties);
            });
        });
}

inline void requestRoutesSystemPCIeFunction(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/system/PCIeDevices/<str>/PCIeFunctions/<str>/")
        .privileges(redfish::privileges::getPCIeFunction)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handlePCIeFunctionGet, std::ref(app)));
}

} // namespace redfish
