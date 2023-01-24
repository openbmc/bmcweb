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

#include <array>
#include <string_view>

namespace redfish
{
/**
 * @brief Fill cable specific properties.
 * @param[in,out]   resp        HTTP response.
 * @param[in]       ec          Error code corresponding to Async method call.
 * @param[in]       properties  List of Cable Properties key/value pairs.
 */
inline void
    fillCableProperties(crow::Response& resp,
                        const boost::system::error_code& ec,
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

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "CableTypeDescription",
        cableTypeDescription, "Length", length);

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

    for (const auto& [service, interfaces] : serviceMap)
    {
        for (const auto& interface : interfaces)
        {
            if (interface == "xyz.openbmc_project.Inventory.Item.Cable")
            {
                sdbusplus::asio::getAllProperties(
                    *crow::connections::systemBus, service, cableObjectPath,
                    interface,
                    [asyncResp](
                        const boost::system::error_code& ec,
                        const dbus::utility::DBusPropertiesMap& properties) {
                    fillCableProperties(asyncResp->res, ec, properties);
                    });
            }
            else if (interface == "xyz.openbmc_project.Inventory.Item")
            {
                sdbusplus::asio::getProperty<bool>(
                    *crow::connections::systemBus, service, cableObjectPath,
                    interface, "Present",
                    [asyncResp,
                     cableObjectPath](const boost::system::error_code& ec,
                                      bool present) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG << "get presence failed for Cable "
                                         << cableObjectPath << " with error "
                                         << ec;
                        if (ec.value() != EBADR)
                        {
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

                asyncResp->res.jsonValue["@odata.type"] = "#Cable.v1_0_0.Cable";
                asyncResp->res.jsonValue["@odata.id"] =
                    crow::utility::urlFromPieces("redfish", "v1", "Cables",
                                                 cableId);
                asyncResp->res.jsonValue["Id"] = cableId;
                asyncResp->res.jsonValue["Name"] = "Cable";
                asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

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
