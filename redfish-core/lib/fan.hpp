#pragma once

#include <app.hpp>
#include <utils/chassis_utils.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

inline void doFanCollection(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& chassisId,
                            const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        BMCWEB_LOG_ERROR << "Not a valid chassis ID" << chassisId;
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] = "#FanCollection.FanCollection";
    asyncResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Chassis", chassisId, "ThermalSubsystem", "Fans");
    asyncResp->res.jsonValue["Name"] = "Fan Collection";
    asyncResp->res.jsonValue["Description"] =
        "The collection of Fan resource instances " + chassisId;
    crow::connections::systemBus->async_method_call(
        [asyncResp,
         chassisId](const boost::system::error_code ec,
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error: " << ec.message();
            messages::internalError(asyncResp->res);
            return;
        }

        nlohmann::json& fanList = asyncResp->res.jsonValue["Members"];
        fanList = nlohmann::json::array();

        std::string newPath =
            "/redfish/v1/Chassis/" + chassisId + "/ThermalSubsystem/Fans/";

        for (const auto& [objectPath, serviceMap] : subtree)
        {
            for (const auto& [service, interfaces] : serviceMap)
            {
                if (service == "xyz.openbmc_project.PLDM")
                {
                    // get Mex Fan: 5 below comes from
                    // /xyz/openbmc_project/inventory/system/chassisName/fanName
                    //   0      1             2        3        4         5
                    std::string chassisName;
                    std::string fanName;
                    if (!dbus::utility::getNthStringFromPath(objectPath, 4,
                                                             chassisName) ||
                        !dbus::utility::getNthStringFromPath(objectPath, 5,
                                                             fanName) ||
                        chassisName != chassisId)
                    {
                        BMCWEB_LOG_ERROR << "Got invalid path " << objectPath;
                        continue;
                    }

                    fanList.push_back(
                        {{"@odata.id",
                          crow::utility::urlFromPieces(
                              "redfish", "v1", "Chassis", chassisId,
                              "ThermalSubsystem", "Fans", fanName)}});
                    asyncResp->res.jsonValue["Members@odata.count"] =
                        fanList.size();
                }
                else if (service == "xyz.openbmc_project.Inventory.Manager")
                {
                    // get cec fan: 6 below comes from
                    // /xyz/openbmc_project/inventory/system/chassisName/motherboard/fanName
                    //   0      1             2         3         4         5 6
                    std::string chassisName;
                    if (!dbus::utility::getNthStringFromPath(objectPath, 4,
                                                             chassisName) ||
                        chassisName != chassisId)
                    {
                        BMCWEB_LOG_ERROR << "Got invalid path " << objectPath;
                        continue;
                    }

                    sdbusplus::asio::getProperty<std::vector<std::string>>(
                        *crow::connections::systemBus,
                        "xyz.openbmc_project.ObjectMapper",
                        objectPath + "/sensors",
                        "xyz.openbmc_project.Association", "endpoints",
                        [asyncResp, chassisId,
                         &fanList](const boost::system::error_code ec1,
                                   const std::vector<std::string>&
                                       fanSensorsEndpoints) {
                        if (ec1)
                        {
                            BMCWEB_LOG_DEBUG << "DBUS response error: "
                                             << ec1.message();
                            if (ec1.value() == boost::system::errc::io_error)
                            {
                                messages::resourceNotFound(
                                    asyncResp->res, "Chassis", chassisId);
                                return;
                            }

                            messages::internalError(asyncResp->res);
                            return;
                        }

                        for (const auto& fanSensorPath : fanSensorsEndpoints)
                        {
                            const std::string& fanName =
                                sdbusplus::message::object_path(fanSensorPath)
                                    .filename();
                            if (fanName.empty())
                            {
                                continue;
                            }

                            sdbusplus::asio::getProperty<
                                std::vector<std::string>>(
                                *crow::connections::systemBus,
                                "xyz.openbmc_project.ObjectMapper",
                                fanSensorPath + "/chassis",
                                "xyz.openbmc_project.Association", "endpoints",
                                [asyncResp, fanName, chassisId,
                                 &fanList](const boost::system::error_code ec2,
                                           const std::vector<std::string>&
                                               fanChassisEndpoints) {
                                if (ec2)
                                {
                                    BMCWEB_LOG_DEBUG << "DBUS response error: "
                                                     << ec2.message();
                                    if (ec2.value() == EBADR)
                                    {
                                        // This fan have no chassis association
                                        return;
                                    }
                                    messages::internalError(asyncResp->res);
                                    return;
                                }

                                for (const auto& fanChassisPath :
                                     fanChassisEndpoints)
                                {
                                    const std::string& fanChassisName =
                                        sdbusplus::message::object_path(
                                            fanChassisPath)
                                            .filename();
                                    if (fanChassisName != chassisId)
                                    {
                                        // The Fan does't belong to the
                                        // chassisId
                                        return;
                                    }
                                }

                                fanList.push_back(
                                    {{"@odata.id",
                                      crow::utility::urlFromPieces(
                                          "redfish", "v1", "Chassis", chassisId,
                                          "ThermalSubsystem", "Fans",
                                          fanName)}});
                                asyncResp->res
                                    .jsonValue["Members@odata.count"] =
                                    fanList.size();
                                });
                        }
                        });
                }
            }
        }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0,
        std::array<const char*, 1>{"xyz.openbmc_project.Inventory.Item.Fan"});
}

inline void
    handleFanCollectionGet(App& app, const crow::Request& req,
                           const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        std::bind_front(doFanCollection, asyncResp, chassisId));
}

inline void requestRoutesFanCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/Fans/")
        .privileges(redfish::privileges::getFanCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFanCollectionGet, std::ref(app)));
}

} // namespace redfish
