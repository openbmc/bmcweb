#pragma once
#include <boost/container/flat_map.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{
// Map of service name to list of interfaces
using MapperServiceMap =
    std::vector<std::pair<std::string, std::vector<std::string>>>;

// Map of object paths to MapperServiceMaps
using MapperGetSubTreeResponse =
    std::vector<std::pair<std::string, MapperServiceMap>>;

// List of cable property key/value pairs
using CableProperties =
    std::vector<std::pair<std::string, std::variant<std::string, double>>>;

/**
 * @brief Fill cable specific properties.
 * @param[in,out]   asyncResp   Async HTTP response.
 * @param[in]       ec          Error code corresponding to Async method call.
 * @param[in]       properties  List of Cable Properties key/value pairs.
 */
inline void
    fillCableProperties(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const boost::system::error_code ec,
                        const CableProperties& properties)
{
    for (const auto& [propKey, propVariant] : properties)
    {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
            messages::internalError(asyncResp->res);
            return;
        }

        if (propKey == "CableTypeDescription")
        {
            const std::string* cableTypeDescription =
                std::get_if<std::string>(&propVariant);
            if (cableTypeDescription == nullptr)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            asyncResp->res.jsonValue["CableType"] = *cableTypeDescription;
        }
        else if (propKey == "Length")
        {
            const double* cableLength = std::get_if<double>(&propVariant);
            if (cableLength == nullptr)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            if (std::isnan(*cableLength))
            {
                BMCWEB_LOG_DEBUG << "Cable length is NaN";
                continue;
            }
            asyncResp->res.jsonValue["LengthMeters"] = *cableLength;
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
                       const MapperServiceMap& serviceMap)
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
                [asyncResp](const boost::system::error_code ec,
                            const CableProperties& properties) {
                    fillCableProperties(asyncResp, ec, properties);
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
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& cableId) {
                BMCWEB_LOG_DEBUG << "Cable Id: " << cableId;
                auto respHandler =
                    [asyncResp,
                     cableId](const boost::system::error_code ec,
                              const MapperGetSubTreeResponse& subtree) {
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
                            asyncResp->res.jsonValue["Name"] =
                                "Cable Properties";

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
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
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
