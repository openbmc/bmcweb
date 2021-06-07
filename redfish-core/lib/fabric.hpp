/*
// Copyright (c) 2021, NVIDIA Corporation
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
#include <utils/collection.hpp>

#include <variant>

namespace redfish
{

inline std::string getLinkStates(const std::string& linkState)
{
    if (linkState ==
        "xyz.openbmc_project.Inventory.Item.Port.LinkStates.Enabled")
    {
        return "Enabled";
    }
    if (linkState ==
        "xyz.openbmc_project.Inventory.Item.Port.LinkStates.Disabled")
    {
        return "Disabled";
    }
    // Unknown or others
    return std::string();
}

inline std::string getLinkStatusType(const std::string& linkStatusType)
{
    if (linkStatusType ==
        "xyz.openbmc_project.Inventory.Item.Port.LinkStatusType.LinkDown")
    {
        return "LinkDown";
    }
    if (linkStatusType ==
        "xyz.openbmc_project.Inventory.Item.Port.LinkStatusType.LinkUp")
    {
        return "LinkUp";
    }
    if (linkStatusType ==
        "xyz.openbmc_project.Inventory.Item.Port.LinkStatusType.NoLink")
    {
        return "NoLink";
    }
    if (linkStatusType ==
        "xyz.openbmc_project.Inventory.Item.Port.LinkStatusType.Starting")
    {
        return "Starting";
    }
    if (linkStatusType ==
        "xyz.openbmc_project.Inventory.Item.Port.LinkStatusType.Training")
    {
        return "Training";
    }
    // Unknown or others
    return std::string();
}

inline std::string getPortProtocol(const std::string& portProtocol)
{
    if (portProtocol ==
        "xyz.openbmc_project.Inventory.Item.Port.PortProtocol.Ethernet")
    {
        return "Ethernet";
    }
    if (portProtocol ==
        "xyz.openbmc_project.Inventory.Item.Port.PortProtocol.FC")
    {
        return "FC";
    }
    if (portProtocol ==
        "xyz.openbmc_project.Inventory.Item.Port.PortProtocol.NVLink")
    {
        return "NVLink";
    }
    if (portProtocol ==
        "xyz.openbmc_project.Inventory.Item.Port.PortProtocol.OEM")
    {
        return "OEM";
    }
    // Unknown or others
    return std::string();
}

inline std::string getPortType(const std::string& portType)
{
    if (portType ==
        "xyz.openbmc_project.Inventory.Item.Port.PortType.BidirectionalPort")
    {
        return "BidirectionalPort";
    }
    if (portType ==
        "xyz.openbmc_project.Inventory.Item.Port.PortType.DownstreamPort")
    {
        return "DownstreamPort";
    }
    if (portType ==
        "xyz.openbmc_project.Inventory.Item.Port.PortType.InterswitchPort")
    {
        return "InterswitchPort";
    }
    if (portType ==
        "xyz.openbmc_project.Inventory.Item.Port.PortType.ManagementPort")
    {
        return "ManagementPort";
    }
    if (portType ==
        "xyz.openbmc_project.Inventory.Item.Port.PortType.UnconfiguredPort")
    {
        return "UnconfiguredPort";
    }
    if (portType ==
        "xyz.openbmc_project.Inventory.Item.Port.PortType.UpstreamPort")
    {
        return "UpstreamPort";
    }
    // Unknown or others
    return std::string();
}

inline std::string getSwitchType(const std::string& switchType)
{
    if (switchType ==
        "xyz.openbmc_project.Inventory.Item.Switch.SwitchType.Ethernet")
    {
        return "Ethernet";
    }
    if (switchType == "xyz.openbmc_project.Inventory.Item.Switch.SwitchType.FC")
    {
        return "FC";
    }
    if (switchType ==
        "xyz.openbmc_project.Inventory.Item.Switch.SwitchType.NVLink")
    {
        return "NVLink";
    }
    if (switchType ==
        "xyz.openbmc_project.Inventory.Item.Switch.SwitchType.OEM")
    {
        return "OEM";
    }
    // Unknown or others
    return std::string();
}

inline std::string getFabricType(const std::string& fabricType)
{
    if (fabricType ==
        "xyz.openbmc_project.Inventory.Item.Fabric.FabricType.Ethernet")
    {
        return "Ethernet";
    }
    if (fabricType == "xyz.openbmc_project.Inventory.Item.Fabric.FabricType.FC")
    {
        return "FC";
    }
    if (fabricType ==
        "xyz.openbmc_project.Inventory.Item.Fabric.FabricType.NVLink")
    {
        return "NVLink";
    }
    if (fabricType ==
        "xyz.openbmc_project.Inventory.Item.Fabric.FabricType.OEM")
    {
        return "OEM";
    }
    // Unknown or others
    return std::string();
}

/**
 * @brief Get all switch info by requesting data
 * from the given D-Bus object.
 *
 * @param[in,out]   asyncResp   Async HTTP response.
 * @param[in]       service     D-Bus service to query.
 * @param[in]       objPath     D-Bus object to query.
 */
inline void updatePortData(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& service,
                           const std::string& objPath)
{
    BMCWEB_LOG_DEBUG << "Get Port Data";
    using PropertyType =
        std::variant<std::string, bool, size_t, std::vector<std::string>>;
    using PropertiesMap = boost::container::flat_map<std::string, PropertyType>;
    // Get interface properties
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const PropertiesMap& properties) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            for (const auto& property : properties)
            {
                const std::string& propertyName = property.first;
                if (propertyName == "Type")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value == nullptr)
                    {
                        BMCWEB_LOG_DEBUG << "Null value returned "
                                            "for port type";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    asyncResp->res.jsonValue["PortType"] = getPortType(*value);
                }
                else if (propertyName == "Protocol")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value == nullptr)
                    {
                        BMCWEB_LOG_DEBUG << "Null value returned "
                                            "for protocol type";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    asyncResp->res.jsonValue["PortProtocol"] =
                        getPortProtocol(*value);
                }
                else if (propertyName == "LinkStatus")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value == nullptr)
                    {
                        BMCWEB_LOG_DEBUG << "Null value returned "
                                            "for link status";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    asyncResp->res.jsonValue["LinkStatus"] =
                        getLinkStatusType(*value);
                }
                else if (propertyName == "LinkState")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value == nullptr)
                    {
                        BMCWEB_LOG_DEBUG << "Null value returned "
                                            "for link state";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    asyncResp->res.jsonValue["LinkState"] =
                        getLinkStates(*value);
                }
                else if (propertyName == "CurrentSpeed")
                {
                    const size_t* value = std::get_if<size_t>(&property.second);
                    if (value == nullptr)
                    {
                        BMCWEB_LOG_DEBUG << "Null value returned "
                                            "for CurrentSpeed";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    asyncResp->res.jsonValue["CurrentSpeedGbps"] = *value;
                }
                else if (propertyName == "MaxSpeed")
                {
                    const size_t* value = std::get_if<size_t>(&property.second);
                    if (value == nullptr)
                    {
                        BMCWEB_LOG_DEBUG << "Null value returned "
                                            "for MaxSpeed";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    asyncResp->res.jsonValue["MaxSpeedGbps"] = *value;
                }
                else if (propertyName == "Width")
                {
                    const size_t* value = std::get_if<size_t>(&property.second);
                    if (value == nullptr)
                    {
                        BMCWEB_LOG_DEBUG << "Null value returned "
                                            "for Width";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    asyncResp->res.jsonValue[propertyName] = *value;
                }
                else if (propertyName == "CurrentPowerState")
                {
                    const std::string* state =
                        std::get_if<std::string>(&property.second);
                    if (*state ==
                        "xyz.openbmc_project.State.Chassis.PowerState.On")
                    {
                        asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
                        asyncResp->res.jsonValue["Status"]["Health"] = "OK";
                    }
                    else if (*state ==
                             "xyz.openbmc_project.State.Chassis.PowerState.Off")
                    {
                        asyncResp->res.jsonValue["Status"]["State"] =
                            "StandbyOffline";
                        asyncResp->res.jsonValue["Status"]["Health"] =
                            "Critical";
                    }
                }
            }
        },
        service, objPath, "org.freedesktop.DBus.Properties", "GetAll", "");
}

/**
 * @brief Get all port info by requesting data
 * from the given D-Bus object.
 *
 * @param[in,out]   asyncResp   Async HTTP response.
 * @param[in]       fabricId    Fabric Id.
 * @param[in]       switchId    Switch Id.
 * @param[in]       portId      Port Id.
 * @param[in]       objPath     D-Bus object to query.
 */
inline void getPortObject(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& fabricId,
                          const std::string& switchId,
                          const std::string& portId, const std::string& objPath)
{
    BMCWEB_LOG_DEBUG << "Access port Data";
    crow::connections::systemBus->async_method_call(
        [asyncResp, fabricId, switchId,
         portId](const boost::system::error_code ec,
                 const crow::openbmc_mapper::GetSubTreeType& subtree) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            // Iterate over all retrieved
            // ObjectPaths.
            for (const std::pair<std::string,
                                 std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>&
                     object : subtree)
            {
                // Get the portId object
                const std::string& path = object.first;
                const std::vector<
                    std::pair<std::string, std::vector<std::string>>>&
                    connectionNames = object.second;
                sdbusplus::message::object_path objPath(path);
                if (objPath.filename() != portId)
                {
                    continue;
                }
                if (connectionNames.size() < 1)
                {
                    BMCWEB_LOG_ERROR << "Got 0 Connection names";
                    continue;
                }
                std::string portURI = "/redfish/v1/Fabrics/";
                portURI += fabricId;
                portURI += "/Switches/";
                portURI += switchId;
                portURI += "/Ports/";
                portURI += portId;
                asyncResp->res.jsonValue = {
                    {"@odata.type", "#Port.v1_4_0.Port"},
                    {"@odata.id", portURI},
                    {"Name", portId + " Resource"},
                    {"Id", portId}};
                const std::string& connectionName = connectionNames[0].first;
                updatePortData(asyncResp, connectionName, path);
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", objPath, 0,
        std::array<const char*, 1>{"xyz.openbmc_project.Inventory.Item."
                                   "Port"});
}

/**
 * @brief Get all switch info by requesting data
 * from the given D-Bus object.
 *
 * @param[in,out]   asyncResp   Async HTTP response.
 * @param[in]       service     D-Bus service to query.
 * @param[in]       objPath     D-Bus object to query.
 */
inline void
    updateSwitchData(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& service, const std::string& objPath)
{
    BMCWEB_LOG_DEBUG << "Get Switch Data";
    using PropertyType =
        std::variant<std::string, bool, size_t, std::vector<std::string>>;
    using PropertiesMap = boost::container::flat_map<std::string, PropertyType>;
    // Get interface properties
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const PropertiesMap& properties) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            for (const auto& property : properties)
            {
                const std::string& propertyName = property.first;
                if (propertyName == "Type")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value == nullptr)
                    {
                        BMCWEB_LOG_DEBUG << "Null value returned "
                                            "for switch type";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    asyncResp->res.jsonValue["SwitchType"] =
                        getSwitchType(*value);
                }
                else if (propertyName == "SupportedProtocols")
                {
                    nlohmann::json& protoArray =
                        asyncResp->res.jsonValue["SupportedProtocols"];
                    protoArray = nlohmann::json::array();
                    const std::vector<std::string>* protocols =
                        std::get_if<std::vector<std::string>>(&property.second);
                    if (protocols == nullptr)
                    {
                        BMCWEB_LOG_DEBUG << "Null value returned "
                                            "for supported protocols";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    for (const std::string& protocol : *protocols)
                    {
                        protoArray.push_back(getSwitchType(protocol));
                    }
                }
                else if (propertyName == "Enabled")
                {
                    const bool* value = std::get_if<bool>(&property.second);
                    if (value == nullptr)
                    {
                        BMCWEB_LOG_DEBUG << "Null value returned "
                                            "for enabled";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    asyncResp->res.jsonValue["Enabled"] = *value;
                }
                else if ((propertyName == "Model") ||
                         (propertyName == "PartNumber") ||
                         (propertyName == "SerialNumber") ||
                         (propertyName == "Manufacturer"))
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value == nullptr)
                    {
                        BMCWEB_LOG_DEBUG << "Null value returned "
                                            "for asset properties";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    asyncResp->res.jsonValue[propertyName] = *value;
                }
                else if (propertyName == "Version")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value == nullptr)
                    {
                        BMCWEB_LOG_DEBUG << "Null value returned "
                                            "for revision";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    asyncResp->res.jsonValue["FirmwareVersion"] = *value;
                }
                else if (propertyName == "CurrentBandwidth")
                {
                    const size_t* value = std::get_if<size_t>(&property.second);
                    if (value == nullptr)
                    {
                        BMCWEB_LOG_DEBUG << "Null value returned "
                                            "for CurrentBandwidth";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    asyncResp->res.jsonValue["CurrentBandwidthGbps"] = *value;
                }
                else if (propertyName == "MaxBandwidth")
                {
                    const size_t* value = std::get_if<size_t>(&property.second);
                    if (value == nullptr)
                    {
                        BMCWEB_LOG_DEBUG << "Null value returned "
                                            "for MaxBandwidth";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    asyncResp->res.jsonValue["MaxBandwidthGbps"] = *value;
                }
                else if (propertyName == "TotalSwitchWidth")
                {
                    const size_t* value = std::get_if<size_t>(&property.second);
                    if (value == nullptr)
                    {
                        BMCWEB_LOG_DEBUG << "Null value returned "
                                            "for TotalSwitchWidth";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    asyncResp->res.jsonValue["TotalSwitchWidth"] = *value;
                }
                else if (propertyName == "CurrentPowerState")
                {
                    const std::string* state =
                        std::get_if<std::string>(&property.second);
                    if (*state ==
                        "xyz.openbmc_project.State.Chassis.PowerState.On")
                    {
                        asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
                        asyncResp->res.jsonValue["Status"]["Health"] = "OK";
                    }
                    else if (*state ==
                             "xyz.openbmc_project.State.Chassis.PowerState.Off")
                    {
                        asyncResp->res.jsonValue["Status"]["State"] =
                            "StandbyOffline";
                        asyncResp->res.jsonValue["Status"]["Health"] =
                            "Critical";
                    }
                }
            }
        },
        service, objPath, "org.freedesktop.DBus.Properties", "GetAll", "");
}

/**
 * FabricCollection derived class for delivering Fabric Collection Schema
 */
inline void requestRoutesFabricCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Fabrics/")
        .privileges({{"Login"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#FabricCollection.FabricCollection";
                asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Fabrics";
                asyncResp->res.jsonValue["Name"] = "Fabric Collection";

                collection_util::getCollectionMembers(
                    asyncResp, "/redfish/v1/Fabrics",
                    {"xyz.openbmc_project.Inventory.Item.Fabric"});
            });
}

/**
 * Fabric override class for delivering Fabric Schema
 */
inline void requestRoutesFabric(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Fabrics/<str>/")
        .privileges({{"Login"}})
        .methods(
            boost::beast::http::verb::get)([](const crow::Request&,
                                              const std::shared_ptr<
                                                  bmcweb::AsyncResp>& asyncResp,
                                              const std::string& fabricId) {
            crow::connections::systemBus->async_method_call(
                [asyncResp, fabricId(std::string(fabricId))](
                    const boost::system::error_code ec,
                    const crow::openbmc_mapper::GetSubTreeType& subtree) {
                    if (ec)
                    {
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
                        const std::string& path = object.first;
                        const std::vector<
                            std::pair<std::string, std::vector<std::string>>>&
                            connectionNames = object.second;
                        sdbusplus::message::object_path objPath(path);
                        if (objPath.filename() != fabricId)
                        {
                            continue;
                        }
                        if (connectionNames.size() < 1)
                        {
                            BMCWEB_LOG_ERROR << "Got 0 Connection names";
                            continue;
                        }

                        asyncResp->res.jsonValue["@odata.type"] =
                            "#Fabric.v1_2_0.Fabric";
                        asyncResp->res.jsonValue["@odata.id"] =
                            "/redfish/v1/Fabrics/" + fabricId;
                        asyncResp->res.jsonValue["Id"] = fabricId;
                        asyncResp->res.jsonValue["Name"] =
                            fabricId + " Resource";
                        asyncResp->res.jsonValue["Endpoints"] = {
                            {"@odata.id",
                             "/redfish/v1/Fabrics/" + fabricId + "/Endpoints"}};
                        asyncResp->res.jsonValue["Switches"] = {
                            {"@odata.id",
                             "/redfish/v1/Fabrics/" + fabricId + "/Switches"}};

                        const std::string& connectionName =
                            connectionNames[0].first;

                        // Fabric item properties
                        crow::connections::systemBus->async_method_call(
                            [asyncResp](
                                const boost::system::error_code ec,
                                const std::vector<
                                    std::pair<std::string, VariantType>>&
                                    propertiesList) {
                                if (ec)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                for (const std::pair<std::string, VariantType>&
                                         property : propertiesList)
                                {
                                    if (property.first == "Type")
                                    {
                                        const std::string* value =
                                            std::get_if<std::string>(
                                                &property.second);
                                        if (value == nullptr)
                                        {
                                            BMCWEB_LOG_DEBUG
                                                << "Null value returned "
                                                   "for fabric type";
                                            messages::internalError(
                                                asyncResp->res);
                                            return;
                                        }
                                        asyncResp->res.jsonValue["FabricType"] =
                                            getFabricType(*value);
                                    }
                                }
                            },
                            connectionName, path,
                            "org.freedesktop.DBus.Properties", "GetAll",
                            "xyz.openbmc_project.Inventory.Item.Fabric");

                        return;
                    }
                    // Couldn't find an object with that name. Return an error
                    messages::resourceNotFound(
                        asyncResp->res, "#Fabric.v1_2_0.Fabric", fabricId);
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                "/xyz/openbmc_project/inventory", 0,
                std::array<const char*, 1>{
                    "xyz.openbmc_project.Inventory.Item.Fabric"});
        });
}

/**
 * SwitchCollection derived class for delivering Switch Collection Schema
 */
inline void requestRoutesSwitchCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Fabrics/<str>/Switches/")
        .privileges({{"Login"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& fabricId) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#SwitchCollection.SwitchCollection";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Fabrics/" + fabricId + "/Switches";
                asyncResp->res.jsonValue["Name"] = "Switch Collection";

                crow::connections::systemBus->async_method_call(
                    [asyncResp,
                     fabricId](const boost::system::error_code ec,
                               const std::vector<std::string>& objects) {
                        if (ec)
                        {
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        for (const std::string& object : objects)
                        {
                            // Get the fabricId object
                            if (!boost::ends_with(object, fabricId))
                            {
                                continue;
                            }
                            collection_util::getCollectionMembers(
                                asyncResp,
                                "/redfish/v1/Fabrics/" + fabricId + "/Switches",
                                {"xyz.openbmc_project.Inventory.Item.Switch"},
                                object.c_str());
                            return;
                        }
                    },
                    "xyz.openbmc_project.ObjectMapper",
                    "/xyz/openbmc_project/object_mapper",
                    "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
                    "/xyz/openbmc_project/inventory", 0,
                    std::array<const char*, 1>{
                        "xyz.openbmc_project.Inventory.Item.Fabric"});
            });
}

/**
 * Switch override class for delivering Switch Schema
 */
inline void requestRoutesSwitch(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Fabrics/<str>/Switches/<str>")
        .privileges({{"Login"}})
        .methods(
            boost::beast::http::verb::get)([](const crow::Request&,
                                              const std::shared_ptr<
                                                  bmcweb::AsyncResp>& asyncResp,
                                              const std::string& fabricId,
                                              const std::string& switchId) {
            crow::connections::systemBus->async_method_call(
                [asyncResp, fabricId,
                 switchId](const boost::system::error_code ec,
                           const std::vector<std::string>& objects) {
                    if (ec)
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    for (const std::string& object : objects)
                    {
                        // Get the fabricId object
                        if (!boost::ends_with(object, fabricId))
                        {
                            continue;
                        }
                        crow::connections::systemBus->async_method_call(
                            [asyncResp, fabricId, switchId](
                                const boost::system::error_code ec,
                                const crow::openbmc_mapper::GetSubTreeType&
                                    subtree) {
                                if (ec)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                // Iterate over all retrieved ObjectPaths.
                                for (const std::pair<
                                         std::string,
                                         std::vector<std::pair<
                                             std::string,
                                             std::vector<std::string>>>>&
                                         object : subtree)
                                {
                                    // Get the switchId object
                                    const std::string& path = object.first;
                                    const std::vector<std::pair<
                                        std::string, std::vector<std::string>>>&
                                        connectionNames = object.second;
                                    sdbusplus::message::object_path objPath(
                                        path);
                                    if (objPath.filename() != switchId)
                                    {
                                        continue;
                                    }
                                    if (connectionNames.size() < 1)
                                    {
                                        BMCWEB_LOG_ERROR
                                            << "Got 0 Connection names";
                                        continue;
                                    }
                                    std::string switchURI =
                                        "/redfish/v1/Fabrics/";
                                    switchURI += fabricId;
                                    switchURI += "/Switches/";
                                    switchURI += switchId;
                                    std::string portsURI = switchURI;
                                    portsURI += "/Ports";
                                    asyncResp->res.jsonValue["@odata.type"] =
                                        "#Switch.v1_6_0.Switch";
                                    asyncResp->res.jsonValue["@odata.id"] =
                                        switchURI;
                                    asyncResp->res.jsonValue["Id"] = switchId;
                                    asyncResp->res.jsonValue["Name"] =
                                        switchId + " Resource";
                                    asyncResp->res.jsonValue["Ports"] = {
                                        {"@odata.id", portsURI}};
                                    const std::string& connectionName =
                                        connectionNames[0].first;
                                    updateSwitchData(asyncResp, connectionName,
                                                     path);
                                }
                            },
                            "xyz.openbmc_project.ObjectMapper",
                            "/xyz/openbmc_project/object_mapper",
                            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                            object, 0,
                            std::array<const char*, 1>{
                                "xyz.openbmc_project.Inventory.Item.Switch"});
                        return;
                    }
                    // Couldn't find an object with that name. Return an error
                    messages::resourceNotFound(
                        asyncResp->res, "#Switch.v1_6_0.Switch", switchId);
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
                "/xyz/openbmc_project/inventory", 0,
                std::array<const char*, 1>{
                    "xyz.openbmc_project.Inventory.Item.Fabric"});
        });
}

/**
 * PortCollection derived class for delivering Port Collection Schema
 */
inline void requestRoutesPortCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Fabrics/<str>/Switches/<str>/Ports/")
        .privileges({{"Login"}})
        .methods(
            boost::beast::http::verb::get)([](const crow::Request&,
                                              const std::shared_ptr<
                                                  bmcweb::AsyncResp>& asyncResp,
                                              const std::string& fabricId,
                                              const std::string& switchId) {
            std::string portsURI = "/redfish/v1/Fabrics/";
            portsURI += fabricId;
            portsURI += "/Switches/";
            portsURI += switchId;
            portsURI += "/Ports";

            asyncResp->res.jsonValue["@odata.type"] =
                "#PortCollection.PortCollection";
            asyncResp->res.jsonValue["@odata.id"] = portsURI;
            asyncResp->res.jsonValue["Name"] = "Port Collection";

            crow::connections::systemBus->async_method_call(
                [asyncResp, fabricId, switchId,
                 portsURI](const boost::system::error_code ec,
                           const std::vector<std::string>& objects) {
                    if (ec)
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    for (const std::string& object : objects)
                    {
                        // Get the fabricId object
                        if (!boost::ends_with(object, fabricId))
                        {
                            continue;
                        }
                        crow::connections::systemBus->async_method_call(
                            [asyncResp, fabricId, switchId, portsURI](
                                const boost::system::error_code ec,
                                const std::vector<std::string>& objects) {
                                if (ec)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                for (const std::string& object : objects)
                                {
                                    // Get the switchId object
                                    if (!boost::ends_with(object, switchId))
                                    {
                                        continue;
                                    }
                                    collection_util::getCollectionMembers(
                                        asyncResp, portsURI,
                                        {"xyz.openbmc_project.Inventory.Item."
                                         "Port"},
                                        object.c_str());
                                    return;
                                }
                            },
                            "xyz.openbmc_project.ObjectMapper",
                            "/xyz/openbmc_project/object_mapper",
                            "xyz.openbmc_project.ObjectMapper",
                            "GetSubTreePaths", object.c_str(), 0,
                            std::array<const char*, 1>{
                                "xyz.openbmc_project.Inventory.Item.Switch"});
                    }
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
                "/xyz/openbmc_project/inventory", 0,
                std::array<const char*, 1>{
                    "xyz.openbmc_project.Inventory.Item.Fabric"});
        });
}

/**
 * Port override class for delivering Port Schema
 */
inline void requestRoutesPort(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Fabrics/<str>/Switches/<str>/Ports/<str>")
        .privileges({{"Login"}})
        .methods(
            boost::beast::http::verb::get)([](const crow::Request&,
                                              const std::shared_ptr<
                                                  bmcweb::AsyncResp>& asyncResp,
                                              const std::string& fabricId,
                                              const std::string& switchId,
                                              const std::string& portId) {
            crow::connections::systemBus->async_method_call(
                [asyncResp, fabricId, switchId,
                 portId](const boost::system::error_code ec,
                         const std::vector<std::string>& objects) {
                    if (ec)
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    for (const std::string& object : objects)
                    {
                        // Get the fabricId object
                        if (!boost::ends_with(object, fabricId))
                        {
                            continue;
                        }
                        crow::connections::systemBus->async_method_call(
                            [asyncResp, fabricId, switchId,
                             portId](const boost::system::error_code ec,
                                     const std::vector<std::string>& objects) {
                                if (ec)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }

                                for (const std::string& object : objects)
                                {
                                    // Get the switchId object
                                    if (!boost::ends_with(object, switchId))
                                    {
                                        continue;
                                    }
                                    getPortObject(asyncResp, fabricId, switchId,
                                                  portId, object);
                                    return;
                                }
                                // Couldn't find an object with that name.
                                // Return an error
                                messages::resourceNotFound(asyncResp->res,
                                                           "#Port.v1_4_0.Port",
                                                           portId);
                            },
                            "xyz.openbmc_project.ObjectMapper",
                            "/xyz/openbmc_project/object_mapper",
                            "xyz.openbmc_project.ObjectMapper",
                            "GetSubTreePaths", object, 0,
                            std::array<const char*, 1>{
                                "xyz.openbmc_project.Inventory.Item.Switch"});
                    }
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
                "/xyz/openbmc_project/inventory", 0,
                std::array<const char*, 1>{
                    "xyz.openbmc_project.Inventory.Item.Fabric"});
        });
}

} // namespace redfish
