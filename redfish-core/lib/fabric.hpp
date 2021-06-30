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
 * @param[in,out]   aResp   Async HTTP response.
 * @param[in]       objPath     D-Bus object to query.
 * @param[in]       fabricId    fabric id for redfish URI.
 */
inline void updatePortLinks(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                            const std::string& objPath,
                            const std::string& fabricId)
{
    BMCWEB_LOG_DEBUG << "Get Port Links";
    crow::connections::systemBus->async_method_call(
        [aResp, fabricId](const boost::system::error_code ec,
                          std::variant<std::vector<std::string>>& resp) {
            if (ec)
            {
                return; // no endpoint = no failures
            }
            std::vector<std::string>* data =
                std::get_if<std::vector<std::string>>(&resp);
            if (data == nullptr)
            {
                return;
            }
            nlohmann::json& linksArray =
                aResp->res.jsonValue["Links"]["AssociatedEndpoints"];
            linksArray = nlohmann::json::array();
            for (const std::string& portPath : *data)
            {
                sdbusplus::message::object_path objPath(portPath);
                const std::string& endpointId = objPath.filename();
                std::string endpointURI = "/redfish/v1/Fabrics/";
                endpointURI += fabricId;
                endpointURI += "/Endpoints/";
                endpointURI += endpointId;
                linksArray.push_back({{"@odata.id", endpointURI}});
            }
        },
        "xyz.openbmc_project.ObjectMapper", objPath + "/associated_endpoint",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Association", "endpoints");
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
                updatePortLinks(asyncResp, path, fabricId);
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
 * @brief Fill out links for parent chassis PCIeDevice by
 * requesting data from the given D-Bus association object.
 *
 * @param[in,out]   aResp       Async HTTP response.
 * @param[in]       objPath     D-Bus object to query.
 * @param[in]       chassisName D-Bus object chassisName.
 */
inline void getSwitchParentChassisPCIeDeviceLink(
    std::shared_ptr<bmcweb::AsyncResp> aResp, const std::string& objPath,
    const std::string& chassisName)
{
    crow::connections::systemBus->async_method_call(
        [aResp{std::move(aResp)},
         chassisName](const boost::system::error_code ec,
                      std::variant<std::vector<std::string>>& resp) {
            if (ec)
            {
                return; // no chassis = no failures
            }
            std::vector<std::string>* data =
                std::get_if<std::vector<std::string>>(&resp);
            if (data == nullptr && data->size() > 1)
            {
                // Chassis must have single parent chassis
                return;
            }
            const std::string& parentChassisPath = data->front();
            sdbusplus::message::object_path objectPath(parentChassisPath);
            std::string parentChassisName = objectPath.filename();
            if (parentChassisName.empty())
            {
                messages::internalError(aResp->res);
                return;
            }
            crow::connections::systemBus->async_method_call(
                [aResp, chassisName, parentChassisName](
                    const boost::system::error_code ec,
                    const crow::openbmc_mapper::GetSubTreeType& subtree) {
                    if (ec)
                    {
                        messages::internalError(aResp->res);
                        return;
                    }
                    for (const auto& [objectPath, serviceMap] : subtree)
                    {
                        // Process same device
                        if (!boost::ends_with(objectPath, chassisName))
                        {
                            continue;
                        }
                        std::string pcieDeviceLink = "/redfish/v1/Chassis/";
                        pcieDeviceLink += parentChassisName;
                        pcieDeviceLink += "/PCIeDevices/";
                        pcieDeviceLink += chassisName;
                        aResp->res.jsonValue["Links"]["PCIeDevice"] = {
                            {"@odata.id", pcieDeviceLink}};
                    }
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                parentChassisPath, 0,
                std::array<const char*, 1>{"xyz.openbmc_project.Inventory.Item."
                                           "PCIeDevice"});
        },
        "xyz.openbmc_project.ObjectMapper", objPath + "/parent_chassis",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Association", "endpoints");
}

/**
 * @brief Fill out links association to parent chassis by
 * requesting data from the given D-Bus association object.
 *
 * @param[in,out]   aResp       Async HTTP response.
 * @param[in]       objPath     D-Bus object to query.
 */
inline void getSwitchChassisLink(std::shared_ptr<bmcweb::AsyncResp> aResp,
                                 const std::string& objPath)
{
    BMCWEB_LOG_DEBUG << "Get parent chassis link";
    crow::connections::systemBus->async_method_call(
        [aResp{std::move(aResp)},
         objPath](const boost::system::error_code ec,
                  std::variant<std::vector<std::string>>& resp) {
            if (ec)
            {
                return; // no chassis = no failures
            }
            std::vector<std::string>* data =
                std::get_if<std::vector<std::string>>(&resp);
            if (data == nullptr && data->size() > 1)
            {
                // Switch must have single parent chassis
                return;
            }
            const std::string& chassisPath = data->front();
            sdbusplus::message::object_path objectPath(chassisPath);
            std::string chassisName = objectPath.filename();
            if (chassisName.empty())
            {
                messages::internalError(aResp->res);
                return;
            }
            aResp->res.jsonValue["Links"]["Chassis"] = {
                {"@odata.id", "/redfish/v1/Chassis/" + chassisName}};

            // Check if PCIeDevice on this chassis
            crow::connections::systemBus->async_method_call(
                [aResp, chassisName, chassisPath](
                    const boost::system::error_code ec,
                    const crow::openbmc_mapper::GetSubTreeType& subtree) {
                    if (ec)
                    {
                        messages::internalError(aResp->res);
                        return;
                    }
                    // If PCIeDevice doesn't exists on this chassis
                    // Check PCIeDevice on its parent chassis
                    if (subtree.empty())
                    {
                        getSwitchParentChassisPCIeDeviceLink(aResp, chassisPath,
                                                             chassisName);
                    }
                    else
                    {
                        for (const auto& [objectPath, serviceMap] : subtree)
                        {
                            // Process same device
                            if (!boost::ends_with(objectPath, chassisName))
                            {
                                continue;
                            }
                            std::string pcieDeviceLink = "/redfish/v1/Chassis/";
                            pcieDeviceLink += chassisName;
                            pcieDeviceLink += "/PCIeDevices/";
                            pcieDeviceLink += chassisName;
                            aResp->res.jsonValue["Links"]["PCIeDevice"] = {
                                {"@odata.id", pcieDeviceLink}};
                        }
                    }
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree", chassisPath,
                0,
                std::array<const char*, 1>{
                    "xyz.openbmc_project.Inventory.Item.PCIeDevice"});
        },
        "xyz.openbmc_project.ObjectMapper", objPath + "/parent_chassis",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Association", "endpoints");
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
                                    // Link association to parent chassis
                                    getSwitchChassisLink(asyncResp, path);
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

/**
 * Endpoint derived class for delivering Endpoint Collection Schema
 */
inline void requestRoutesEndpointCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Fabrics/<str>/Endpoints/")
        .privileges({{"Login"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& fabricId) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#EndpointCollection.EndpointCollection";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Fabrics/" + fabricId + "/Endpoints";
                asyncResp->res.jsonValue["Name"] = "Endpoint Collection";

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
                                "/redfish/v1/Fabrics/" + fabricId +
                                    "/Endpoints",
                                {"xyz.openbmc_project.Inventory.Item.Endpoint"},
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
 * @brief Get all endpoint pcie device info by requesting data
 * from the given D-Bus object.
 *
 * @param[in,out]   aResp   Async HTTP response.
 * @param[in]       service     D-Bus service to query.
 * @param[in]       objPath     D-Bus object to query.
 * @param[in]       entityLink  redfish entity link.
 */
inline void getProcessorPCIeDeviceData(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp, const std::string& service,
    const std::string& objPath, const std::string& entityLink)
{
    crow::connections::systemBus->async_method_call(
        [aResp, entityLink](
            const boost::system::error_code ec,
            const boost::container::flat_map<
                std::string, std::variant<std::string>>& pcieDevProperties) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }
            // Get the device data from single function
            const std::string& function = "0";
            std::string deviceId, vendorId, subsystemId, subsystemVendorId;
            for (const auto& property : pcieDevProperties)
            {
                const std::string& propertyName = property.first;
                if (propertyName == "Function" + function + "DeviceId")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value != nullptr)
                    {
                        deviceId = *value;
                    }
                }
                else if (propertyName == "Function" + function + "VendorId")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value != nullptr)
                    {
                        vendorId = *value;
                    }
                }
                else if (propertyName == "Function" + function + "SubsystemId")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value != nullptr)
                    {
                        subsystemId = *value;
                    }
                }
                else if (propertyName ==
                         "Function" + function + "SubsystemVendorId")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value != nullptr)
                    {
                        subsystemVendorId = *value;
                    }
                }
            }
            nlohmann::json& connectedEntitiesArray =
                aResp->res.jsonValue["ConnectedEntities"];
            connectedEntitiesArray.push_back(
                {{"EntityType", "Processor"},
                 {"EntityPciId",
                  {{"DeviceId", deviceId},
                   {"VendorId", vendorId},
                   {"SubsystemId", subsystemId},
                   {"SubsystemVendorId", subsystemVendorId}}},
                 {"EntityLink", {{"@odata.id", entityLink}}}});
        },
        service, objPath, "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Inventory.Item.PCIeDevice");
}

/**
 * @brief Fill out links for parent chassis PCIeDevice by
 * requesting data from the given D-Bus association object.
 *
 * @param[in,out]   aResp       Async HTTP response.
 * @param[in]       objPath     D-Bus object to query.
 * @param[in]       chassisName D-Bus object chassisName.
 * @param[in]       entityLink  redfish entity link.
 */
inline void getProcessorParentEndpointData(
    std::shared_ptr<bmcweb::AsyncResp> aResp, const std::string& objPath,
    const std::string& chassisName, const std::string& entityLink)
{
    crow::connections::systemBus->async_method_call(
        [aResp{std::move(aResp)}, chassisName,
         entityLink](const boost::system::error_code ec,
                     std::variant<std::vector<std::string>>& resp) {
            if (ec)
            {
                return; // no chassis = no failures
            }
            std::vector<std::string>* data =
                std::get_if<std::vector<std::string>>(&resp);
            if (data == nullptr && data->size() > 1)
            {
                // Chassis must have single parent chassis
                return;
            }
            const std::string& parentChassisPath = data->front();
            sdbusplus::message::object_path objectPath(parentChassisPath);
            std::string parentChassisName = objectPath.filename();
            if (parentChassisName.empty())
            {
                messages::internalError(aResp->res);
                return;
            }
            crow::connections::systemBus->async_method_call(
                [aResp, chassisName, parentChassisName, entityLink](
                    const boost::system::error_code ec,
                    const crow::openbmc_mapper::GetSubTreeType& subtree) {
                    if (ec)
                    {
                        messages::internalError(aResp->res);
                        return;
                    }
                    for (const auto& [objectPath, serviceMap] : subtree)
                    {
                        // Process same device
                        if (!boost::ends_with(objectPath, chassisName))
                        {
                            continue;
                        }
                        if (serviceMap.size() < 1)
                        {
                            BMCWEB_LOG_ERROR << "Got 0 service "
                                                "names";
                            messages::internalError(aResp->res);
                            return;
                        }
                        const std::string& serviceName = serviceMap[0].first;
                        // Get PCIeDevice Data
                        getProcessorPCIeDeviceData(aResp, serviceName,
                                                   objectPath, entityLink);
                    }
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                parentChassisPath, 0,
                std::array<const char*, 1>{"xyz.openbmc_project.Inventory.Item."
                                           "PCIeDevice"});
        },
        "xyz.openbmc_project.ObjectMapper", objPath + "/parent_chassis",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Association", "endpoints");
}

/**
 * @brief Get all endpoint pcie device info by requesting data
 * from the given D-Bus object.
 *
 * @param[in,out]   aResp       Async HTTP response.
 * @param[in]       objPath     D-Bus service to query.
 * @param[in]       entityLink  redfish entity link.
 */
inline void
    getProcessorEndpointData(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             const std::string& objPath,
                             const std::string& entityLink)
{
    BMCWEB_LOG_DEBUG << "Get processor endpoint data";
    crow::connections::systemBus->async_method_call(
        [aResp, objPath,
         entityLink](const boost::system::error_code ec,
                     std::variant<std::vector<std::string>>& resp) {
            if (ec)
            {
                return; // no chassis = no failures
            }
            std::vector<std::string>* data =
                std::get_if<std::vector<std::string>>(&resp);
            if (data == nullptr && data->size() > 1)
            {
                // Processor must have single parent chassis
                return;
            }
            const std::string& chassisPath = data->front();
            sdbusplus::message::object_path objectPath(chassisPath);
            std::string chassisName = objectPath.filename();
            if (chassisName.empty())
            {
                messages::internalError(aResp->res);
                return;
            }
            // Check if PCIeDevice on this chassis
            crow::connections::systemBus->async_method_call(
                [aResp, chassisName, chassisPath, entityLink](
                    const boost::system::error_code ec,
                    const crow::openbmc_mapper::GetSubTreeType& subtree) {
                    if (ec)
                    {
                        messages::internalError(aResp->res);
                        return;
                    }
                    // If PCIeDevice doesn't exists on this chassis
                    // Check PCIeDevice on its parent chassis
                    if (subtree.empty())
                    {
                        getProcessorParentEndpointData(aResp, chassisPath,
                                                       chassisName, entityLink);
                    }
                    else
                    {
                        for (const auto& [objectPath, serviceMap] : subtree)
                        {
                            // Process same device
                            if (!boost::ends_with(objectPath, chassisName))
                            {
                                continue;
                            }
                            if (serviceMap.size() < 1)
                            {
                                BMCWEB_LOG_ERROR << "Got 0 service names";
                                messages::internalError(aResp->res);
                                return;
                            }
                            const std::string& serviceName =
                                serviceMap[0].first;
                            // Get PCIeDevice Data
                            getProcessorPCIeDeviceData(aResp, serviceName,
                                                       objectPath, entityLink);
                        }
                    }
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree", chassisPath,
                0,
                std::array<const char*, 1>{
                    "xyz.openbmc_project.Inventory.Item.PCIeDevice"});
        },
        "xyz.openbmc_project.ObjectMapper", objPath + "/parent_chassis",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Association", "endpoints");
}

/**
 * @brief Get all endpoint pcie device info by requesting data
 * from the given D-Bus object.
 *
 * @param[in,out]   aResp   Async HTTP response.
 * @param[in]       service     D-Bus service to query.
 * @param[in]       objPath     D-Bus object to query.
 */
inline void getPortData(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                        const std::string& service, const std::string& objPath)
{
    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                const boost::container::flat_map<
                    std::string, std::variant<std::string, bool, size_t,
                                              std::vector<std::string>>>&
                    properties) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }
            // Get port protocol
            for (const auto& property : properties)
            {
                if (property.first == "Protocol")
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value == nullptr)
                    {
                        BMCWEB_LOG_DEBUG << "Null value returned "
                                            "for protocol type";
                        messages::internalError(aResp->res);
                        return;
                    }
                    aResp->res.jsonValue["EndpointProtocol"] =
                        getPortProtocol(*value);
                }
            }
        },
        service, objPath, "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Inventory.Item.Port");
}

/**
 * @brief Get all endpoint info by requesting data
 * from the given D-Bus object.
 *
 * @param[in,out]   aResp   Async HTTP response.
 * @param[in]       objPath     D-Bus object to query.
 */
inline void updateEndpointData(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                               const std::string& objPath)
{
    BMCWEB_LOG_DEBUG << "Get Endpoint Data";
    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                std::variant<std::vector<std::string>>& resp) {
            if (ec)
            {
                return; // no entity link = no failures
            }
            std::vector<std::string>* data =
                std::get_if<std::vector<std::string>>(&resp);
            if (data == nullptr)
            {
                return;
            }
            for (const std::string& entityPath : *data)
            {
                // Get subtree for entity link parent path
                size_t separator = entityPath.rfind('/');
                if (separator == std::string::npos)
                {
                    BMCWEB_LOG_ERROR << "Invalid entity link path";
                    continue;
                }
                std::string entityInventoryPath =
                    entityPath.substr(0, separator);
                // Get entity subtree
                crow::connections::systemBus->async_method_call(
                    [aResp, entityPath](
                        const boost::system::error_code ec,
                        const crow::openbmc_mapper::GetSubTreeType& subtree) {
                        if (ec)
                        {
                            messages::internalError(aResp->res);
                            return;
                        }
                        // Iterate over all retrieved ObjectPaths.
                        for (const std::pair<
                                 std::string,
                                 std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>&
                                 object : subtree)
                        {
                            // Filter entity link object
                            if (object.first != entityPath)
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
                            const std::vector<std::string>& interfaces =
                                connectionNames[0].second;
                            const std::string acceleratorInterface =
                                "xyz.openbmc_project.Inventory.Item."
                                "Accelerator";
                            if (std::find(interfaces.begin(), interfaces.end(),
                                          acceleratorInterface) !=
                                interfaces.end())
                            {
                                sdbusplus::message::object_path objectPath(
                                    entityPath);
                                const std::string& entityLink =
                                    "/redfish/v1/Systems/system/Processors/" +
                                    objectPath.filename();
                                // Get processor PCIe device data
                                getProcessorEndpointData(aResp, entityPath,
                                                         entityLink);
                            }
                        }
                    },
                    "xyz.openbmc_project.ObjectMapper",
                    "/xyz/openbmc_project/object_mapper",
                    "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                    entityInventoryPath, 0, std::array<const char*, 0>());
            }
        },
        "xyz.openbmc_project.ObjectMapper", objPath + "/entity_link",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Association", "endpoints");
    // Endpoint protocol
    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                std::variant<std::vector<std::string>>& resp) {
            if (ec)
            {
                return; // no endpoint port = no failures
            }
            std::vector<std::string>* data =
                std::get_if<std::vector<std::string>>(&resp);
            if (data == nullptr)
            {
                return;
            }
            for (const std::string& portPath : *data)
            {
                // Get subtree for port parent path
                size_t separator = portPath.rfind('/');
                if (separator == std::string::npos)
                {
                    BMCWEB_LOG_ERROR << "Invalid port link path";
                    continue;
                }
                std::string portInventoryPath = portPath.substr(0, separator);
                // Get port subtree
                crow::connections::systemBus->async_method_call(
                    [aResp, portPath](
                        const boost::system::error_code ec,
                        const crow::openbmc_mapper::GetSubTreeType& subtree) {
                        if (ec)
                        {
                            messages::internalError(aResp->res);
                            return;
                        }
                        // Iterate over all retrieved ObjectPaths.
                        for (const std::pair<
                                 std::string,
                                 std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>&
                                 object : subtree)
                        {
                            // Filter port link object
                            if (object.first != portPath)
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
                            const std::vector<std::string>& interfaces =
                                connectionNames[0].second;
                            const std::string portInterface =
                                "xyz.openbmc_project.Inventory.Item.Port";
                            if (std::find(interfaces.begin(), interfaces.end(),
                                          portInterface) != interfaces.end())
                            {
                                // Get port protocol data
                                getPortData(aResp, connectionName, portPath);
                            }
                        }
                    },
                    "xyz.openbmc_project.ObjectMapper",
                    "/xyz/openbmc_project/object_mapper",
                    "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                    portInventoryPath, 0, std::array<const char*, 0>());
            }
        },
        "xyz.openbmc_project.ObjectMapper", objPath + "/connected_port",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Association", "endpoints");
}

/**
 * Endpoint override class for delivering Endpoint Schema
 */
inline void requestRoutesEndpoint(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Fabrics/<str>/Endpoints/<str>")
        .privileges({{"Login"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& fabricId, const std::string& endpointId) {
                crow::connections::systemBus->async_method_call(
                    [asyncResp, fabricId,
                     endpointId](const boost::system::error_code ec,
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
                                [asyncResp, fabricId, endpointId](
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
                                        // Get the endpointId object
                                        const std::string& path = object.first;
                                        sdbusplus::message::object_path objPath(
                                            path);
                                        if (objPath.filename() != endpointId)
                                        {
                                            continue;
                                        }
                                        std::string endpointURI =
                                            "/redfish/v1/Fabrics/";
                                        endpointURI += fabricId;
                                        endpointURI += "/Endpoints/";
                                        endpointURI += endpointId;
                                        asyncResp->res
                                            .jsonValue["@odata.type"] =
                                            "#Endpoint.v1_6_0.Endpoint";
                                        asyncResp->res.jsonValue["@odata.id"] =
                                            endpointURI;
                                        asyncResp->res.jsonValue["Id"] =
                                            endpointId;
                                        asyncResp->res.jsonValue["Name"] =
                                            endpointId + " Endpoint Resource";
                                        nlohmann::json& connectedEntitiesArray =
                                            asyncResp->res
                                                .jsonValue["ConnectedEntities"];
                                        connectedEntitiesArray =
                                            nlohmann::json::array();
                                        updateEndpointData(asyncResp, path);
                                    }
                                },
                                "xyz.openbmc_project.ObjectMapper",
                                "/xyz/openbmc_project/object_mapper",
                                "xyz.openbmc_project.ObjectMapper",
                                "GetSubTree", object, 0,
                                std::array<const char*, 1>{
                                    "xyz.openbmc_project.Inventory.Item."
                                    "Endpoint"});
                            return;
                        }
                        // Couldn't find an object with that name. Return an
                        // error
                        messages::resourceNotFound(asyncResp->res,
                                                   "#Endpoint.v1_6_0.Endpoint",
                                                   endpointId);
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
