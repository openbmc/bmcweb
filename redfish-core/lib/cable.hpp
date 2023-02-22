#pragma once

#include "dbus_utility.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"

#include <boost/system/error_code.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>
#include <utils/chassis_utils.hpp>
#include <utils/dbus_utils.hpp>
#include <utils/json_utils.hpp>

#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace redfish
{

/**
 * @brief Api to get Cable Association.
 * @param[in,out]   asyncResp       Async HTTP response.
 * @param[in]       cableObjectPath Object path of the Cable with association.
 * @param[in]       callback        callback method
 */
template <typename Callback>
inline void
    linkAssociatedCable(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& cableObjectPath, Callback&& callback)
{

    dbus::utility::getAssociationEndPoints(
        cableObjectPath,
        [asyncResp, callback](const boost::system::error_code& ec,
                              const dbus::utility::MapperEndPoints& endpoints) {
        if (ec)
        {
            if (ec.value() == EBADR)
            {
                // This cable have no association.
                BMCWEB_LOG_DEBUG << "No association found";
                return;
            }
            BMCWEB_LOG_ERROR << "DBUS response error" << ec.message();
            messages::internalError(asyncResp->res);
            return;
        }

        if (endpoints.empty())
        {
            BMCWEB_LOG_DEBUG << "No association found for cable";
            messages::internalError(asyncResp->res);
            return;
        }
        callback(endpoints);
        });
}

/**
 * @brief Fill cable specific properties.
 * @param[in,out]   resp        HTTP response.
 * @param[in]       ec          Error code corresponding to Async method call.
 * @param[in]       properties  List of Cable Properties key/value pairs.
 */
inline void
    fillCableProperties(crow::Response& resp,
                        const boost::system::error_code ec,
                        const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
        messages::internalError(resp);
        return;
    }

    const std::string* cableTypeDescription = nullptr;
    const double* length = nullptr;
    const std::string* cableStatus = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "CableTypeDescription",
        cableTypeDescription, "Length", length, "CableStatus", cableStatus);

    if (!success)
    {
        messages::internalError(resp);
        return;
    }

    if (cableTypeDescription != nullptr)
    {
        resp.jsonValue["CableType"] = *cableTypeDescription;
    }

    if (length != nullptr)
    {
        if (!std::isfinite(*length))
        {
            // Cable length is NaN by default, do not throw an error
            if (!std::isnan(*length))
            {
                messages::internalError(resp);
                return;
            }
        }
        else
        {
            resp.jsonValue["LengthMeters"] = *length;
        }
    }

    if (cableStatus != nullptr && !cableStatus->empty())
    {
        if (*cableStatus ==
            "xyz.openbmc_project.Inventory.Item.Cable.Status.Inactive")
        {
            resp.jsonValue["CableStatus"] = "Normal";
            resp.jsonValue["Status"]["State"] = "StandbyOffline";
            resp.jsonValue["Status"]["Health"] = "OK";
        }
        else if (*cableStatus == "xyz.openbmc_project.Inventory.Item."
                                 "Cable.Status.Running")
        {
            resp.jsonValue["CableStatus"] = "Normal";
            resp.jsonValue["Status"]["State"] = "Enabled";
            resp.jsonValue["Status"]["Health"] = "OK";
        }
        else if (*cableStatus == "xyz.openbmc_project.Inventory.Item."
                                 "Cable.Status.PoweredOff")
        {
            resp.jsonValue["CableStatus"] = "Disabled";
            resp.jsonValue["Status"]["State"] = "StandbyOffline";
            resp.jsonValue["Status"]["Health"] = "OK";
        }
    }
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
    BMCWEB_LOG_DEBUG << "Get Properties for cable " << cableObjectPath;

    // retrieve Upstream Resources
    std::string upstreamResource = cableObjectPath + "/upstream_resource";
    linkAssociatedCable(asyncResp, upstreamResource,
                        [asyncResp](const std::vector<std::string>& value) {
        nlohmann::json& linkArray =
            asyncResp->res.jsonValue["Links"]["UpstreamResources"];
        linkArray = nlohmann::json::array();

        for (const auto& fullPath : value)
        {
            sdbusplus::message::object_path path(fullPath);
            std::string leaf = path.filename();
            if (leaf.empty())
            {
                continue;
            }
            linkArray.push_back(
                {{"@odata.id",
                  "/redfish/v1/Systems/system/PCIeDevices/" + leaf}});
        }
    });

    // retrieve Downstream Resources
    auto chassisAssociation =
        [asyncResp,
         cableObjectPath](std::vector<std::string>& updatedAssemblyList) {
        std::string downstreamResource =
            cableObjectPath + "/downstream_resource";
        linkAssociatedCable(asyncResp, downstreamResource,
                            [asyncResp, updatedAssemblyList](
                                const std::vector<std::string>& value) {
            nlohmann::json& linkArray =
                asyncResp->res.jsonValue["Links"]["DownstreamResources"];
            linkArray = nlohmann::json::array();

            for (const auto& fullPath : value)
            {
                auto it = find(updatedAssemblyList.begin(),
                               updatedAssemblyList.end(), fullPath);

                // If element was found
                if (it != updatedAssemblyList.end())
                {

                    uint index =
                        static_cast<uint>(it - updatedAssemblyList.begin());
                    linkArray.push_back(
                        {{"@odata.id", "/redfish/v1/Chassis/chassis/"
                                       "Assembly#/Assemblies/" +
                                           std::to_string(index)}});
                }
                else
                {
                    BMCWEB_LOG_ERROR << "in Downstream Resources " << fullPath
                                     << " isn't found in chassis assembly list";
                }
            }
        });
    };

    redfish::chassis_utils::getChassisAssembly(asyncResp, "chassis",
                                               std::move(chassisAssociation));

    // retrieve Upstream Ports
    std::string upstreamConnector = cableObjectPath + "/upstream_connector";
    linkAssociatedCable(asyncResp, upstreamConnector,
                        [asyncResp](const std::vector<std::string>& value) {
        nlohmann::json& linkArray =
            asyncResp->res.jsonValue["Links"]["UpstreamPorts"];
        linkArray = nlohmann::json::array();

        for (const auto& fullPath : value)
        {
            sdbusplus::message::object_path path(fullPath);
            std::string leaf = path.filename();
            if (leaf.empty())
            {
                continue;
            }
            std::string endpointLeaf = path.parent_path().filename();
            if (endpointLeaf.empty())
            {
                continue;
            }

            //  insert/create link using endpointLeaf
            endpointLeaf += "/Ports/";
            endpointLeaf += leaf;
            linkArray.push_back(
                {{"@odata.id", "/redfish/v1/Systems/system/FabricAdapters/" +
                                   endpointLeaf}});
        }
    });

    // retrieve Downstream Ports
    std::string downstreamConnector = cableObjectPath + "/downstream_connector";
    linkAssociatedCable(asyncResp, downstreamConnector,
                        [asyncResp](const std::vector<std::string>& value) {
        nlohmann::json& linkArray =
            asyncResp->res.jsonValue["Links"]["DownstreamPorts"];
        linkArray = nlohmann::json::array();

        for (const auto& fullPath : value)
        {
            sdbusplus::message::object_path path(fullPath);
            std::string leaf = path.filename();
            if (leaf.empty())
            {
                continue;
            }
            std::string endpointLeaf = path.parent_path().filename();
            if (endpointLeaf.empty())
            {
                continue;
            }

            //  insert/create link suing endpointLeaf
            endpointLeaf += "/Ports/";
            endpointLeaf += leaf;
            linkArray.push_back(
                {{"@odata.id", "/redfish/v1/Systems/system/FabricAdapters/" +
                                   endpointLeaf}});
        }
    });

    // retrieve Upstream Chassis
    std::string upstreamChassis = cableObjectPath + "/upstream_chassis";
    linkAssociatedCable(asyncResp, upstreamChassis,
                        [asyncResp](const std::vector<std::string>& value) {
        nlohmann::json& linkArray =
            asyncResp->res.jsonValue["Links"]["UpstreamChassis"];
        linkArray = nlohmann::json::array();

        for (const auto& fullPath : value)
        {
            sdbusplus::message::object_path path(fullPath);
            std::string leaf = path.filename();
            if (leaf.empty())
            {
                continue;
            }
            linkArray.push_back({{"@odata.id", "/redfish/v1/Chassis/" + leaf}});
        }
    });

    // retrieve Downstream Chassis
    std::string downstreamChassis = cableObjectPath + "/downstream_chassis";
    linkAssociatedCable(asyncResp, downstreamChassis,
                        [asyncResp](const std::vector<std::string>& value) {
        nlohmann::json& linkArray =
            asyncResp->res.jsonValue["Links"]["DownstreamChassis"];
        linkArray = nlohmann::json::array();
        for (const auto& fullPath : value)
        {
            sdbusplus::message::object_path path(fullPath);
            std::string leaf = path.filename();
            if (leaf.empty())
            {
                continue;
            }
            linkArray.push_back({{"@odata.id", "/redfish/v1/Chassis/" + leaf}});
        }
    });

    for (const auto& [service, interfaces] : serviceMap)
    {
        for (const auto& interface : interfaces)
        {
            if (interface != "xyz.openbmc_project.Inventory.Item.Cable")
            {
                continue;
            }

            sdbusplus::asio::getAllProperties(
                *crow::connections::systemBus, service, cableObjectPath,
                interface,
                [asyncResp](
                    const boost::system::error_code ec,
                    const dbus::utility::DBusPropertiesMap& properties) {
                fillCableProperties(asyncResp->res, ec, properties);
                });

            sdbusplus::asio::getProperty<std::string>(
                *crow::connections::systemBus, service, cableObjectPath,
                "xyz.openbmc_project.Inventory.Decorator.Asset", "PartNumber",
                [asyncResp](const boost::system::error_code ec,
                            const std::string& property) {
                if (ec.value() == EBADR)
                {
                    // PartNumber is optional, ignore the failure if it
                    // doesn't exist.
                    return;
                }
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "DBus response error for PartNumber";
                    messages::internalError(asyncResp->res);
                    return;
                }
                asyncResp->res.jsonValue["PartNumber"] = property;
                });
        }
    }
}

/**
 * The Cable schema
 */
inline void requestRoutesCable(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Cables/<str>/")
        .privileges(redfish::privileges::getCable)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& cableId) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        BMCWEB_LOG_DEBUG << "Cable Id: " << cableId;
        constexpr std::array<std::string_view, 1> interfaces = {
            "xyz.openbmc_project.Inventory.Item.Cable"};
        dbus::utility::getSubTree(
            "/xyz/openbmc_project/inventory", 0, interfaces,
            [asyncResp,
             cableId](const boost::system::error_code& ec,
                      const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec.value() == EBADR)
            {
                messages::resourceNotFound(asyncResp->res, "Cable", cableId);
                return;
            }

            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBUS response error " << ec;
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
                    "/redfish/v1/Cables/" + cableId;
                asyncResp->res.jsonValue["Id"] = cableId;
                asyncResp->res.jsonValue["Name"] = "Cable";

                getCableProperties(asyncResp, objectPath, serviceMap);
                return;
            }
            messages::resourceNotFound(asyncResp->res, "Cable", cableId);
            });
        });
}

/**
 * Collection of Cable resource instances
 */
inline void requestRoutesCableCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Cables/")
        .privileges(redfish::privileges::getCableCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        asyncResp->res.jsonValue["@odata.type"] =
            "#CableCollection.CableCollection";
        asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Cables";
        asyncResp->res.jsonValue["Name"] = "Cable Collection";
        asyncResp->res.jsonValue["Description"] = "Collection of Cable Entries";
        constexpr std::array<std::string_view, 1> interfaces{
            "xyz.openbmc_project.Inventory.Item.Cable"};
        collection_util::getCollectionMembers(
            asyncResp, boost::urls::url("/redfish/v1/Cables"), interfaces);
        });
}

} // namespace redfish
