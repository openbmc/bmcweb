#pragma once

#include "assembly.hpp"
#include "dbus_utility.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/fabric_util.hpp"
#include "utils/json_utils.hpp"
#include "utils/pcie_util.hpp"

#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <cmath>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace redfish
{
constexpr std::array<std::string_view, 1> cableInterfaces = {
    "xyz.openbmc_project.Inventory.Item.Cable"};

/**
 * @brief Fill cable specific properties.
 * @param[in,out]   asyncResp       Async HTTP response.
 * @param[in]       properties  List of Cable Properties key/value pairs.
 */
inline void afterFillCableProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const dbus::utility::DBusPropertiesMap& properties)
{
    const std::string* cableTypeDescription = nullptr;
    const double* length = nullptr;
    const std::string* cableStatus = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "CableTypeDescription",
        cableTypeDescription, "Length", length, "CableStatus", cableStatus);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    if (cableTypeDescription != nullptr)
    {
        asyncResp->res.jsonValue["CableType"] = *cableTypeDescription;
    }

    if (length != nullptr)
    {
        if (!std::isfinite(*length))
        {
            // Cable length is NaN by default, do not throw an error
            if (!std::isnan(*length))
            {
                BMCWEB_LOG_ERROR("Cable length value is invalid");
                messages::internalError(asyncResp->res);
                return;
            }
        }
        else
        {
            asyncResp->res.jsonValue["LengthMeters"] = *length;
        }
    }

    if (cableStatus != nullptr && !cableStatus->empty())
    {
        if (*cableStatus ==
            "xyz.openbmc_project.Inventory.Item.Cable.Status.Inactive")
        {
            asyncResp->res.jsonValue["CableStatus"] = "Normal";
            asyncResp->res.jsonValue["Status"]["State"] = "StandbyOffline";
            asyncResp->res.jsonValue["Status"]["Health"] = "OK";
        }
        else if (*cableStatus ==
                 "xyz.openbmc_project.Inventory.Item.Cable.Status.Running")
        {
            asyncResp->res.jsonValue["CableStatus"] = "Normal";
            asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
            asyncResp->res.jsonValue["Status"]["Health"] = "OK";
        }
        else if (*cableStatus ==
                 "xyz.openbmc_project.Inventory.Item.Cable.Status.PoweredOff")
        {
            asyncResp->res.jsonValue["CableStatus"] = "Disabled";
            asyncResp->res.jsonValue["Status"]["State"] = "StandbyOffline";
            asyncResp->res.jsonValue["Status"]["Health"] = "OK";
        }
    }
}

/**
 * @brief Fill cable specific properties.
 * @param[in,out]   asyncResp       Async HTTP response.
 * @param[in]       cableObjectPath Object path of the Cable with association.
 * @param[in]      service - serviceName of cable object
 */
inline void
    fillCableProperties(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& cableObjectPath,
                        const std::string& service)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, cableObjectPath,
        "xyz.openbmc_project.Inventory.Item.Cable",
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::DBusPropertiesMap& properties) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
                messages::internalError(asyncResp->res);
            }
            return;
        }

        afterFillCableProperties(asyncResp, properties);
    });

    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, service, cableObjectPath,
        "xyz.openbmc_project.Inventory.Decorator.Asset", "PartNumber",
        [asyncResp](const boost::system::error_code& ec,
                    const std::string& property) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR("DBus response error for PartNumber, {}",
                                 ec.value());
                messages::internalError(asyncResp->res);
            }

            // PartNumber is optional, ignore the failure if it doesn't exist.
            return;
        }
        asyncResp->res.jsonValue["PartNumber"] = property;
    });
}

inline void
    fillCableHealthState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& cableObjectPath,
                         const std::string& service)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, service, cableObjectPath,
        "xyz.openbmc_project.Inventory.Item", "Present",
        [asyncResp, cableObjectPath](const boost::system::error_code& ec,
                                     bool present) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR(
                    "get presence failed for Cable {} with error {}",
                    cableObjectPath, ec.value());
                messages::internalError(asyncResp->res);
            }
            return;
        }

        if (!present)
        {
            asyncResp->res.jsonValue["Status"]["State"] = "Absent";
        }
    });
}

inline void afterGetCableUpstreamResources(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperEndPoints& endpoints)
{
    if (ec && ec.value() != EBADR)
    {
        BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }
    if (endpoints.empty())
    {
        BMCWEB_LOG_DEBUG("No association found");
        return;
    }

    nlohmann::json::array_t linkArray;
    for (const auto& fullPath : endpoints)
    {
        std::string devName = pcie_util::buildPCIeUniquePath(fullPath);
        if (devName.empty())
        {
            continue;
        }
        nlohmann::json::object_t item;
        item["@odata.id"] = boost::urls::format(
            "/redfish/v1/Systems/system/PCIeDevices/{}", devName);
        linkArray.emplace_back(item);
    }
    asyncResp->res.jsonValue["Links"]["UpstreamResources@odata.count"] =
        linkArray.size();
    asyncResp->res.jsonValue["Links"]["UpstreamResources"] =
        std::move(linkArray);
}

inline void getCableUpstreamResources(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& cableObjectPath)
{
    // retrieve Upstream Resources
    sdbusplus::message::object_path endpointPath{cableObjectPath};
    endpointPath /= "upstream_resource";

    constexpr std::array<std::string_view, 1> pcieDeviceInterface = {
        "xyz.openbmc_project.Inventory.Item.PCIeDevice"};

    dbus::utility::getAssociatedSubTreePaths(
        endpointPath,
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        pcieDeviceInterface,
        std::bind_front(afterGetCableUpstreamResources, asyncResp));
}

inline void afterGetCableDownstreamResources(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::vector<std::string>& updatedAssemblyList,
    const boost::system::error_code& ec,
    const dbus::utility::MapperEndPoints& endpoints)
{
    if (ec && ec.value() != EBADR)
    {
        BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }
    if (endpoints.empty())
    {
        BMCWEB_LOG_DEBUG("No association found");
        return;
    }

    nlohmann::json::array_t linkArray;
    for (const auto& fullPath : endpoints)
    {
        auto it = std::ranges::find(updatedAssemblyList.begin(),
                                    updatedAssemblyList.end(), fullPath);

        // If element was found
        if (it == updatedAssemblyList.end())
        {
            BMCWEB_LOG_WARNING(
                "in Downstream Resources {} isn't found in chassis assembly list",
                fullPath);
            continue;
        }
        uint index = static_cast<uint>(it - updatedAssemblyList.begin());

        nlohmann::json::object_t item;
        item["@odata.id"] = boost::urls::format(
            "/redfish/v1/Chassis/chassis/Assembly#/Assemblies/{}",
            std::to_string(index));
        linkArray.emplace_back(item);
    }
    asyncResp->res.jsonValue["Links"]["DownstreamResources@odata.count"] =
        linkArray.size();
    asyncResp->res.jsonValue["Links"]["DownstreamResources"] =
        std::move(linkArray);
}

inline void doGetCableDownstreamResources(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& cableObjectPath,
    const std::vector<std::string>& updatedAssemblyList)
{
    sdbusplus::message::object_path endpointPath{cableObjectPath};
    endpointPath /= "downstream_resource";
    dbus::utility::getAssociationEndPoints(
        endpointPath, std::bind_front(afterGetCableDownstreamResources,
                                      asyncResp, updatedAssemblyList));
}

inline void getCableDownstreamResources(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& cableObjectPath)
{
    // retrieve Downstream Resources
    getChassisAssembly(
        asyncResp, "chassis",
        [asyncResp,
         cableObjectPath](const std::optional<std::string>& validChassisPath,
                          const std::vector<std::string>& updatedAssemblyList) {
        if (!validChassisPath || updatedAssemblyList.empty())
        {
            BMCWEB_LOG_DEBUG("Chassis not found");
            return;
        }
        doGetCableDownstreamResources(asyncResp, cableObjectPath,
                                      updatedAssemblyList);
    });
}

inline void afterGetCableAssociatedPorts(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::json_pointer& jsonKeyName,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& endpoints)
{
    if (ec && ec.value() != EBADR)
    {
        BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }
    if (endpoints.empty())
    {
        BMCWEB_LOG_DEBUG("No association found");
        return;
    }
    nlohmann::json::json_pointer jsonCountKeyName = jsonKeyName;
    std::string back = jsonCountKeyName.back();
    jsonCountKeyName.pop_back();
    jsonCountKeyName /= back + "@odata.count";

    nlohmann::json::array_t linkArray;
    for (const auto& fullPath : endpoints)
    {
        sdbusplus::message::object_path path(fullPath);
        std::string portName = path.filename();
        if (portName.empty())
        {
            continue;
        }

        // NOTE: adapterId is currently assumed as the parent of port
        std::string parentPath = path.parent_path();
        std::string adapterName =
            fabric_util::buildFabricUniquePath(parentPath);
        if (adapterName.empty())
        {
            continue;
        }
        nlohmann::json::object_t item;
        item["@odata.id"] = boost::urls::format(
            "/redfish/v1/Systems/system/FabricAdapters/{}/Ports/{}",
            adapterName, portName);
        linkArray.emplace_back(item);
    }
    asyncResp->res.jsonValue["Links"][jsonCountKeyName] = linkArray.size();
    asyncResp->res.jsonValue["Links"][jsonKeyName] = std::move(linkArray);
}

inline void
    getCableAssociatedPorts(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const nlohmann::json::json_pointer& jsonKeyName,
                            const std::string& cableObjectPath,
                            const std::string& associationName)
{
    sdbusplus::message::object_path endpointPath{cableObjectPath};
    endpointPath /= associationName;

    // NOTE: "xyz.openbmc_project.Inventory.Item.Connector" may go away later
    constexpr std::array<std::string_view, 2> portInterfaces = {
        "xyz.openbmc_project.Inventory.Item.Connector",
        "xyz.openbmc_project.Inventory.Connector.Port"};

    dbus::utility::getAssociatedSubTreePaths(
        endpointPath,
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        portInterfaces,
        std::bind_front(afterGetCableAssociatedPorts, asyncResp, jsonKeyName));
}

inline void afterGetCableAssociatedChassis(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::json_pointer& jsonKeyName,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& endpoints)
{
    if (ec && ec.value() != EBADR)
    {
        BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }
    if (endpoints.empty())
    {
        BMCWEB_LOG_DEBUG("No association found");
        return;
    }
    nlohmann::json::json_pointer jsonCountKeyName = jsonKeyName;
    std::string back = jsonCountKeyName.back();
    jsonCountKeyName.pop_back();
    jsonCountKeyName /= back + "@odata.count";

    nlohmann::json::array_t linkArray;
    for (const auto& fullPath : endpoints)
    {
        sdbusplus::message::object_path path(fullPath);
        std::string leaf = path.filename();
        if (leaf.empty())
        {
            continue;
        }
        nlohmann::json::object_t item;
        item["@odata.id"] = boost::urls::format("/redfish/v1/Chassis/{}", leaf);
        linkArray.emplace_back(item);
    }
    asyncResp->res.jsonValue["Links"][jsonCountKeyName] = linkArray.size();
    asyncResp->res.jsonValue["Links"][jsonKeyName] = std::move(linkArray);
}

inline void getCableAssociatedChassis(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::json_pointer& jsonKeyName,
    const std::string& cableObjectPath, const std::string& associationName)
{
    sdbusplus::message::object_path endpointPath{cableObjectPath};
    endpointPath /= associationName;

    constexpr std::array<std::string_view, 2> chassisInterfaces = {
        "xyz.openbmc_project.Inventory.Item.Board",
        "xyz.openbmc_project.Inventory.Item.Chassis"};

    dbus::utility::getAssociatedSubTreePaths(
        endpointPath,
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        chassisInterfaces,
        std::bind_front(afterGetCableAssociatedChassis, asyncResp,
                        jsonKeyName));
}

/**
 * @brief Api to get Cable properties.
 * @param[in,out]   asyncResp       Async HTTP response.
 * @param[in]       cableObjectPath Object path of the Cable.
 * @param[in]       serviceMap      A map to hold Service and corresponding
 * interface list for the given cable id.
 */
inline void
    getCableProperties(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& cableObjectPath,
                       const dbus::utility::MapperServiceMap& serviceMap)
{
    BMCWEB_LOG_DEBUG("Get Properties for cable {}", cableObjectPath);

    // retrieve Upstream/downstream resources
    getCableUpstreamResources(asyncResp, cableObjectPath);
    getCableDownstreamResources(asyncResp, cableObjectPath);

    // retrieve Upstream/downstream ports
    getCableAssociatedPorts(asyncResp,
                            nlohmann::json::json_pointer("/UpstreamPorts"),
                            cableObjectPath, "upstream_connector");
    getCableAssociatedPorts(asyncResp,
                            nlohmann::json::json_pointer("/DownstreamPorts"),
                            cableObjectPath, "downstream_connector");

    // retrieve Upstream/downstream Chassis
    getCableAssociatedChassis(asyncResp,
                              nlohmann::json::json_pointer("/UpstreamChassis"),
                              cableObjectPath, "upstream_chassis");
    getCableAssociatedChassis(
        asyncResp, nlohmann::json::json_pointer("/DownstreamChassis"),
        cableObjectPath, "downstream_chassis");

    for (const auto& [service, interfaces] : serviceMap)
    {
        for (const auto& interface : interfaces)
        {
            if (interface == "xyz.openbmc_project.Inventory.Item.Cable")
            {
                fillCableProperties(asyncResp, cableObjectPath, service);
            }
            else if (interface == "xyz.openbmc_project.Inventory.Item")
            {
                fillCableHealthState(asyncResp, cableObjectPath, service);
            }
        }
    }
}

inline void
    afterHandleCableGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& cableId,
                        const boost::system::error_code& ec,
                        const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec.value() == EBADR)
    {
        messages::resourceNotFound(asyncResp->res, "Cable", cableId);
        return;
    }

    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    for (const auto& [objectPath, serviceMap] : subtree)
    {
        sdbusplus::message::object_path path(objectPath);
        if (path.filename() != cableId)
        {
            continue;
        }

        asyncResp->res.jsonValue["@odata.type"] = "#Cable.v1_2_0.Cable";
        asyncResp->res.jsonValue["@odata.id"] =
            boost::urls::format("/redfish/v1/Cables/{}", cableId);
        asyncResp->res.jsonValue["Id"] = cableId;
        asyncResp->res.jsonValue["Name"] = "Cable";
        asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

        getCableProperties(asyncResp, objectPath, serviceMap);
        return;
    }
    messages::resourceNotFound(asyncResp->res, "Cable", cableId);
}

inline void handleCableGet(App& app, const crow::Request& req,
                           const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& cableId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    BMCWEB_LOG_DEBUG("Cable Id: {}", cableId);

    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, cableInterfaces,
        std::bind_front(afterHandleCableGet, asyncResp, cableId));
}

/**
 * The Cable schema
 */
inline void requestRoutesCable(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Cables/<str>/")
        .privileges(redfish::privileges::getCable)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleCableGet, std::ref(app)));
}

inline void handleCableCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] =
        "#CableCollection.CableCollection";
    asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Cables";
    asyncResp->res.jsonValue["Name"] = "Cable Collection";
    asyncResp->res.jsonValue["Description"] = "Collection of Cable Entries";
    collection_util::getCollectionMembers(
        asyncResp, boost::urls::url("/redfish/v1/Cables"), cableInterfaces,
        "/xyz/openbmc_project/inventory");
}

/**
 * Collection of Cable resource instances
 */
inline void requestRoutesCableCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Cables/")
        .privileges(redfish::privileges::getCableCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleCableCollectionGet, std::ref(app)));
}

} // namespace redfish
