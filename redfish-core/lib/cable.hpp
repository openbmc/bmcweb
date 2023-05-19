#pragma once

#include "dbus_utility.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"

#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>
#include <utils/dbus_tree_parser.hpp>

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
    using namespace dbus::utility;
    DbusBaseMapper mapper;
    mapper.addMap("CableTypeDescription",
                  mapToKeyOrError<std::string>("CableType"));
    mapper.addMap("Length", mapToKeyOrError<double, double>("LengthMeters",
                                                            [](auto length) {
        if (!std::isfinite(length) && !std::isnan(length))
        {
            return std::optional<double>();
        }
        return std::optional(length);
                  }));

    DbusTreeParser(mapper, true)
        .parse(properties,
               [&resp](DbusParserStatus status, const auto& target) {
        if (status == DbusParserStatus::Failed)
        {
            resp.result(boost::beast::http::status::internal_server_error);
        }
        resp.jsonValue.merge_patch(target);
        });
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
            if (interface != "xyz.openbmc_project.Inventory.Item.Cable")
            {
                continue;
            }

            sdbusplus::asio::getAllProperties(
                *crow::connections::systemBus, service, cableObjectPath,
                interface,
                [asyncResp](
                    const boost::system::error_code& ec,
                    const dbus::utility::DBusPropertiesMap& properties) {
                fillCableProperties(asyncResp->res, ec, properties);
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

                asyncResp->res.jsonValue["@odata.type"] = "#Cable.v1_0_0.Cable";
                asyncResp->res.jsonValue["@odata.id"] =
                    boost::urls::format("/redfish/v1/Cables/{}", cableId);
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
