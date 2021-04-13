#pragma once

#include <node.hpp>
#include <utils/chassis_utils.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

inline void getFanState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& connectionName,
                        const std::string& path)
{
    // Set the default state to Enabled
    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

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
            if (value != nullptr)
            {
                if (*value == false)
                {
                    asyncResp->res.jsonValue["Status"]["State"] = "Absent";
                }
            }
        },
        connectionName, path, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Inventory.Item", "Present");
}

inline void getFanHealth(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& connectionName,
                         const std::string& path)
{
    // Set the default Health to OK
    asyncResp->res.jsonValue["Status"]["Health"] = "OK";

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
            if (value != nullptr)
            {
                if (*value == false)
                {
                    asyncResp->res.jsonValue["Status"]["Health"] = "Critical";
                }
            }
        },
        connectionName, path, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.State.Decorator.OperationalStatus", "Functional");
}

inline void
    getFanSpeedPercent(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& connectionName,
                       const std::string& path, const std::string& chassisId,
                       const std::string& fanId)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, chassisId, fanId](const boost::system::error_code ec,
                                      const std::variant<double>& value) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "Can't get Fan speed!";
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

inline void getFanInfo(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& chassisId, const std::string& fanId)
{
    BMCWEB_LOG_DEBUG << "Get properties for getFan associated to chassis = "
                     << chassisId << " fan = " << fanId;
    const std::array<std::string, 1> sensorInterfaces = {
        "xyz.openbmc_project.Sensor.Value"};
    crow::connections::systemBus->async_method_call(
        [asyncResp, fanId,
         chassisId](const boost::system::error_code ec,
                    const std::vector<std::string>& sensorpaths) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                if (ec.value() == boost::system::errc::io_error)
                {
                    messages::resourceNotFound(
                        asyncResp->res, "fan inventory item,fanId = ", fanId);
                    return;
                }
                messages::internalError(asyncResp->res);
                return;
            }

            for (const auto& tempsensorpath : sensorpaths)
            {
                sdbusplus::message::object_path path(tempsensorpath);
                const std::string& leaf = path.filename();
                if (leaf.empty())
                {
                    continue;
                }
                if (leaf == fanId)
                {
                    const std::string& fanAssociationPath =
                        tempsensorpath + "/inventory";

                    crow::connections::systemBus->async_method_call(
                        [asyncResp,
                         fanId](const boost::system::error_code ec,
                                const std::variant<std::vector<std::string>>&
                                    property) {
                            if (ec)
                            {
                                BMCWEB_LOG_DEBUG << "DBUS response error";
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            auto* values =
                                std::get_if<std::vector<std::string>>(
                                    &property);
                            if (values == nullptr)
                            {
                                // illegal property
                                BMCWEB_LOG_DEBUG
                                    << "No endpoints, skipping get fan ";
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            for (const auto& fanPath : *values)
                            {
                                const std::array<const char*, 1> fanInterfaces =
                                    {"xyz.openbmc_project.Inventory.Item.Fan"};
                                crow::connections::systemBus->async_method_call(
                                    [asyncResp, fanId, fanPath](
                                        const boost::system::error_code ec,
                                        const std::vector<std::pair<
                                            std::string,
                                            std::vector<std::string>>>&
                                            object) {
                                        if (ec)
                                        {
                                            BMCWEB_LOG_DEBUG << "DBUS "
                                                                "response "
                                                                "error";
                                            messages::internalError(
                                                asyncResp->res);
                                            return;
                                        }
                                        for (const auto& tempObject : object)
                                        {
                                            const std::string& connectionName =
                                                tempObject.first;
                                            // Get fan state
                                            getFanState(asyncResp,
                                                        connectionName,
                                                        fanPath);
                                            // Get fan health
                                            getFanHealth(asyncResp,
                                                         connectionName,
                                                         fanPath);
                                        }
                                    },
                                    "xyz.openbmc_project.ObjectMapper",
                                    "/xyz/openbmc_project/object_mapper",
                                    "xyz.openbmc_project.ObjectMapper",
                                    "GetObject", fanPath, fanInterfaces);
                            }
                        },
                        "xyz.openbmc_project.ObjectMapper", fanAssociationPath,
                        "org.freedesktop.DBus.Properties", "Get",
                        "xyz.openbmc_project.Association", "endpoints");
                }
                else
                {
                    BMCWEB_LOG_ERROR << "This is not a fan-related sensor  "
                                     << leaf;
                    continue;
                }
                return;
            }
            asyncResp->res.clear();
            messages::resourceNotFound(asyncResp->res, "fan", fanId);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/sensors", 0, sensorInterfaces);
}

inline void getFanSpeed(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& chassisId, const std::string& fanId)
{
    BMCWEB_LOG_DEBUG << "Get properties for getFan associated to chassis = "
                     << chassisId << " fan = " << fanId;
    const std::array<std::string, 1> sensorInterfaces = {
        "xyz.openbmc_project.Sensor.Value"};
    crow::connections::systemBus->async_method_call(
        [asyncResp, chassisId, fanId](
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

            for (const auto& [objectPath, serviceName] : sensorsubtree)
            {
                if (objectPath.empty() || serviceName.size() != 1)
                {
                    BMCWEB_LOG_DEBUG << "Error getting Fan D-Bus object!";
                    messages::internalError(asyncResp->res);
                    return;
                }
                sdbusplus::message::object_path path(objectPath);
                const std::string& leaf = path.filename();
                if (leaf.empty())
                {
                    continue;
                }
                if (leaf == fanId)
                {
                    const std::string& tempPath = objectPath;
                    const std::string& connectionName = serviceName[0].first;
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
                return;
            }
            asyncResp->res.clear();
            messages::resourceNotFound(asyncResp->res, "fan", fanId);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/sensors", 0, sensorInterfaces);
}

inline void
    getFanInventoryItem(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& chassisId, const std::string& fanId)
{
    BMCWEB_LOG_DEBUG << "Get inventory Item for getFan associated to chassis = "
                     << chassisId << " fan = " << fanId;
    // Get fan status
    getFanInfo(asyncResp, chassisId, fanId);
    // Get fan speed
    getFanSpeed(asyncResp, chassisId, fanId);
}

inline void getFan(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& chassisId)
{
    BMCWEB_LOG_DEBUG << "Get fan list associated to chassis = " << chassisId;
    asyncResp->res.jsonValue = {
        {"@odata.type", "#FanCollection.FanCollection"},
        {"@odata.id",
         "/redfish/v1/Chassis/" + chassisId + "/ThermalSubsystem/Fans/"},
        {"Name", "Fan Collection"},
        {"Description",
         "The collection of Fan resource instances " + chassisId}};
    crow::connections::systemBus->async_method_call(
        [asyncResp,
         chassisId](const boost::system::error_code ec,
                    const std::vector<std::string>& fanInventorypaths) {
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

            nlohmann::json& fanList = asyncResp->res.jsonValue["Members"];
            fanList = nlohmann::json::array();

            std::string newPath =
                "/redfish/v1/Chassis/" + chassisId + "/ThermalSubsystem/Fans/";

            for (const auto& tempObject : fanInventorypaths)
            {
                sdbusplus::message::object_path path(tempObject);
                const std::string& tempLeaf = path.filename();
                if (tempLeaf.empty())
                {
                    continue;
                }
                const std::string& fansensorPath = tempObject + "/sensors";

                crow::connections::systemBus->async_method_call(
                    [asyncResp, chassisId, &fanList,
                     newPath](const boost::system::error_code ec,
                              const std::variant<std::vector<std::string>>&
                                  property) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG << "DBUS response error";
                            if (ec.value() == boost::system::errc::io_error)
                            {
                                messages::resourceNotFound(
                                    asyncResp->res, "Chassis", chassisId);
                                return;
                            }
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
                            const std::string& tempPath = value + "/chassis";
                            sdbusplus::message::object_path path(value);
                            const std::string& fanName = path.filename();
                            // The association of this fan is used to determine
                            // whether it belongs to this ChassisId
                            crow::connections::systemBus->async_method_call(
                                [asyncResp, chassisId, fanName, &fanList,
                                 newPath](
                                    const boost::system::error_code ec,
                                    const std::variant<
                                        std::vector<std::string>>& property) {
                                    if (ec)
                                    {
                                        BMCWEB_LOG_DEBUG
                                            << "DBUS response error";
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
                                    if (values != nullptr)
                                    {
                                        if ((*values).size() != 1)
                                        {
                                            BMCWEB_LOG_DEBUG
                                                << "Fan association error! ";
                                            messages::internalError(
                                                asyncResp->res);
                                            return;
                                        }
                                        for (const auto& value : *values)
                                        {
                                            sdbusplus::message::object_path
                                                path(value);
                                            const std::string& chassisName =
                                                path.filename();
                                            if (chassisName != chassisId)
                                            {
                                                // The Fan does't belong to the
                                                // chassisId
                                                return;
                                            }
                                            if (fanName.empty())
                                            {
                                                continue;
                                            }
                                            fanList.push_back(
                                                {{"@odata.id",
                                                  newPath + fanName + "/"}});
                                        }
                                    }
                                },
                                "xyz.openbmc_project.ObjectMapper", tempPath,
                                "org.freedesktop.DBus.Properties", "Get",
                                "xyz.openbmc_project.Association", "endpoints");
                        }
                        asyncResp->res.jsonValue["Members@odata.count"] =
                            fanList.size();
                    },
                    "xyz.openbmc_project.ObjectMapper", fansensorPath,
                    "org.freedesktop.DBus.Properties", "Get",
                    "xyz.openbmc_project.Association", "endpoints");
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/inventory", 0,
        std::array<const char*, 1>{"xyz.openbmc_project.Inventory.Item.Fan"});
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
    void doGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const crow::Request&,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string& chassisId = params[0];

        auto getChassisId = [asyncResp, chassisId](
                                const std::optional<std::string>& chassisPath) {
            if (!chassisPath)
            {
                BMCWEB_LOG_ERROR << "Not a valid chassis ID" << chassisId;
                messages::resourceNotFound(asyncResp->res, "Chassis",
                                           chassisId);
                return;
            }
            getFan(asyncResp, chassisId);
        };
        redfish::chassis_utils::getValidChassisID(asyncResp, chassisId,
                                                  std::move(getChassisId));
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
    void doGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const crow::Request&,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 2)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string& chassisId = params[0];
        const std::string& fanId = params[1];

        auto getChassisId = [asyncResp, chassisId, fanId](
                                const std::optional<std::string>& chassisPath) {
            if (!chassisPath)
            {
                BMCWEB_LOG_ERROR << "Not a valid chassis ID" << chassisId;
                messages::resourceNotFound(asyncResp->res, "Chassis",
                                           chassisId);
                return;
            }
            std::string newPath =
                "/redfish/v1/Chassis/" + chassisId + "/ThermalSubsystem/Fans/";
            asyncResp->res.jsonValue["@odata.type"] = "#Fan.v1_0_0.Fan";
            asyncResp->res.jsonValue["Name"] = fanId;
            asyncResp->res.jsonValue["Id"] = fanId;
            asyncResp->res.jsonValue["@odata.id"] = newPath + fanId + "/";
            // Get fan inventory item
            getFanInventoryItem(asyncResp, chassisId, fanId);
        };
        redfish::chassis_utils::getValidChassisID(asyncResp, chassisId,
                                                  std::move(getChassisId));
    }
};

} // namespace redfish