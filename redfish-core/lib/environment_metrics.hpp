#pragma once

#include <node.hpp>
#include <utils/chassis_utils.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{
inline void
    getfanSpeedsPercent(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& chassisId)
{
    BMCWEB_LOG_DEBUG << "Get properties for getFan associated to chassis = "
                     << chassisId;
    const std::array<std::string, 1> sensorInterfaces = {
        "xyz.openbmc_project.Sensor.Value"};
    crow::connections::systemBus->async_method_call(
        [asyncResp, chassisId](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                sensorsubtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "D-Bus response error on GetSubTree " << ec;
                if (ec.value() == boost::system::errc::io_error)
                {
                    messages::resourceNotFound(asyncResp->res, "Chassis",
                                               chassisId);
                    return;
                }
                messages::internalError(asyncResp->res);
                return;
            }
            nlohmann::json& fanList =
                asyncResp->res.jsonValue["FanSpeedsPercent"];
            fanList = nlohmann::json::array();

            for (const auto& [objectPath, serviceName] : sensorsubtree)
            {
                if (objectPath.empty() || serviceName.size() != 1)
                {
                    BMCWEB_LOG_DEBUG << "Error getting D-Bus object!";
                    messages::internalError(asyncResp->res);
                    return;
                }
                const std::string& validPath = objectPath;
                const std::string& connectionName = serviceName[0].first;
                std::vector<std::string> split;
                // Reserve space for
                // /xyz/openbmc_project/sensors/<name>/<subname>
                split.reserve(6);
                boost::algorithm::split(split, validPath,
                                        boost::is_any_of("/"));
                if (split.size() < 6)
                {
                    BMCWEB_LOG_ERROR << "Got path that isn't long enough "
                                     << validPath;
                    continue;
                }
                // These indexes aren't intuitive, as boost::split puts an empty
                // string at the beginning
                const std::string& sensorType = split[4];
                const std::string& sensorName = split[5];
                BMCWEB_LOG_DEBUG << "sensorName " << sensorName
                                 << " sensorType " << sensorType;
                if (sensorType == "fan" || sensorType == "fan_tach" ||
                    sensorType == "fan_pwm")
                {
                    crow::connections::systemBus->async_method_call(
                        [asyncResp, chassisId, &fanList,
                         sensorName](const boost::system::error_code ec,
                                     const std::variant<double>& value) {
                            if (ec)
                            {
                                BMCWEB_LOG_DEBUG << "Can't get Fan speed!";
                                messages::internalError(asyncResp->res);
                                return;
                            }

                            const double* attributeValue =
                                std::get_if<double>(&value);
                            if (attributeValue == nullptr)
                            {
                                // illegal property
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            std::string tempPath = "/redfish/v1/Chassis/" +
                                                   chassisId + "/Sensors/";
                            fanList.push_back(
                                {{"DeviceName", "Chassis Fan #" + sensorName},
                                 {"SpeedRPM", *attributeValue},
                                 {"DataSourceUri", tempPath + sensorName},
                                 {"@odata.id", tempPath + sensorName}});
                        },
                        connectionName, validPath,
                        "org.freedesktop.DBus.Properties", "Get",
                        "xyz.openbmc_project.Sensor.Value", "Value");
                }
                else
                {
                    BMCWEB_LOG_DEBUG
                        << "This is not a fan-related sensor,sensortype = "
                        << sensorType;
                    continue;
                }
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/sensors", 0, sensorInterfaces);
}

inline void getPowerWatts(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& chassisId)
{
    const std::array<const char*, 1> totalPowerInterfaces = {
        "xyz.openbmc_project.Sensor.Value"};
    const std::string& totalPowerPath =
        "/xyz/openbmc_project/sensors/power/total_power";
    crow::connections::systemBus->async_method_call(
        [asyncResp, chassisId, totalPowerPath](
            const boost::system::error_code ec,
            const std::vector<std::pair<std::string, std::vector<std::string>>>&
                object) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(asyncResp->res);
                return;
            }
            for (const auto& tempObject : object)
            {
                const std::string& connectionName = tempObject.first;
                crow::connections::systemBus->async_method_call(
                    [asyncResp, chassisId](const boost::system::error_code ec,
                                           const std::variant<double>& value) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG << "Can't get Power Watts!";
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        const double* attributeValue =
                            std::get_if<double>(&value);
                        if (attributeValue == nullptr)
                        {
                            // illegal property
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        std::string tempPath =
                            "/redfish/v1/Chassis/" + chassisId + "/Sensors/";
                        asyncResp->res.jsonValue["PowerWatts"] = {
                            {"Reading", *attributeValue},
                            {"DataSourceUri", tempPath + "total_power"},
                            {"@odata.id", tempPath + "total_power"}};
                    },
                    connectionName, totalPowerPath,
                    "org.freedesktop.DBus.Properties", "Get",
                    "xyz.openbmc_project.Sensor.Value", "Value");
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", totalPowerPath,
        totalPowerInterfaces);
}

inline void
    getEnvironmentMetrics(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& chassisId)
{
    BMCWEB_LOG_DEBUG
        << "Get properties for EnvironmentMetrics associated to chassis = "
        << chassisId;
    asyncResp->res.jsonValue["@odata.type"] =
        "#EnvironmentMetrics.v1_0_0.EnvironmentMetrics";
    asyncResp->res.jsonValue["Name"] = "Chassis Environment Metrics";
    asyncResp->res.jsonValue["Id"] = "EnvironmentMetrics";
    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Chassis/" + chassisId + "/EnvironmentMetrics";
    getfanSpeedsPercent(asyncResp, chassisId);
    getPowerWatts(asyncResp, chassisId);
}

class EnvironmentMetrics : public Node
{
  public:
    /*
     * Default Constructor
     */
    EnvironmentMetrics(App& app) :
        Node(app, "/redfish/v1/Chassis/<str>/EnvironmentMetrics/",
             std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    void doGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const crow::Request&,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& chassisID = params[0];

        auto getChassisID =
            [asyncResp,
             chassisID](const std::optional<std::string>& validChassisID) {
                if (!validChassisID)
                {
                    BMCWEB_LOG_ERROR << "Not a valid chassis ID:" << chassisID;
                    messages::resourceNotFound(asyncResp->res, "Chassis",
                                               chassisID);
                    return;
                }

                getEnvironmentMetrics(asyncResp, *validChassisID);
            };
        redfish::chassis_utils::getValidChassisID(asyncResp, chassisID,
                                                  std::move(getChassisID));
    }
}; // namespace redfish

} // namespace redfish