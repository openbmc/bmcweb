#pragma once
#include <dbus_utility.hpp>
#include <query.hpp>
#include <utils/json_utils.hpp>

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
                        const boost::system::error_code ec,
                        const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
        messages::internalError(resp);
        return;
    }

    for (const auto& [propKey, propVariant] : properties)
    {
        if (propKey == "CableTypeDescription")
        {
            const std::string* cableTypeDescription =
                std::get_if<std::string>(&propVariant);
            if (cableTypeDescription == nullptr)
            {
                messages::internalError(resp);
                return;
            }
            resp.jsonValue["CableType"] = *cableTypeDescription;
        }
        else if (propKey == "Length")
        {
            const double* cableLength = std::get_if<double>(&propVariant);
            if (cableLength == nullptr)
            {
                messages::internalError(resp);
                return;
            }

            if (!std::isfinite(*cableLength))
            {
                if (std::isnan(*cableLength))
                {
                    continue;
                }
                messages::internalError(resp);
                return;
            }

            resp.jsonValue["LengthMeters"] = *cableLength;
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
            if (interface != "xyz.openbmc_project.Inventory.Item.Cable")
            {
                continue;
            }

            crow::connections::systemBus->async_method_call(
                [asyncResp](
                    const boost::system::error_code ec,
                    const dbus::utility::DBusPropertiesMap& properties) {
                    fillCableProperties(asyncResp->res, ec, properties);
                },
                service, cableObjectPath, "org.freedesktop.DBus.Properties",
                "GetAll", interface);
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
                if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
                {
                    return;
                }
                BMCWEB_LOG_DEBUG << "Cable Id: " << cableId;
                auto respHandler =
                    [asyncResp,
                     cableId](const boost::system::error_code ec,
                              const dbus::utility::MapperGetSubTreeResponse&
                                  subtree) {
                        if (ec.value() == EBADR)
                        {
                            messages::resourceNotFound(
                                asyncResp->res, "#Cable.v1_0_0.Cable", cableId);
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

                            asyncResp->res.jsonValue["@odata.type"] =
                                "#Cable.v1_0_0.Cable";
                            asyncResp->res.jsonValue["@odata.id"] =
                                "/redfish/v1/Cables/" + cableId;
                            asyncResp->res.jsonValue["Id"] = cableId;
                            asyncResp->res.jsonValue["Name"] = "Cable";

                            getCableProperties(asyncResp, objectPath,
                                               serviceMap);
                            return;
                        }
                        messages::resourceNotFound(asyncResp->res, "Cable",
                                                   cableId);
                    };

                crow::connections::systemBus->async_method_call(
                    respHandler, "xyz.openbmc_project.ObjectMapper",
                    "/xyz/openbmc_project/object_mapper",
                    "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                    "/xyz/openbmc_project/inventory", 0,
                    std::array<const char*, 1>{
                        "xyz.openbmc_project.Inventory.Item.Cable"});
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
                if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
                {
                    return;
                }
                asyncResp->res.jsonValue["@odata.type"] =
                    "#CableCollection.CableCollection";
                asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Cables";
                asyncResp->res.jsonValue["Name"] = "Cable Collection";
                asyncResp->res.jsonValue["Description"] =
                    "Collection of Cable Entries";

                collection_util::getCollectionMembers(
                    asyncResp, "/redfish/v1/Cables",
                    {"xyz.openbmc_project.Inventory.Item.Cable"});
            });
}

} // namespace redfish
