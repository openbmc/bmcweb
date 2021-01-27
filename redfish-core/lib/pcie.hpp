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
#include "utils/dbus_utils.hpp"

#include <boost/system/linux_error.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

namespace redfish
{

static constexpr char const* pcieService = "xyz.openbmc_project.PCIe";
static constexpr char const* pciePath = "/xyz/openbmc_project/PCIe";
static constexpr char const* pcieDeviceInterface =
    "xyz.openbmc_project.Inventory.Item.PCIeDevice";

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
            pcieDevice["@odata.id"] =
                "/redfish/v1/Systems/system/PCIeDevices/" + devName;
            pcieDeviceList.push_back(std::move(pcieDevice));
        }
        asyncResp->res.jsonValue[name + "@odata.count"] = pcieDeviceList.size();
        });
}

inline void requestRoutesSystemPCIeDeviceCollection(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/PCIeDevices/")
        .privileges(redfish::privileges::getPCIeDeviceCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        asyncResp->res.jsonValue["@odata.type"] =
            "#PCIeDeviceCollection.PCIeDeviceCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/PCIeDevices";
        asyncResp->res.jsonValue["Name"] = "PCIe Device Collection";
        asyncResp->res.jsonValue["Description"] = "Collection of PCIe Devices";
        asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
        asyncResp->res.jsonValue["Members@odata.count"] = 0;
        getPCIeDeviceList(asyncResp, "Members");
        });
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

inline void requestRoutesSystemPCIeDevice(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/PCIeDevices/<str>/")
        .privileges(redfish::privileges::getPCIeDevice)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName, const std::string& device) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        asyncResp->res.jsonValue = {
            {"@odata.type", "#PCIeDevice.v1_6_0.PCIeDevice"},
            {"@odata.id", "/redfish/v1/Systems/system/PCIeDevices/" + device},
            {"Name", "PCIe Device"},
            {"Id", device}};

        crow::connections::systemBus->async_method_call(
            [asyncResp, device](
                const boost::system::error_code ec,
                const std::vector<std::pair<
                    std::string, std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>>&
                    subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "D-Bus response error on GetSubTree " << ec;
                return;
            }
            if (subtree.empty())
            {
                BMCWEB_LOG_DEBUG << "Can't find bmc D-Bus object!";
                return;
            }

            for (const auto& [objectPath, serviceMap] : subtree)
            {
                // Ignore any objects which don't end with our desired
                // device name
                if (!boost::ends_with(objectPath, device))
                {
                    continue;
                }

                for (const auto& [serviceName, interfaceList] : serviceMap)
                {
                    auto getPCIeDeviceCallback =
                        [asyncResp,
                         device](const boost::system::error_code ec2,
                                 const dbus::utility::DBusPropertiesMap&
                                     pcieDevProperties) {
                        if (ec2)
                        {
                            BMCWEB_LOG_DEBUG << "failed to get PCIe "
                                                "Device properties ec: "
                                             << ec2.value() << ": "
                                             << ec2.message();
                            if (ec2.value() == boost::system::linux_error::
                                                   bad_request_descriptor)
                            {
                                messages::resourceNotFound(
                                    asyncResp->res, "PCIeDevice", device);
                            }
                            else
                            {
                                messages::internalError(asyncResp->res);
                            }
                            return;
                        }

                        const std::string* manufacturer = nullptr;
                        const std::string* deviceType = nullptr;
                        const std::string* generationInUse = nullptr;
                        const std::string* partNumber = nullptr;
                        const std::string* serialNumber = nullptr;
                        const std::string* model = nullptr;
                        const std::string* sparePartNumber = nullptr;
                        const size_t* lanesInUse = nullptr;

                        const bool success = sdbusplus::unpackPropertiesNoThrow(
                            dbus_utils::UnpackErrorPrinter(), pcieDevProperties,
                            "Manufacturer", manufacturer, "DeviceType",
                            deviceType, "GenerationInUse", generationInUse,
                            "LanesInUse", lanesInUse, "GenerationInUse",
                            generationInUse, "PartNumber", partNumber,
                            "SerialNumber", serialNumber, "Model", model,
                            "SparePartNumber", sparePartNumber);

                        if (!success)
                        {
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        // The default value of LanesInUse is 0, and the field
                        // will be
                        // left as off if it is a default value.
                        if (lanesInUse != nullptr && *lanesInUse != 0)
                        {
                            asyncResp->res
                                .jsonValue["PCIeInterface"]["LanesInUse"] =
                                *lanesInUse;
                        }

                        if (generationInUse != nullptr)
                        {
                            std::optional<pcie_device::PCIeTypes>
                                redfishGenerationInUse =
                                    redfishPcieGenerationFromDbus(
                                        *generationInUse);
                            if (!redfishGenerationInUse)
                            {
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            if (*redfishGenerationInUse !=
                                pcie_device::PCIeTypes::Invalid)
                            {
                                asyncResp->res
                                    .jsonValue["PCIeInterface"]["PCIeType"] =
                                    *redfishGenerationInUse;
                            }
                        }

                        if (manufacturer != nullptr)
                        {
                            asyncResp->res.jsonValue["Manufacturer"] =
                                *manufacturer;
                        }

                        if (deviceType != nullptr && !deviceType->empty())
                        {
                            asyncResp->res.jsonValue["DeviceType"] =
                                *deviceType;
                        }

                        if (partNumber != nullptr)
                        {
                            asyncResp->res.jsonValue["PartNumber"] =
                                *partNumber;
                        }

                        if (serialNumber != nullptr)
                        {
                            asyncResp->res.jsonValue["SerialNumber"] =
                                *serialNumber;
                        }

                        if (model != nullptr)
                        {
                            asyncResp->res.jsonValue["Model"] = *model;
                        }

                        if (sparePartNumber != nullptr)
                        {
                            asyncResp->res.jsonValue["SparePartNumber"] =
                                *sparePartNumber;
                        }

                        asyncResp->res.jsonValue["PCIeFunctions"] = {
                            {"@odata.id",
                             "/redfish/v1/Systems/system/PCIeDevices/" +
                                 device + "/PCIeFunctions"}};
                    };

                    sdbusplus::asio::getAllProperties(
                        *crow::connections::systemBus, serviceName, objectPath,
                        "", std::move(getPCIeDeviceCallback));
                }
            }
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
            "/xyz/openbmc_project/inventory", int32_t(0),
            std::array<const char*, 1>{pcieDeviceInterface});
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
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& device) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }

        asyncResp->res.jsonValue["@odata.type"] =
            "#PCIeFunctionCollection.PCIeFunctionCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/PCIeDevices/" + device +
            "/PCIeFunctions";
        asyncResp->res.jsonValue["Name"] = "PCIe Function Collection";
        asyncResp->res.jsonValue["Description"] =
            "Collection of PCIe Functions for PCIe Device " + device;

        crow::connections::systemBus->async_method_call(
            [asyncResp, device](
                const boost::system::error_code ec,
                const std::vector<std::pair<
                    std::string, std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>>&
                    subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "D-Bus response error on GetSubTree " << ec;
                return;
            }
            if (subtree.empty())
            {
                BMCWEB_LOG_DEBUG << "Can't find bmc D-Bus object!";
                return;
            }

            for (const auto& [objectPath, serviceMap] : subtree)
            {
                // Ignore any objects which don't end with our desired
                // device name
                if (!boost::ends_with(objectPath, device))
                {
                    continue;
                }

                for (const auto& [serviceName, interfaceList] : serviceMap)
                {
                    for (const auto& interface : interfaceList)
                    {
                        if (interface != pcieDeviceInterface)
                        {
                            continue;
                        }
                        auto getPCIeDeviceCallback =
                            [asyncResp,
                             device](const boost::system::error_code ec2,
                                     const dbus::utility::DBusPropertiesMap&
                                         pcieDevProperties) {
                            if (ec2)
                            {
                                BMCWEB_LOG_DEBUG << "failed to get PCIe Device "
                                                    "properties ec: "
                                                 << ec2.value() << ": "
                                                 << ec2.message();
                                if (ec2.value() == boost::system::linux_error::
                                                       bad_request_descriptor)
                                {
                                    messages::resourceNotFound(
                                        asyncResp->res, "PCIeDevice", device);
                                }
                                else
                                {
                                    messages::internalError(asyncResp->res);
                                }
                                return;
                            }

                            nlohmann::json& pcieFunctionList =
                                asyncResp->res.jsonValue["Members"];
                            pcieFunctionList = nlohmann::json::array();
                            static constexpr const int maxPciFunctionNum = 8;

                            for (int functionNum = 0;
                                 functionNum < maxPciFunctionNum; functionNum++)
                            {
                                // Check if this function exists by
                                // looking for a device ID
                                std::string devIDProperty =
                                    "Function" + std::to_string(functionNum) +
                                    "DeviceId";
                                const std::string* property = nullptr;
                                for (const auto& propEntry : pcieDevProperties)
                                {
                                    if (propEntry.first == devIDProperty)
                                    {
                                        property = std::get_if<std::string>(
                                            &propEntry.second);
                                    }
                                }
                                if (property == nullptr || property->empty())
                                {
                                    continue;
                                }

                                pcieFunctionList.push_back(
                                    {{"@odata.id",
                                      "/redfish/v1/Systems/"
                                      "system/PCIeDevices/" +
                                          device + "/PCIeFunctions/" +
                                          std::to_string(functionNum)}});
                            }
                            asyncResp->res
                                .jsonValue["PCIeFunctions@odata.count"] =
                                pcieFunctionList.size();
                        };
                        sdbusplus::asio::getAllProperties(
                            *crow::connections::systemBus, serviceName,
                            objectPath, pcieDeviceInterface,
                            std::move(getPCIeDeviceCallback));
                    }
                }
            }
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
            "/xyz/openbmc_project/inventory", int32_t(0),
            std::array<const char*, 1>{pcieDeviceInterface});
        });
} // namespace redfish

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

        crow::connections::systemBus->async_method_call(
            [asyncResp, device, function](
                const boost::system::error_code ec,
                const std::vector<std::pair<
                    std::string, std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>>&
                    subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "D-Bus response error on GetSubTree " << ec;
                return;
            }

            if (subtree.empty())
            {
                BMCWEB_LOG_DEBUG << "Can't find bmc D-Bus object!";
                return;
            }

            for (const auto& [objectPath, serviceMap] : subtree)
            {
                // Ignore any objects which don't end with our desired
                // device name
                if (!boost::ends_with(objectPath, device))
                {
                    continue;
                }

                for (const auto& [serviceName, interfaceList] : serviceMap)
                {
                    for (const auto& interface : interfaceList)
                    {
                        if (interface != pcieDeviceInterface)
                        {
                            continue;
                        }
                        auto getPCIeDeviceCallback =
                            [asyncResp, device,
                             function](const boost::system::error_code ec2,
                                       const dbus::utility::DBusPropertiesMap&
                                           pcieDevProperties) {
                            if (ec2)
                            {
                                BMCWEB_LOG_DEBUG << "failed to get PCIe Device "
                                                    "properties ec: "
                                                 << ec2.value() << ": "
                                                 << ec2.message();
                                if (ec2.value() == boost::system::linux_error::
                                                       bad_request_descriptor)
                                {
                                    messages::resourceNotFound(
                                        asyncResp->res, "PCIeDevice", device);
                                }
                                else
                                {
                                    messages::internalError(asyncResp->res);
                                }
                                return;
                            }

                            // Check if this function exists by
                            // looking for a device ID
                            std::string functionName = "Function" + function;
                            std::string devIDProperty =
                                functionName + "DeviceId";

                            const std::string* devIdProperty = nullptr;
                            for (const auto& property : pcieDevProperties)
                            {
                                if (property.first == devIDProperty)
                                {
                                    devIdProperty = std::get_if<std::string>(
                                        &property.second);
                                    continue;
                                }
                            }
                            if (devIdProperty == nullptr ||
                                devIdProperty->empty())
                            {
                                messages::resourceNotFound(
                                    asyncResp->res, "PCIeFunction", function);
                                return;
                            }

                            std::string dataId = "/redfish/v1/Systems/system/"
                                                 "PCIeDevices/";
                            dataId.append(device);
                            dataId.append("/PCIeFunctions/");
                            dataId.append(function);

                            asyncResp->res.jsonValue = {
                                {"@odata.type", "#PCIeFunction.v1_"
                                                "2_3.PCIeFunction"},
                                {"@odata.id", dataId},
                                {"Name", "PCIe Function"},
                                {"Id", function},
                                {"FunctionId", std::stoi(function)},
                                {"Links",
                                 {{"PCIeDevice",
                                   {{"@odata.id", "/redfish/v1/Systems/system/"
                                                  "PCIeDevices/" +
                                                      device}}}}}};

                            for (const auto& property : pcieDevProperties)
                            {
                                const std::string* strProperty =
                                    std::get_if<std::string>(&property.second);

                                if (property.first == functionName + "DeviceId")
                                {
                                    asyncResp->res.jsonValue["DeviceId"] =
                                        *strProperty;
                                }
                                if (property.first == functionName + "VendorId")
                                {
                                    asyncResp->res.jsonValue["VendorId"] =
                                        *strProperty;
                                }
                                if (property.first ==
                                    functionName + "FunctionType")
                                {
                                    if (!strProperty->empty())
                                    {
                                        asyncResp->res
                                            .jsonValue["FunctionType"] =
                                            *strProperty;
                                    }
                                }
                                if (property.first ==
                                    functionName + "DeviceClass")
                                {
                                    if (!strProperty->empty())
                                    {
                                        asyncResp->res
                                            .jsonValue["DeviceClass"] =
                                            *strProperty;
                                    }
                                }
                                if (property.first ==
                                    functionName + "ClassCode")
                                {
                                    asyncResp->res.jsonValue["ClassCode"] =
                                        *strProperty;
                                }
                                if (property.first ==
                                    functionName + "RevisionId")
                                {
                                    asyncResp->res.jsonValue["RevisionId"] =
                                        *strProperty;
                                }
                                if (property.first ==
                                    functionName + "SubsystemId")
                                {
                                    asyncResp->res.jsonValue["SubsystemId"] =
                                        *strProperty;
                                }
                                if (property.first ==
                                    functionName + "SubsystemVendorId")
                                {
                                    asyncResp->res
                                        .jsonValue["SubsystemVendorId"] =
                                        *strProperty;
                                }
                            }
                        };

                        sdbusplus::asio::getAllProperties(
                            *crow::connections::systemBus, serviceName,
                            objectPath, pcieDeviceInterface,
                            std::move(getPCIeDeviceCallback));
                    }
                }
            }
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
            "/xyz/openbmc_project/inventory", int32_t(0),
            std::array<const char*, 1>{pcieDeviceInterface});
        });
}

} // namespace redfish
