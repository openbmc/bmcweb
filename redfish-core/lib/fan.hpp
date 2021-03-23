#pragma once

#include <boost/algorithm/string/split.hpp>
#include <node.hpp>
#include <utils/chassis_utils.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

/**
 * @brief Retrieves valid fan ID
 * @param asyncResp   Pointer to object holding response data
 * @param callback  Callback for next step to get valid fan ID
 */
template <typename Callback>
inline void getValidFanID(const std::shared_ptr<AsyncResp>& asyncResp,
                          const std::string& fanId, Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "checkFanId enter";
    const std::array<const char*, 1> interfaces = {
        "xyz.openbmc_project.Sensor.Value"};
    auto respHandler = [callback{std::move(callback)}, asyncResp,
                        fanId](const boost::system::error_code ec,
                               const std::vector<std::string>& sensorPaths) {
        BMCWEB_LOG_DEBUG << "getValidFanID respHandler enter";
        if (ec)
        {
            BMCWEB_LOG_ERROR << "getValidFanID respHandler DBUS error: " << ec;
            messages::internalError(asyncResp->res);
            return;
        }

        std::optional<std::string> validFanId;
        for (const std::string& sensor : sensorPaths)
        {
            const std::string& path = sensor;
            std::vector<std::string> split;
            // Reserve space for
            // /xyz/openbmc_project/sensors/<name>/<subname>
            split.reserve(6);
            boost::algorithm::split(split, path, boost::is_any_of("/"));
            if (split.size() < 6)
            {
                BMCWEB_LOG_ERROR << "Got path that "
                                    "isn't long enough "
                                 << path;
                continue;
            }
            // These indexes aren't intuitive, as
            // boost::split puts an empty string at the
            // beginning
            const std::string& fanType = split[4];
            const std::string& fanName = split[5];
            if (fanType == "fan" || fanType == "fan_tach" ||
                fanType == "fan_pwm")
            {
                if (fanName == fanId)
                {
                    validFanId = fanId;
                    break;
                }
            }
            else
            {
                BMCWEB_LOG_ERROR << "This is not fan " << fanType;
                continue;
            }
        }
        callback(validFanId);
    };
    // Get the collection of all sensors including the fan
    crow::connections::systemBus->async_method_call(
        respHandler, "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/sensors", 0, interfaces);
    BMCWEB_LOG_DEBUG << "checkFanId exit";
}

inline void getFanState(const std::shared_ptr<AsyncResp>& asyncResp,
                        const std::string& connectionName,
                        const std::string& path)
{
    // Set the default state to Absent
    asyncResp->res.jsonValue["Status"]["State"] = "Absent";

    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const std::variant<bool>& state) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "Can't get Fan state!";
                messages::internalError(asyncResp->res);
                return;
            }

            const bool* value = std::get_if<bool>(&state);
            if (value == nullptr)
            {
                // illegal property
                messages::internalError(asyncResp->res);
                return;
            }
            if (*value)
            {
                asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
            }
        },
        connectionName, path, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Inventory.Item", "Present");
}

inline void getFanHealth(const std::shared_ptr<AsyncResp>& asyncResp,
                         const std::string& connectionName,
                         const std::string& path)
{
    // Set the default Health to Critical
    asyncResp->res.jsonValue["Status"]["Health"] = "Critical";

    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const std::variant<bool>& health) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "Can't get Fan health!";
                messages::internalError(asyncResp->res);
                return;
            }

            const bool* value = std::get_if<bool>(&health);
            if (value == nullptr)
            {
                // illegal property
                messages::internalError(asyncResp->res);
                return;
            }
            if (*value)
            {
                asyncResp->res.jsonValue["Status"]["Health"] = "OK";
            }
        },
        connectionName, path, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.State.Decorator.OperationalStatus", "Functional");
}

inline void getFanSpeedPercent(const std::shared_ptr<AsyncResp>& asyncResp,
                               const std::string& connectionName,
                               const std::string& path,
                               const std::string& chassisId,
                               const std::string& fanId)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, chassisId, fanId](const boost::system::error_code ec,
                                      const std::variant<double>& value) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "Can't get Fan speed!";
                if (ec.value() == boost::system::errc::io_error)
                {
                    messages::resourceNotFound(asyncResp->res, "Chassis",
                                               chassisId);
                    return;
                }
                messages::internalError(asyncResp->res);
                return;
            }

            const double* attributeValue = std::get_if<double>(&value);
            if (attributeValue == nullptr)
            {
                // illegal property
                messages::internalError(asyncResp->res);
                return;
            }
            std::string tempPath =
                "/redfish/v1/Chassis/" + chassisId + "/Sensors/";
            asyncResp->res.jsonValue["SpeedPercent"]["Reading"] =
                *attributeValue;
            asyncResp->res.jsonValue["SpeedPercent"]["DataSourceUri"] =
                tempPath + fanId + "/";
            asyncResp->res.jsonValue["SpeedPercent"]["@odata.id"] =
                tempPath + fanId + "/";
        },
        connectionName, path, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Sensor.Value", "Value");
}

inline void getFanInfo(const std::shared_ptr<AsyncResp>& asyncResp,
                       const std::string& chassisId, const std::string& fanId)
{
    BMCWEB_LOG_DEBUG << "Get properties for getFan associated to chassis = "
                     << chassisId << " fan = " << fanId;
    crow::connections::systemBus->async_method_call(
        [asyncResp, chassisId, fanId](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                if (ec.value() == boost::system::errc::io_error)
                {
                    messages::resourceNotFound(asyncResp->res, "Chassis",
                                               chassisId);
                    return;
                }
                messages::internalError(asyncResp->res);
                return;
            }

            for (const auto& tempObject : subtree)
            {
                sdbusplus::message::object_path path(tempObject.first);
                const std::string& tempFanId = path.filename();
                if (tempFanId.empty())
                {
                    continue;
                }
                const std::string& tempPath = tempObject.first + "/";
                const std::string& connectionName = tempObject.second[0].first;

                crow::connections::systemBus->async_method_call(
                    [asyncResp, fanId, tempPath, connectionName](
                        const boost::system::error_code ec,
                        const std::variant<std::vector<std::string>>&
                            property) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG << "DBUS response error";
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        auto* values =
                            std::get_if<std::vector<std::string>>(&property);
                        if (values == nullptr)
                        {
                            // illegal property
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        for (const auto& value : *values)
                        {
                            sdbusplus::message::object_path path(value);
                            const std::string& leaf = path.filename();
                            if (leaf.empty())
                            {
                                continue;
                            }

                            if (leaf == fanId)
                            {
                                // Get fan state
                                getFanState(asyncResp, connectionName,
                                            tempPath);
                                // Get fan health
                                getFanHealth(asyncResp, connectionName,
                                             tempPath);
                            }
                        }
                    },
                    "xyz.openbmc_project.ObjectMapper", tempPath,
                    "org.freedesktop.DBus.Properties", "Get",
                    "xyz.openbmc_project.Association", "endpoints");
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0,
        std::array<const char*, 1>{"xyz.openbmc_project.Inventory.Item.Fan"});
}

inline void getFanSpeed(const std::shared_ptr<AsyncResp>& asyncResp,
                        const std::string& chassisId, const std::string& fanId)
{
    BMCWEB_LOG_DEBUG << "Get properties for getFan associated to chassis = "
                     << chassisId << fanId;
    const std::array<std::string, 1> interfaces = {
        "xyz.openbmc_project.Sensor.Value"};
    crow::connections::systemBus->async_method_call(
        [asyncResp, chassisId, fanId](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                subtree) {
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

            for (const auto& object : subtree)
            {
                sdbusplus::message::object_path path(object.first);
                const std::string& leaf = path.filename();
                if (leaf.empty())
                {
                    continue;
                }
                if (leaf == fanId)
                {
                    const std::string& tempPath = object.first;
                    const std::string& connectionName = object.second[0].first;
                    // Get fan state
                    getFanSpeedPercent(asyncResp, connectionName, tempPath,
                                       chassisId, fanId);
                }
                else
                {
                    BMCWEB_LOG_ERROR << "This is not a fan-related sensor  "
                                     << leaf;
                    continue;
                }
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/sensors", 2, interfaces);
}

class FanCollection : public Node
{
  public:
    /*
     * Default Constructor
     */
    FanCollection(App& app) :
        Node(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/Fans/",
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string& chassisId = params[0];

        auto getChassisPath = [asyncResp,
                               chassisId](const std::optional<std::string>&
                                              chassisPath) {
            if (!chassisPath)
            {
                BMCWEB_LOG_ERROR << "Not a valid chassis ID" << chassisId;
                messages::resourceNotFound(asyncResp->res, "Chassis",
                                           chassisId);
                return;
            }
            asyncResp->res.jsonValue = {
                {"@odata.type", "#FanCollection.FanCollection"},
                {"@odata.id", "/redfish/v1/Chassis/" + chassisId +
                                  "/ThermalSubsystem/Fans/"},
                {"Name", "Fan Collection"},
                {"Description",
                 "The collection of Fan resource instances " + chassisId}};
            crow::connections::systemBus->async_method_call(
                [asyncResp,
                 chassisId](const boost::system::error_code ec,
                            const std::vector<std::string>& subtreepaths) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG << "DBUS response error";
                        if (ec.value() == boost::system::errc::io_error)
                        {
                            messages::resourceNotFound(asyncResp->res,
                                                       "Chassis", chassisId);
                            return;
                        }
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    nlohmann::json& fanList =
                        asyncResp->res.jsonValue["Members"];
                    fanList = nlohmann::json::array();

                    std::string newPath = "/redfish/v1/Chassis/" + chassisId +
                                          "/ThermalSubsystem/Fans/";

                    for (const auto& tempObject : subtreepaths)
                    {
                        sdbusplus::message::object_path path(tempObject);
                        const std::string& tempLeaf = path.filename();
                        if (tempLeaf.empty())
                        {
                            continue;
                        }
                        const std::string& tempPath = tempObject + "/sensors";

                        crow::connections::systemBus->async_method_call(
                            [asyncResp, chassisId, &fanList, newPath](
                                const boost::system::error_code ec,
                                const std::variant<std::vector<std::string>>&
                                    property) {
                                if (ec)
                                {
                                    BMCWEB_LOG_DEBUG << "DBUS response error";
                                    if (ec.value() ==
                                        boost::system::errc::io_error)
                                    {
                                        messages::resourceNotFound(
                                            asyncResp->res, "Chassis",
                                            chassisId);
                                        return;
                                    }
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                auto* values =
                                    std::get_if<std::vector<std::string>>(
                                        &property);
                                if (values == nullptr)
                                {
                                    // illegal property
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                for (const auto& value : *values)
                                {
                                    sdbusplus::message::object_path path(value);
                                    const std::string& fanName =
                                        path.filename();
                                    if (fanName.empty())
                                    {
                                        continue;
                                    }
                                    fanList.push_back(
                                        {{"@odata.id",
                                          newPath + fanName + "/"}});
                                }
                                asyncResp->res
                                    .jsonValue["Members@odata.count"] =
                                    fanList.size();
                            },
                            "xyz.openbmc_project.ObjectMapper", tempPath,
                            "org.freedesktop.DBus.Properties", "Get",
                            "xyz.openbmc_project.Association", "endpoints");
                    }
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
                "/xyz/openbmc_project/inventory", 0,
                std::array<const char*, 1>{
                    "xyz.openbmc_project.Inventory.Item.Fan"});
        };
        redfish::chassis_utils::getValidChassisID(asyncResp, chassisId,
                                                  std::move(getChassisPath));
    }
};

class Fan : public Node
{
  public:
    /*
     * Default Constructor
     */
    Fan(App& app) :
        Node(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/Fans/<str>/",
             std::string(), std::string())
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 2)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string& chassisId = params[0];
        const std::string& fanId = params[1];

        auto getChassisPath = [asyncResp, chassisId,
                               fanId](const std::optional<std::string>&
                                          chassisPath) {
            if (!chassisPath)
            {
                BMCWEB_LOG_ERROR << "Not a valid chassis ID" << chassisId;
                messages::resourceNotFound(asyncResp->res, "Chassis",
                                           chassisId);
                return;
            }
            auto getFanPath = [asyncResp, chassisId, fanId](
                                  const std::optional<std::string>& fanPath) {
                if (!fanPath)
                {
                    BMCWEB_LOG_ERROR << "Not a valid fan ID" << fanId;
                    messages::resourceNotFound(asyncResp->res, "Fan", fanId);
                    return;
                }

                std::string newPath = "/redfish/v1/Chassis/" + chassisId +
                                      "/ThermalSubsystem/Fans/";
                asyncResp->res.jsonValue["@odata.type"] = "#Fan.v1_0_0.Fan";
                asyncResp->res.jsonValue["Name"] = fanId;
                asyncResp->res.jsonValue["Id"] = fanId;
                asyncResp->res.jsonValue["@odata.id"] = newPath + fanId + "/";
                // Get fan supply asset and status
                getFanInfo(asyncResp, chassisId, fanId);
                // Get fan speed
                getFanSpeed(asyncResp, chassisId, fanId);
            };
            getValidFanID(asyncResp, fanId, std::move(getFanPath));
        };
        redfish::chassis_utils::getValidChassisID(asyncResp, chassisId,
                                                  std::move(getChassisPath));
    }
};

} // namespace redfish