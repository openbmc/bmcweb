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

#include <app.hpp>
#include <boost/system/linux_error.hpp>
#include <registries/privilege_registry.hpp>

namespace redfish
{

static constexpr char const* pcieService = "xyz.openbmc_project.PCIe";
static constexpr char const* pciePath = "/xyz/openbmc_project/PCIe";
static constexpr char const* pcieDeviceInterface =
    "xyz.openbmc_project.PCIe.Device";
static constexpr char const* assetInterface =
    "xyz.openbmc_project.Inventory.Decorator.Asset";
static constexpr char const* uuidInterface = "xyz.openbmc_project.Common.UUID";
static constexpr char const* stateInterface =
    "xyz.openbmc_project.State.Chassis";

static inline std::string getPCIeType(const std::string& pcieType)
{
    if (pcieType ==
        "xyz.openbmc_project.Inventory.Item.PCIeDevice.PCIeTypes.Gen1")
    {
        return "Gen1";
    }
    if (pcieType ==
        "xyz.openbmc_project.Inventory.Item.PCIeDevice.PCIeTypes.Gen2")
    {
        return "Gen2";
    }
    if (pcieType ==
        "xyz.openbmc_project.Inventory.Item.PCIeDevice.PCIeTypes.Gen3")
    {
        return "Gen3";
    }
    if (pcieType ==
        "xyz.openbmc_project.Inventory.Item.PCIeDevice.PCIeTypes.Gen4")
    {
        return "Gen4";
    }
    if (pcieType ==
        "xyz.openbmc_project.Inventory.Item.PCIeDevice.PCIeTypes.Gen5")
    {
        return "Gen5";
    }
    // Unknown or others
    return "Unknown";
}

static inline void
    getPCIeDeviceList(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& name,
                      const std::string& path = pciePath,
                      const std::string& chassisId = std::string())
{
    auto getPCIeMapCallback =
        [asyncResp, name,
         chassisId](const boost::system::error_code ec,
                    const std::vector<std::string>& pcieDevicePaths) {
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
                if (!chassisId.empty())
                {
                    std::string pcieDeviceURI = "/redfish/v1/Chassis/";
                    pcieDeviceURI += chassisId;
                    pcieDeviceURI += "/PCIeDevices/";
                    pcieDeviceURI += devName;
                    pcieDeviceList.push_back({{"@odata.id", pcieDeviceURI}});
                }
                else
                {
                    pcieDeviceList.push_back(
                        {{"@odata.id",
                          "/redfish/v1/Systems/system/PCIeDevices/" +
                              devName}});
                }
            }
            asyncResp->res.jsonValue[name + "@odata.count"] =
                pcieDeviceList.size();
        };
    crow::connections::systemBus->async_method_call(
        std::move(getPCIeMapCallback), "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        std::string(path) + "/", 1, std::array<std::string, 0>());
}

// PCIeDevice asset properties
static inline void
    getPCIeDeviceAssetData(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& device, const std::string& path,
                           const std::string& service)
{
    auto getPCIeDeviceAssetCallback =
        [asyncResp](const boost::system::error_code ec,
                    const std::vector<
                        std::pair<std::string, std::variant<std::string>>>&
                        propertiesList) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(asyncResp->res);
                return;
            }
            for (const std::pair<std::string, std::variant<std::string>>&
                     property : propertiesList)
            {
                const std::string& propertyName = property.first;
                if ((propertyName == "PartNumber") ||
                    (propertyName == "SerialNumber") ||
                    (propertyName == "Manufacturer") ||
                    (propertyName == "Model"))
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value != nullptr)
                    {
                        asyncResp->res.jsonValue[propertyName] = *value;
                    }
                }
            }
        };
    std::string escapedPath = std::string(path) + "/" + device;
    dbus::utility::escapePathForDbus(escapedPath);
    crow::connections::systemBus->async_method_call(
        std::move(getPCIeDeviceAssetCallback), service, escapedPath,
        "org.freedesktop.DBus.Properties", "GetAll", assetInterface);
}

// PCIeDevice UUID
static inline void
    getPCIeDeviceUUID(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& device, const std::string& path,
                      const std::string& service)
{
    auto getPCIeDeviceUUIDCallback =
        [asyncResp](const boost::system::error_code ec,
                    const std::variant<std::string>& uuid) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(asyncResp->res);
                return;
            }
            const std::string* s = std::get_if<std::string>(&uuid);
            if (s != nullptr)
            {
                asyncResp->res.jsonValue["UUID"] = *s;
            }
        };
    std::string escapedPath = std::string(path) + "/" + device;
    dbus::utility::escapePathForDbus(escapedPath);
    crow::connections::systemBus->async_method_call(
        std::move(getPCIeDeviceUUIDCallback), service, escapedPath,
        "org.freedesktop.DBus.Properties", "Get", uuidInterface, "UUID");
}

// PCIeDevice State
static inline void
    getPCIeDeviceState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& device, const std::string& path,
                       const std::string& service)
{
    auto getPCIeDeviceStateCallback =
        [asyncResp](const boost::system::error_code ec,
                    const std::variant<std::string>& deviceState) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(asyncResp->res);
                return;
            }
            const std::string* s = std::get_if<std::string>(&deviceState);
            if (s != nullptr)
            {
                if (*s == "xyz.openbmc_project.State.Chassis.PowerState.On")
                {
                    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
                    asyncResp->res.jsonValue["Status"]["Health"] = "OK";
                }
                else if (*s ==
                         "xyz.openbmc_project.State.Chassis.PowerState.Off")
                {
                    asyncResp->res.jsonValue["Status"]["State"] =
                        "StandbyOffline";
                    asyncResp->res.jsonValue["Status"]["Health"] = "Critical";
                }
            }
        };
    std::string escapedPath = std::string(path) + "/" + device;
    dbus::utility::escapePathForDbus(escapedPath);
    crow::connections::systemBus->async_method_call(
        std::move(getPCIeDeviceStateCallback), service, escapedPath,
        "org.freedesktop.DBus.Properties", "Get", stateInterface,
        "CurrentPowerState");
}

static inline void
    getPCIeDevice(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                  const std::string& device, const std::string& path = pciePath,
                  const std::string& service = pcieService,
                  const std::string& deviceIntf = pcieDeviceInterface)
{
    auto getPCIeDeviceCallback =
        [asyncResp,
         device](const boost::system::error_code ec,
                 const std::vector<
                     std::pair<std::string, std::variant<std::string, size_t>>>&
                     propertiesList) {
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

            for (const std::pair<std::string,
                                 std::variant<std::string, size_t>>& property :
                 propertiesList)
            {
                const std::string& propertyName = property.first;
                if ((propertyName == "Manufacturer") ||
                    (propertyName == "DeviceType"))
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value != nullptr)
                    {
                        asyncResp->res.jsonValue[propertyName] = *value;
                    }
                }
                else if ((propertyName == "LanesInUse") ||
                         (propertyName == "MaxLanes"))
                {
                    const size_t* value = std::get_if<size_t>(&property.second);
                    if (value != nullptr)
                    {
                        asyncResp->res
                            .jsonValue["PCIeInterface"][propertyName] = *value;
                    }
                }
                else if ((propertyName == "PCIeType") ||
                         (propertyName == "MaxPCIeType"))
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value != nullptr)
                    {
                        asyncResp->res
                            .jsonValue["PCIeInterface"][propertyName] =
                            getPCIeType(*value);
                    }
                }
            }
        };
    std::string escapedPath = std::string(path) + "/" + device;
    dbus::utility::escapePathForDbus(escapedPath);
    crow::connections::systemBus->async_method_call(
        std::move(getPCIeDeviceCallback), service, escapedPath,
        "org.freedesktop.DBus.Properties", "GetAll", deviceIntf);
}

static inline void getPCIeDeviceFunctionsList(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& device, const std::string& path = pciePath,
    const std::string& service = pcieService,
    const std::string& deviceIntf = pcieDeviceInterface,
    const std::string& chassisId = std::string())
{
    auto getPCIeDeviceCallback =
        [asyncResp, device, chassisId](
            const boost::system::error_code ec,
            boost::container::flat_map<std::string, std::variant<std::string>>&
                pcieDevProperties) {
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

            nlohmann::json& pcieFunctionList =
                asyncResp->res.jsonValue["Members"];
            pcieFunctionList = nlohmann::json::array();
            static constexpr const int maxPciFunctionNum = 8;
            for (int functionNum = 0; functionNum < maxPciFunctionNum;
                 functionNum++)
            {
                // Check if this function exists by looking for a device
                // ID
                std::string devIDProperty =
                    "Function" + std::to_string(functionNum) + "DeviceId";
                std::string* property =
                    std::get_if<std::string>(&pcieDevProperties[devIDProperty]);
                if (property && !property->empty())
                {
                    if (!chassisId.empty())
                    {
                        std::string pcieFunctionURI = "/redfish/v1/Chassis/";
                        pcieFunctionURI += chassisId;
                        pcieFunctionURI += "/PCIeDevices/";
                        pcieFunctionURI += device;
                        pcieFunctionURI += "/PCIeFunctions/";
                        pcieFunctionURI += std::to_string(functionNum);
                        pcieFunctionList.push_back(
                            {{"@odata.id", pcieFunctionURI}});
                    }
                    else
                    {
                        pcieFunctionList.push_back(
                            {{"@odata.id",
                              "/redfish/v1/Systems/system/PCIeDevices/" +
                                  device + "/PCIeFunctions/" +
                                  std::to_string(functionNum)}});
                    }
                }
            }
            asyncResp->res.jsonValue["PCIeFunctions@odata.count"] =
                pcieFunctionList.size();
        };
    std::string escapedPath = std::string(path) + "/" + device;
    dbus::utility::escapePathForDbus(escapedPath);
    crow::connections::systemBus->async_method_call(
        std::move(getPCIeDeviceCallback), service, escapedPath,
        "org.freedesktop.DBus.Properties", "GetAll", deviceIntf);
}

static inline void
    getPCIeDeviceFunction(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& device,
                          const std::string& function,
                          const std::string& path = pciePath,
                          const std::string& service = pcieService,
                          const std::string& deviceIntf = pcieDeviceInterface)
{
    auto getPCIeDeviceCallback =
        [asyncResp, device, function](
            const boost::system::error_code ec,
            boost::container::flat_map<std::string, std::variant<std::string>>&
                pcieDevProperties) {
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

            // Check if this function exists by looking for a device ID
            std::string devIDProperty = "Function" + function + "DeviceId";
            if (std::string* property =
                    std::get_if<std::string>(&pcieDevProperties[devIDProperty]);
                property && property->empty())
            {
                messages::resourceNotFound(asyncResp->res, "PCIeFunction",
                                           function);
                return;
            }

            for (const auto& property : pcieDevProperties)
            {
                const std::string& propertyName = property.first;
                if (propertyName == "Function" + function + "DeviceId")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value != nullptr)
                    {
                        asyncResp->res.jsonValue["DeviceId"] = *value;
                    }
                }
                else if (propertyName == "Function" + function + "VendorId")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value != nullptr)
                    {
                        asyncResp->res.jsonValue["VendorId"] = *value;
                    }
                }
                else if (propertyName == "Function" + function + "FunctionType")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value != nullptr)
                    {
                        asyncResp->res.jsonValue["FunctionType"] = *value;
                    }
                }
                else if (propertyName == "Function" + function + "DeviceClass")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value != nullptr)
                    {
                        asyncResp->res.jsonValue["DeviceClass"] = *value;
                    }
                }
                else if (propertyName == "Function" + function + "ClassCode")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value != nullptr)
                    {
                        asyncResp->res.jsonValue["ClassCode"] = *value;
                    }
                }
                else if (propertyName == "Function" + function + "RevisionId")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value != nullptr)
                    {
                        asyncResp->res.jsonValue["RevisionId"] = *value;
                    }
                }
                else if (propertyName == "Function" + function + "SubsystemId")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value != nullptr)
                    {
                        asyncResp->res.jsonValue["SubsystemId"] = *value;
                    }
                }
                else if (propertyName ==
                         "Function" + function + "SubsystemVendorId")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value != nullptr)
                    {
                        asyncResp->res.jsonValue["SubsystemVendorId"] = *value;
                    }
                }
            }
        };
    std::string escapedPath = std::string(path) + "/" + device;
    dbus::utility::escapePathForDbus(escapedPath);
    crow::connections::systemBus->async_method_call(
        std::move(getPCIeDeviceCallback), service, escapedPath,
        "org.freedesktop.DBus.Properties", "GetAll", deviceIntf);
}

inline void requestRoutesSystemPCIeDeviceCollection(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/PCIeDevices/")
        .privileges(redfish::privileges::getPCIeDeviceCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)

            {
                asyncResp->res.jsonValue = {
                    {"@odata.type",
                     "#PCIeDeviceCollection.PCIeDeviceCollection"},
                    {"@odata.id", "/redfish/v1/Systems/system/PCIeDevices"},
                    {"Name", "PCIe Device Collection"},
                    {"Description", "Collection of PCIe Devices"},
                    {"Members", nlohmann::json::array()},
                    {"Members@odata.count", 0}};
                getPCIeDeviceList(asyncResp, "Members");
            });
}

inline void requestRoutesSystemPCIeDevice(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/PCIeDevices/<str>/")
        .privileges(redfish::privileges::getPCIeDevice)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& device)

            {
                asyncResp->res.jsonValue = {
                    {"@odata.type", "#PCIeDevice.v1_4_0.PCIeDevice"},
                    {"@odata.id",
                     "/redfish/v1/Systems/system/PCIeDevices/" + device},
                    {"Name", "PCIe Device"},
                    {"Id", device},
                    {"PCIeFunctions",
                     {{"@odata.id", "/redfish/v1/Systems/system/PCIeDevices/" +
                                        device + "/PCIeFunctions"}}}};
                getPCIeDevice(asyncResp, device);
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
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& device)

            {
                asyncResp->res.jsonValue = {
                    {"@odata.type",
                     "#PCIeFunctionCollection.PCIeFunctionCollection"},
                    {"@odata.id", "/redfish/v1/Systems/system/PCIeDevices/" +
                                      device + "/PCIeFunctions"},
                    {"Name", "PCIe Function Collection"},
                    {"Description",
                     "Collection of PCIe Functions for PCIe Device " + device}};
                getPCIeDeviceFunctionsList(asyncResp, device);
            });
}

inline void requestRoutesSystemPCIeFunction(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/system/PCIeDevices/<str>/PCIeFunctions/<str>/")
        .privileges(redfish::privileges::getPCIeFunction)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& device, const std::string& function) {
                asyncResp->res.jsonValue = {
                    {"@odata.type", "#PCIeFunction.v1_2_0.PCIeFunction"},
                    {"@odata.id", "/redfish/v1/Systems/system/PCIeDevices/" +
                                      device + "/PCIeFunctions/" + function},
                    {"Name", "PCIe Function"},
                    {"Id", function},
                    {"FunctionId", std::stoi(function)},
                    {"Links",
                     {{"PCIeDevice",
                       {{"@odata.id",
                         "/redfish/v1/Systems/system/PCIeDevices/" +
                             device}}}}}};
                getPCIeDeviceFunction(asyncResp, device, function);
            });
}

inline void requestRoutesChassisPCIeDeviceCollection(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/PCIeDevices/")
        .privileges({{"Login"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisId)

            {
                const std::string& chassisPCIePath =
                    "/xyz/openbmc_project/inventory/system/chassis/" +
                    chassisId + "/PCIeDevices";
                asyncResp->res.jsonValue = {
                    {"@odata.type",
                     "#PCIeDeviceCollection.PCIeDeviceCollection"},
                    {"@odata.id",
                     "/redfish/v1/Chassis/" + chassisId + "/PCIeDevices"},
                    {"Name", "PCIe Device Collection"},
                    {"Description", "Collection of PCIe Devices"},
                    {"Members", nlohmann::json::array()},
                    {"Members@odata.count", 0}};
                getPCIeDeviceList(asyncResp, "Members", chassisPCIePath,
                                  chassisId);
            });
}

inline void requestRoutesChassisPCIeDevice(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/PCIeDevices/<str>/")
        .privileges({{"Login"}})
        .methods(
            boost::beast::http::verb::get)([](const crow::Request&,
                                              const std::shared_ptr<
                                                  bmcweb::AsyncResp>& asyncResp,
                                              const std::string& chassisId,
                                              const std::string& device) {
            const std::string& chassisPCIePath =
                "/xyz/openbmc_project/inventory/system/chassis/" + chassisId +
                "/PCIeDevices";
            const std::string& chassisPCIeDevicePath =
                chassisPCIePath + "/" + device;
            const std::array<const char*, 1> interface = {
                "xyz.openbmc_project.Inventory.Item.PCIeDevice"};
            // Get Inventory Service
            crow::connections::systemBus->async_method_call(
                [asyncResp, device, chassisPCIePath, interface, chassisId,
                 chassisPCIeDevicePath](
                    const boost::system::error_code ec,
                    const crow::openbmc_mapper::GetSubTreeType& subtree) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG << "DBUS response error";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    // Iterate over all retrieved ObjectPaths.
                    for (const std::pair<
                             std::string,
                             std::vector<std::pair<std::string,
                                                   std::vector<std::string>>>>&
                             object : subtree)
                    {
                        if (object.first != chassisPCIeDevicePath)
                        {
                            continue;
                        }
                        const std::vector<
                            std::pair<std::string, std::vector<std::string>>>&
                            connectionNames = object.second;
                        if (connectionNames.size() < 1)
                        {
                            BMCWEB_LOG_ERROR << "Got 0 Connection names";
                            continue;
                        }
                        std::string pcieDeviceURI = "/redfish/v1/Chassis/";
                        pcieDeviceURI += chassisId;
                        pcieDeviceURI += "/PCIeDevices/";
                        pcieDeviceURI += device;
                        std::string pcieFunctionURI = pcieDeviceURI;
                        pcieFunctionURI += "/PCIeFunctions/";
                        asyncResp->res.jsonValue = {
                            {"@odata.type", "#PCIeDevice.v1_5_0.PCIeDevice"},
                            {"@odata.id", pcieDeviceURI},
                            {"Name", "PCIe Device"},
                            {"Id", device},
                            {"PCIeFunctions",
                             {{"@odata.id", pcieFunctionURI}}}};
                        const std::string& connectionName =
                            connectionNames[0].first;
                        const std::vector<std::string>& interfaces2 =
                            connectionNames[0].second;
                        getPCIeDevice(asyncResp, device, chassisPCIePath,
                                      connectionName, interface[0]);
                        // Get asset properties
                        if (std::find(interfaces2.begin(), interfaces2.end(),
                                      assetInterface) != interfaces2.end())
                        {
                            getPCIeDeviceAssetData(asyncResp, device,
                                                   chassisPCIePath,
                                                   connectionName);
                        }
                        // Get UUID
                        if (std::find(interfaces2.begin(), interfaces2.end(),
                                      uuidInterface) != interfaces2.end())
                        {
                            getPCIeDeviceUUID(asyncResp, device,
                                              chassisPCIePath, connectionName);
                        }
                        // Device state
                        if (std::find(interfaces2.begin(), interfaces2.end(),
                                      stateInterface) != interfaces2.end())
                        {
                            getPCIeDeviceState(asyncResp, device,
                                               chassisPCIePath, connectionName);
                        }
                    }
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                "/xyz/openbmc_project/inventory", 0, interface);
        });
}

inline void requestRoutesChassisPCIeFunctionCollection(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Chassis/<str>/PCIeDevices/<str>/PCIeFunctions/")
        .privileges({{"Login"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisId, const std::string& device) {
                const std::string& chassisPCIePath =
                    "/xyz/openbmc_project/inventory/system/chassis/" +
                    chassisId + "/PCIeDevices";
                const std::string& chassisPCIeDevicePath =
                    chassisPCIePath + "/" + device;
                const std::array<const char*, 1> interface = {
                    "xyz.openbmc_project.Inventory.Item.PCIeDevice"};
                // Response
                std::string pcieFunctionURI = "/redfish/v1/Chassis/";
                pcieFunctionURI += chassisId;
                pcieFunctionURI += "/PCIeDevices/";
                pcieFunctionURI += device;
                pcieFunctionURI += "/PCIeFunctions/";
                asyncResp->res.jsonValue = {
                    {"@odata.type",
                     "#PCIeFunctionCollection.PCIeFunctionCollection"},
                    {"@odata.id", pcieFunctionURI},
                    {"Name", "PCIe Function Collection"},
                    {"Description",
                     "Collection of PCIe Functions for PCIe Device " + device}};
                // Get Inventory Service
                crow::connections::systemBus->async_method_call(
                    [asyncResp, device, chassisPCIePath, interface, chassisId,
                     chassisPCIeDevicePath](
                        const boost::system::error_code ec,
                        const crow::openbmc_mapper::GetSubTreeType& subtree) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG << "DBUS response error";
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        // Iterate over all retrieved ObjectPaths.
                        for (const std::pair<
                                 std::string,
                                 std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>&
                                 object : subtree)
                        {
                            if (object.first != chassisPCIeDevicePath)
                            {
                                continue;
                            }
                            const std::vector<std::pair<
                                std::string, std::vector<std::string>>>&
                                connectionNames = object.second;
                            if (connectionNames.size() < 1)
                            {
                                BMCWEB_LOG_ERROR << "Got 0 Connection names";
                                continue;
                            }
                            const std::string& connectionName =
                                connectionNames[0].first;
                            getPCIeDeviceFunctionsList(
                                asyncResp, device, chassisPCIePath,
                                connectionName, interface[0], chassisId);
                        }
                    },
                    "xyz.openbmc_project.ObjectMapper",
                    "/xyz/openbmc_project/object_mapper",
                    "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                    "/xyz/openbmc_project/inventory", 0, interface);
            });
}

inline void requestRoutesChassisPCIeFunction(App& app)
{
    BMCWEB_ROUTE(
        app, "/redfish/v1/Chassis/<str>/PCIeDevices/<str>/PCIeFunctions/<str>/")
        .privileges({{"Login"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisId, const std::string& device,
               const std::string& function) {
                const std::string& chassisPCIePath =
                    "/xyz/openbmc_project/inventory/system/chassis/" +
                    chassisId + "/PCIeDevices";
                const std::string& chassisPCIeDevicePath =
                    chassisPCIePath + "/" + device;
                const std::array<const char*, 1> interface = {
                    "xyz.openbmc_project.Inventory.Item.PCIeDevice"};
                // Get Inventory Service
                crow::connections::systemBus->async_method_call(
                    [asyncResp, device, function, chassisPCIePath, interface,
                     chassisId, chassisPCIeDevicePath](
                        const boost::system::error_code ec,
                        const crow::openbmc_mapper::GetSubTreeType& subtree) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG << "DBUS response error";
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        // Iterate over all retrieved ObjectPaths.
                        for (const std::pair<
                                 std::string,
                                 std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>&
                                 object : subtree)
                        {
                            if (object.first != chassisPCIeDevicePath)
                            {
                                continue;
                            }
                            std::string pcieDeviceURI = "/redfish/v1/Chassis/";
                            pcieDeviceURI += chassisId;
                            pcieDeviceURI += "/PCIeDevices/";
                            pcieDeviceURI += device;
                            std::string pcieFunctionURI = pcieDeviceURI;
                            pcieFunctionURI += "/PCIeFunctions/";
                            pcieFunctionURI += function;

                            asyncResp->res.jsonValue = {
                                {"@odata.type",
                                 "#PCIeFunction.v1_2_0.PCIeFunction"},
                                {"@odata.id", pcieFunctionURI},
                                {"Name", "PCIe Function"},
                                {"Id", function},
                                {"FunctionId", std::stoi(function)},
                                {"Links",
                                 {{"PCIeDevice",
                                   {{"@odata.id", pcieDeviceURI}}}}}};
                            const std::vector<std::pair<
                                std::string, std::vector<std::string>>>&
                                connectionNames = object.second;
                            if (connectionNames.size() < 1)
                            {
                                BMCWEB_LOG_ERROR << "Got 0 Connection names";
                                continue;
                            }
                            const std::string& connectionName =
                                connectionNames[0].first;
                            getPCIeDeviceFunction(asyncResp, device, function,
                                                  chassisPCIePath,
                                                  connectionName, interface[0]);
                        }
                    },
                    "xyz.openbmc_project.ObjectMapper",
                    "/xyz/openbmc_project/object_mapper",
                    "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                    "/xyz/openbmc_project/inventory", 0, interface);
            });
}

} // namespace redfish
