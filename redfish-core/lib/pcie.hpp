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
#include "generated/enums/pcie_device.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/system/linux_error.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

namespace redfish
{

static constexpr char const* pcieService = "xyz.openbmc_project.PCIe";
static constexpr char const* pciePath = "/xyz/openbmc_project/PCIe";
static constexpr char const* pcieDeviceInterface =
    "xyz.openbmc_project.PCIe.Device";

static inline void handlePCIeDevicePath(
    const std::string& pcieDeviceId,
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const dbus::utility::MapperGetSubTreePathsResponse& pcieDevicePaths,
    const std::function<void(const std::string& pcieDevicePath,
                             const std::string& service)>&& callback)

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

static inline void getPCIeDevicePath(
    const std::string& pcieDeviceId,
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    std::function<void(const std::string& pcieDevicePath,
                       const std::string& service)>&& callback)
{
    constexpr std::array<std::string_view, 1> interfaces{
        "xyz.openbmc_project.Inventory.Item.PCIeDevice"};

    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [pcieDeviceId, aResp{std::move(aResp)},
         callback](const boost::system::error_code& ec,
                   const dbus::utility::MapperGetSubTreePathsResponse&
                       pcieDevicePaths) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "D-Bus response error on GetSubTree " << ec;
            messages::internalError(aResp->res);
            return;
        }
        handlePCIeDevicePath(pcieDeviceId, aResp, pcieDevicePaths,
                             std::move(callback));
        return;
        });
}

static inline void
    getPCIeDeviceList(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& name)
{
    dbus::utility::getSubTreePaths(
        pciePath, 1, {},
        [asyncResp, name](const boost::system::error_code& ec,
                          const dbus::utility::MapperGetSubTreePathsResponse&
                              pcieDevicePaths) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "no PCIe device paths found ec: "
                             << ec.message();
            // Not an error, system just doesn't have PCIe info
            return;
        }
        nlohmann::json& pcieDeviceList = asyncResp->res.jsonValue[name];
        pcieDeviceList = nlohmann::json::array();
        for (const std::string& pcieDevicePath : pcieDevicePaths)
        {
            size_t devStart = pcieDevicePath.rfind('/');
            if (devStart == std::string::npos)
            {
                continue;
            }

            std::string devName = pcieDevicePath.substr(devStart + 1);
            if (devName.empty())
            {
                continue;
            }
            nlohmann::json::object_t pcieDevice;
            pcieDevice["@odata.id"] = crow::utility::urlFromPieces(
                "redfish", "v1", "Systems", "system", "PCIeDevices", devName);
            pcieDeviceList.push_back(std::move(pcieDevice));
        }
        asyncResp->res.jsonValue[name + "@odata.count"] = pcieDeviceList.size();
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

    constexpr std::array<std::string_view, 1> interfaces{
        "xyz.openbmc_project.Inventory.Item.PCIeDevice"};
    collection_util::getCollectionMembers(
        aResp, boost::urls::url("/redfish/v1/Systems/system/PCIeDevices"),
        interfaces);
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

inline std::optional<pcie_device::PCIeTypes>
    redfishPcieGenerationFromDbus(const std::string& generationInUse)
{
    if (generationInUse ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen1")
    {
        return pcie_device::PCIeTypes::Gen1;
    }
    if (generationInUse ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen2")
    {
        return pcie_device::PCIeTypes::Gen2;
    }
    if (generationInUse ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen3")
    {
        return pcie_device::PCIeTypes::Gen3;
    }
    if (generationInUse ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen4")
    {
        return pcie_device::PCIeTypes::Gen4;
    }
    if (generationInUse ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen5")
    {
        return pcie_device::PCIeTypes::Gen5;
    }
    if (generationInUse.empty() ||
        generationInUse ==
            "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Unknown")
    {
        return pcie_device::PCIeTypes::Invalid;
    }

    // The value is not unknown or Gen1-5, need return an internal error.
    return std::nullopt;
}

inline void addPCIeDeviceProperties(
    crow::Response& resp, const std::string& pcieDeviceId,
    const dbus::utility::DBusPropertiesMap& pcieDevProperties)
{
    const std::string* manufacturer = nullptr;
    const std::string* deviceType = nullptr;
    const std::string* generationInUse = nullptr;
    const int64_t* lanesInUse = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), pcieDevProperties, "DeviceType",
        deviceType, "GenerationInUse", generationInUse, "LanesInUse",
        lanesInUse, "Manufacturer", manufacturer);

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
            redfishPcieGenerationFromDbus(*generationInUse);

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

    if (lanesInUse != nullptr && *lanesInUse != 0)
    {
        resp.jsonValue["PCIeInterface"]["LanesInUse"] = *lanesInUse;
    }

    if (manufacturer != nullptr && !manufacturer->empty())
    {
        resp.jsonValue["PCIeInterface"]["Manufacturer"] = *manufacturer;
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

inline void addPCIeDeviceHeader(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                                const std::string& pcieDeviceId)
{
    aResp->res.addHeader(boost::beast::http::field::link,
                         "</redfish/v1/JsonSchemas/PCIeDevice/"
                         "PCIeDevice.json>; rel=describedby");
    aResp->res.jsonValue["@odata.type"] = "#PCIeDevice.v1_9_0.PCIeDevice";
    aResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Systems", "system", "PCIeDevices", pcieDeviceId);
    aResp->res.jsonValue["Name"] = "PCIe Device";
    aResp->res.jsonValue["Id"] = pcieDeviceId;
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

    getPCIeDevicePath(pcieDeviceId, aResp,
                      [aResp, pcieDeviceId](const std::string& pcieDevicePath,
                                            const std::string& service) {
        addPCIeDeviceHeader(aResp, pcieDeviceId);
        getPCIeDeviceProperties(
            aResp, pcieDevicePath, service,
            [aResp, pcieDeviceId](
                const dbus::utility::DBusPropertiesMap& pcieDevProperties) {
            addPCIeDeviceProperties(aResp->res, pcieDeviceId,
                                    pcieDevProperties);
            });
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

        pcieFunctionList.push_back(
            {{"@odata.id", "/redfish/v1/Systems/"
                           "system/PCIeDevices/" +
                               pcieDeviceId + "/PCIeFunctions/" +
                               std::to_string(functionNum)}});
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
    aResp->res.addHeader(boost::beast::http::field::link,
                         "</redfish/v1/JsonSchemas/PCIeFunctionCollection/"
                         "PCIeFunctionCollection.json>; rel=describedby");
    aResp->res.jsonValue["@odata.type"] =
        "#PCIeFunctionCollection.PCIeFunctionCollection";
    aResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Systems/system/PCIeDevices/" + pcieDeviceId +
        "/PCIeFunctions";
    aResp->res.jsonValue["Name"] = "PCIe Function Collection";
    aResp->res.jsonValue["Description"] =
        "Collection of PCIe Functions for PCIe Device " + pcieDeviceId;

    getPCIeDevicePath(pcieDeviceId, aResp,
                      [aResp, pcieDeviceId](const std::string& pcieDevicePath,
                                            const std::string& service) {
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

inline void requestRoutesSystemPCIeFunction(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/system/PCIeDevices/<str>/PCIeFunctions/<str>/")
        .privileges(redfish::privileges::getPCIeFunction)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& device, const std::string& function) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        auto getPCIeDeviceCallback =
            [asyncResp, device, function](
                const boost::system::error_code& ec,
                const dbus::utility::DBusPropertiesMap& pcieDevProperties) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG
                    << "failed to get PCIe Device properties ec: " << ec.value()
                    << ": " << ec.message();
                if (ec.value() ==
                    boost::system::linux_error::bad_request_descriptor)
                {
                    messages::resourceNotFound(asyncResp->res, "PCIeDevice",
                                               device);
                }
                else
                {
                    messages::internalError(asyncResp->res);
                }
                return;
            }

            // Check if this function exists by looking for a device
            // ID
            std::string functionName = "Function" + function;
            std::string devIDProperty = functionName + "DeviceId";

            const std::string* devIdProperty = nullptr;
            for (const auto& property : pcieDevProperties)
            {
                if (property.first == devIDProperty)
                {
                    devIdProperty = std::get_if<std::string>(&property.second);
                    continue;
                }
            }
            if (devIdProperty == nullptr || devIdProperty->empty())
            {
                messages::resourceNotFound(asyncResp->res, "PCIeFunction",
                                           function);
                return;
            }

            asyncResp->res.jsonValue["@odata.type"] =
                "#PCIeFunction.v1_2_0.PCIeFunction";
            asyncResp->res.jsonValue["@odata.id"] =
                crow::utility::urlFromPieces("redfish", "v1", "Systems",
                                             "system", "PCIeDevices", device,
                                             "PCIeFunctions", function);
            asyncResp->res.jsonValue["Name"] = "PCIe Function";
            asyncResp->res.jsonValue["Id"] = function;
            asyncResp->res.jsonValue["FunctionId"] = std::stoi(function);
            asyncResp->res.jsonValue["Links"]["PCIeDevice"]["@odata.id"] =
                crow::utility::urlFromPieces("redfish", "v1", "Systems",
                                             "system", "PCIeDevices", device);

            for (const auto& property : pcieDevProperties)
            {
                const std::string* strProperty =
                    std::get_if<std::string>(&property.second);
                if (property.first == functionName + "DeviceId")
                {
                    asyncResp->res.jsonValue["DeviceId"] = *strProperty;
                }
                if (property.first == functionName + "VendorId")
                {
                    asyncResp->res.jsonValue["VendorId"] = *strProperty;
                }
                if (property.first == functionName + "FunctionType")
                {
                    asyncResp->res.jsonValue["FunctionType"] = *strProperty;
                }
                if (property.first == functionName + "DeviceClass")
                {
                    asyncResp->res.jsonValue["DeviceClass"] = *strProperty;
                }
                if (property.first == functionName + "ClassCode")
                {
                    asyncResp->res.jsonValue["ClassCode"] = *strProperty;
                }
                if (property.first == functionName + "RevisionId")
                {
                    asyncResp->res.jsonValue["RevisionId"] = *strProperty;
                }
                if (property.first == functionName + "SubsystemId")
                {
                    asyncResp->res.jsonValue["SubsystemId"] = *strProperty;
                }
                if (property.first == functionName + "SubsystemVendorId")
                {
                    asyncResp->res.jsonValue["SubsystemVendorId"] =
                        *strProperty;
                }
            }
        };
        std::string escapedPath = std::string(pciePath) + "/" + device;
        dbus::utility::escapePathForDbus(escapedPath);
        sdbusplus::asio::getAllProperties(
            *crow::connections::systemBus, pcieService, escapedPath,
            pcieDeviceInterface, std::move(getPCIeDeviceCallback));
        });
}

} // namespace redfish
