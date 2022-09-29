#pragma once

#include <app.hpp>
#include <utils/chassis_utils.hpp>

namespace redfish
{

inline void
    updateFanList(const std::shared_ptr<bmcweb::AsyncResp>& /* asyncResp */,
                  const std::string& /* chassisId */,
                  const std::string& fanPath)
{
    std::string fanName = sdbusplus::message::object_path(fanPath).filename();
    if (fanName.empty())
    {
        return;
    }

    // TODO In order for the validator to pass, the Members property will be
    // implemented on the next commit
}

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
    asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
    asyncResp->res.jsonValue["Members@odata.count"] = 0;

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/FanCollection/FanCollection.json>; rel=describedby");

    crow::connections::systemBus->async_method_call(
        [asyncResp, chassisId](
            const boost::system::error_code ec,
            const dbus::utility::MapperGetSubTreePathsResponse& fanPaths) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error: " << ec.message();
            messages::internalError(asyncResp->res);
            return;
        }

        for (const std::string& fanPath : fanPaths)
        {
            // get cec fan:
            // /xyz/openbmc_project/inventory/system/chassisName/motherboard/fanName
            sdbusplus::asio::getProperty<std::vector<std::string>>(
                *crow::connections::systemBus,
                "xyz.openbmc_project.ObjectMapper", fanPath + "/sensors",
                "xyz.openbmc_project.Association", "endpoints",
                [asyncResp, chassisId,
                 fanPath](const boost::system::error_code ec1,
                          const std::vector<std::string>& fanSensorsEndpoints) {
                if (ec1)
                {
                    // get Mex Fan:
                    // /xyz/openbmc_project/inventory/system/chassisName/fanName
                    updateFanList(asyncResp, chassisId, fanPath);
                    return;
                }

                for (const auto& fanSensorPath : fanSensorsEndpoints)
                {
                    updateFanList(asyncResp, chassisId, fanSensorPath);
                }
                });
        }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        *validChassisPath, 0,
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
