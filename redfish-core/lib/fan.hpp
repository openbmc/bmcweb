#pragma once

#include <app.hpp>
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
            if (value == nullptr)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            if (*value == false)
            {
                asyncResp->res.jsonValue["Status"]["State"] = "Absent";
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
            if (value == nullptr)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            if (*value == false)
            {
                asyncResp->res.jsonValue["Status"]["State"] = "Critical";
            }
        },
        connectionName, path, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.State.Decorator.OperationalStatus", "Functional");
}

inline void getFanAsset(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& connectionName,
                        const std::string& path)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const std::vector<
                        std::pair<std::string, std::variant<std::string>>>&
                        propertiesList) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Can't get fan asset! Not implemented";
                return;
            }
            for (const std::pair<std::string, std::variant<std::string>>&
                     property : propertiesList)
            {
                const std::string& propertyName = property.first;

                if ((propertyName == "PartNumber") ||
                    (propertyName == "SerialNumber") ||
                    (propertyName == "Model") ||
                    (propertyName == "SparePartNumber") ||
                    (propertyName == "Manufacturer"))
                {
                    const std::string* value =
                        std::get_if<std::string>(&property.second);
                    if (value == nullptr)
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    asyncResp->res.jsonValue[propertyName] = *value;
                }
            }
        },
        connectionName, path, "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Inventory.Decorator.Asset");
}

inline void getFanLocation(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                           const std::string& connectionName,
                           const std::string& path)
{
    const std::string locationInterface =
        "xyz.openbmc_project.Inventory.Decorator.LocationCode";
    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                const std::variant<std::string>& property) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "Can't get fan Location! Not implemented";
                return;
            }

            const std::string* value = std::get_if<std::string>(&property);

            if (value == nullptr)
            {
                // illegal value
                messages::internalError(aResp->res);
                return;
            }

            aResp->res.jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
                *value;
        },
        connectionName, path, "org.freedesktop.DBus.Properties", "Get",
        locationInterface, "LocationCode");
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

inline void
    getFanSpecificInfo(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& fanPath)
{

    const std::array<const char*, 1> fanInterfaces = {
        "xyz.openbmc_project.Inventory.Item.Fan"};
    crow::connections::systemBus->async_method_call(
        [asyncResp, fanPath](
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
                getFanState(asyncResp, connectionName, fanPath);
                getFanHealth(asyncResp, connectionName, fanPath);
                getFanAsset(asyncResp, connectionName, fanPath);
                getFanLocation(asyncResp, connectionName, fanPath);
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", fanPath,
        fanInterfaces);
}

inline void getFanInfo(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& chassisId, const std::string& fanId,
                       const std::string& path,
                       const std::string& connectionName)
{
    BMCWEB_LOG_DEBUG << "Get properties for getFan associated to chassis = "
                     << chassisId << " fan = " << fanId;
    if (connectionName == "xyz.openbmc_project.PLDM")
    {
        getFanState(asyncResp, connectionName, path);
        getFanHealth(asyncResp, connectionName, path);
        getFanAsset(asyncResp, connectionName, path);
        getFanLocation(asyncResp, connectionName, path);
    }
    else
    {
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
                            asyncResp->res,
                            "fan inventory item,fanId = ", fanId);
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
                    if (leaf != fanId)
                    {
                        continue;
                    }
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
                                // Add this function to make the code easy to
                                // read
                                getFanSpecificInfo(asyncResp, fanPath);
                            }
                        },
                        "xyz.openbmc_project.ObjectMapper", fanAssociationPath,
                        "org.freedesktop.DBus.Properties", "Get",
                        "xyz.openbmc_project.Association", "endpoints");
                }
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
            "/xyz/openbmc_project/sensors", 0, sensorInterfaces);
    }
}

inline void getFanSpeed(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& chassisId, const std::string& fanId,
                        const std::string& connectionName)
{
    BMCWEB_LOG_DEBUG << "Get properties for getFan associated to chassis = "
                     << chassisId << " fan = " << fanId;
    if (connectionName == "xyz.openbmc_project.PLDM")
    {
        BMCWEB_LOG_DEBUG
            << "There is no fan speed related to the MEX chassis, chassis = "
            << chassisId << " fan = " << fanId;
        return;
    }

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
                messages::internalError(asyncResp->res);
                return;
            }

            for (const auto& [objectPath, serviceNames] : sensorsubtree)
            {
                if (objectPath.empty() || serviceNames.size() != 1)
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
                if (leaf != fanId)
                {
                    continue;
                }

                const std::string& tempPath = objectPath;
                const std::string& connectionName = serviceNames[0].first;
                getFanSpeedPercent(asyncResp, connectionName, tempPath,
                                   chassisId, fanId);
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/sensors", 0, sensorInterfaces);
}

inline void
    getFanInventoryItem(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& chassisId, const std::string& fanId,
                        const std::string& path,
                        const std::string& connectionName)
{
    BMCWEB_LOG_DEBUG << "Get inventory Item for getFan associated to chassis = "
                     << chassisId << " fan = " << fanId;
    getFanInfo(asyncResp, chassisId, fanId, path, connectionName);
    getFanSpeed(asyncResp, chassisId, fanId, connectionName);
}

inline void getValidFan(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& chassisId)
{
    BMCWEB_LOG_DEBUG << "Get fan list associated to chassis = " << chassisId;
    const std::array<const char*, 1> fanInterfaces = {
        "xyz.openbmc_project.Inventory.Item.Fan"};
    asyncResp->res.jsonValue["@odata.type"] = "#FanCollection.FanCollection";
    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Chassis/" + chassisId + "/ThermalSubsystem/Fans/";
    asyncResp->res.jsonValue["Name"] = "Fan Collection";
    asyncResp->res.jsonValue["Description"] =
        "The collection of Fan resource instances " + chassisId;
    crow::connections::systemBus->async_method_call(
        [asyncResp, chassisId](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                fansubtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(asyncResp->res);
                return;
            }

            nlohmann::json& fanList = asyncResp->res.jsonValue["Members"];
            fanList = nlohmann::json::array();

            std::string newPath =
                "/redfish/v1/Chassis/" + chassisId + "/ThermalSubsystem/Fans/";

            for (const auto& [objectPath, serviceName] : fansubtree)
            {
                if (objectPath.empty() || serviceName.size() != 1)
                {
                    BMCWEB_LOG_DEBUG << "Error getting D-Bus object!";
                    messages::internalError(asyncResp->res);
                    return;
                }
                const std::string& validPath = objectPath;
                const std::string& connectionName = serviceName[0].first;
                std::string chassisName;
                std::string fanName;

                if (connectionName == "xyz.openbmc_project.PLDM")
                {

                    // 5 below comes from
                    // /xyz/openbmc_project/inventory/system/chassisName/fanName
                    //   0      1             2        3        4         5
                    if (!dbus::utility::getNthStringFromPath(validPath, 4,
                                                             chassisName) ||
                        !dbus::utility::getNthStringFromPath(validPath, 5,
                                                             fanName))
                    {
                        BMCWEB_LOG_ERROR << "Got invalid path " << validPath;
                        messages::invalidObject(asyncResp->res, validPath);
                        continue;
                    }
                    if (chassisName != chassisId)
                    {
                        BMCWEB_LOG_ERROR << "The fan obtained at this time "
                                            "does not belong to this chassis ";
                        continue;
                    }
                    fanList.push_back({{"@odata.id", newPath + fanName + "/"}});
                    asyncResp->res.jsonValue["Members@odata.count"] =
                        fanList.size();
                }
                else
                {
                    // 6 below comes from
                    // /xyz/openbmc_project/inventory/system/chassisName/motherboard/fanName
                    //   0      1             2         3         4         5 6
                    if (!dbus::utility::getNthStringFromPath(validPath, 4,
                                                             chassisName))
                    {
                        BMCWEB_LOG_ERROR << "Got invalid path " << validPath;
                        messages::invalidObject(asyncResp->res, validPath);
                        continue;
                    }
                    if (chassisName != chassisId)
                    {
                        BMCWEB_LOG_ERROR << "The fan obtained at this time "
                                            "does not belong to this chassis ";
                        continue;
                    }
                    const std::string& fanSensorPath = validPath + "/sensors";
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
                            auto* fanSensorEndpoints =
                                std::get_if<std::vector<std::string>>(
                                    &property);
                            if (fanSensorEndpoints == nullptr)
                            {
                                // illegal property
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            for (const auto& fanSensorEndpoint :
                                 *fanSensorEndpoints)
                            {
                                const std::string& tempPath =
                                    fanSensorEndpoint + "/chassis";
                                sdbusplus::message::object_path path(
                                    fanSensorEndpoint);
                                const std::string& fanName = path.filename();
                                if (fanName.empty())
                                {
                                    continue;
                                }
                                // The association of this fan is used to
                                // determine whether it belongs to this
                                // ChassisId
                                crow::connections::systemBus->async_method_call(
                                    [asyncResp, chassisId, fanName, &fanList,
                                     newPath](
                                        const boost::system::error_code ec,
                                        const std::variant<std::vector<
                                            std::string>>& property) {
                                        if (ec)
                                        {
                                            BMCWEB_LOG_DEBUG
                                                << "DBUS response error";
                                            if (ec.value() == EBADR)
                                            {
                                                // This fan have no chassis
                                                // association
                                                return;
                                            }
                                            messages::internalError(
                                                asyncResp->res);
                                            return;
                                        }
                                        auto* fanSensorChassis = std::get_if<
                                            std::vector<std::string>>(
                                            &property);
                                        if (fanSensorChassis == nullptr)
                                        {
                                            messages::internalError(
                                                asyncResp->res);
                                            return;
                                        }
                                        if ((*fanSensorChassis).size() != 1)
                                        {
                                            BMCWEB_LOG_DEBUG
                                                << "Fan association error! ";
                                            messages::internalError(
                                                asyncResp->res);
                                            return;
                                        }
                                        std::vector<std::string> chassisPath =
                                            *fanSensorChassis;
                                        sdbusplus::message::object_path path(
                                            chassisPath[0]);
                                        const std::string& chassisName =
                                            path.filename();
                                        if (chassisName != chassisId)
                                        {
                                            // The Fan does't belong to the
                                            // chassisId
                                            return;
                                        }
                                        fanList.push_back(
                                            {{"@odata.id",
                                              newPath + fanName + "/"}});
                                        asyncResp->res
                                            .jsonValue["Members@odata.count"] =
                                            fanList.size();
                                    },
                                    "xyz.openbmc_project.ObjectMapper",
                                    tempPath, "org.freedesktop.DBus.Properties",
                                    "Get", "xyz.openbmc_project.Association",
                                    "endpoints");
                            }
                        },
                        "xyz.openbmc_project.ObjectMapper", fanSensorPath,
                        "org.freedesktop.DBus.Properties", "Get",
                        "xyz.openbmc_project.Association", "endpoints");
                }
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0, fanInterfaces);
}

template <typename Callback>
inline void getValidfanId(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& chassisId,
                          const std::string& fanId, Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "getValidFanId enter";

    auto respHandler = [callback{std::move(callback)}, asyncResp, chassisId,
                        fanId](
                           const boost::system::error_code ec,
                           const std::vector<std::pair<
                               std::string,
                               std::vector<std::pair<
                                   std::string, std::vector<std::string>>>>>&
                               subtree) {
        BMCWEB_LOG_DEBUG << "getValidfanId respHandler enter";

        if (ec)
        {
            BMCWEB_LOG_ERROR << "getValidfanId respHandler DBUS error: " << ec;
            messages::internalError(asyncResp->res);
            return;
        }

        // Set the default value to resourceNotFound, and if we confirm that
        // fanId is correct, the error response will be cleared.
        messages::resourceNotFound(asyncResp->res, "fan", fanId);

        for (const auto& [objectPath, serviceNames] : subtree)
        {
            if (objectPath.empty() || serviceNames.size() != 1)
            {
                BMCWEB_LOG_DEBUG << "Error getting Fan D-Bus object!";
                messages::internalError(asyncResp->res);
                return;
            }
            const std::string& path = objectPath;
            const std::string& fanSensorPath = path + "/sensors";
            const std::string& connectionName = serviceNames[0].first;

            if (connectionName == "xyz.openbmc_project.PLDM")
            {
                crow::connections::systemBus->async_method_call(
                    [callback{std::move(callback)}, asyncResp, chassisId, fanId,
                     path, connectionName](
                        const boost::system::error_code ec,
                        const std::variant<std::vector<std::string>>&
                            endpoints) {
                        if (ec)
                        {
                            if (ec.value() == EBADR)
                            {
                                // This fan have no chassis association.
                                return;
                            }

                            BMCWEB_LOG_ERROR << "DBUS response error";
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        const std::vector<std::string>* fanChassis =
                            std::get_if<std::vector<std::string>>(&(endpoints));

                        if (fanChassis != nullptr)
                        {
                            if ((*fanChassis).size() != 1)
                            {
                                BMCWEB_LOG_ERROR << "Fan association error! ";
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            std::vector<std::string> chassisPath = *fanChassis;
                            sdbusplus::message::object_path pathChassis(
                                chassisPath[0]);
                            std::string chassisName = pathChassis.filename();
                            if (chassisName != chassisId)
                            {
                                // The Fan does't belong to the
                                // chassisId
                                return;
                            }

                            sdbusplus::message::object_path pathFan(path);
                            const std::string fanName = pathFan.filename();
                            if (fanName.empty())
                            {
                                BMCWEB_LOG_ERROR << "Failed to find fanName in "
                                                 << path;
                                return;
                            }
                            if (fanName == fanId)
                            {
                                // Clear resourceNotFound response
                                asyncResp->res.clear();
                                callback(path, connectionName);
                            }
                        }
                    },
                    "xyz.openbmc_project.ObjectMapper", path + "/chassis",
                    "org.freedesktop.DBus.Properties", "Get",
                    "xyz.openbmc_project.Association", "endpoints");
            }
            else
            {
                crow::connections::systemBus->async_method_call(
                    [callback{std::move(callback)}, asyncResp, chassisId, fanId,
                     path, connectionName](
                        const boost::system::error_code ec,
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
                        auto* fanSensorEndpoints =
                            std::get_if<std::vector<std::string>>(&property);
                        if (fanSensorEndpoints == nullptr)
                        {
                            // illegal property
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        for (const auto& fanSensorEndpoint :
                             *fanSensorEndpoints)
                        {
                            const std::string& tempPath =
                                fanSensorEndpoint + "/chassis";
                            sdbusplus::message::object_path pathFan(
                                fanSensorEndpoint);
                            const std::string& fanName = pathFan.filename();
                            if (fanName.empty())
                            {
                                continue;
                            }
                            if (fanName == fanId)
                            {
                                // The association of this fan is used to
                                // determine whether it belongs to this
                                // ChassisId
                                crow::connections::systemBus->async_method_call(
                                    [callback{std::move(callback)}, asyncResp,
                                     chassisId, path, connectionName](
                                        const boost::system::error_code ec,
                                        const std::variant<std::vector<
                                            std::string>>& endpoints) {
                                        if (ec)
                                        {
                                            BMCWEB_LOG_DEBUG
                                                << "DBUS response error";
                                            if (ec.value() == EBADR)
                                            {
                                                // This fan have no chassis
                                                // association
                                                return;
                                            }
                                            messages::internalError(
                                                asyncResp->res);
                                            return;
                                        }
                                        auto* fanSensorChassis = std::get_if<
                                            std::vector<std::string>>(
                                            &endpoints);
                                        if (fanSensorChassis == nullptr)
                                        {
                                            messages::internalError(
                                                asyncResp->res);
                                            return;
                                        }
                                        if ((*fanSensorChassis).size() != 1)
                                        {
                                            BMCWEB_LOG_DEBUG
                                                << "Fan association error! ";
                                            messages::internalError(
                                                asyncResp->res);
                                            return;
                                        }
                                        std::vector<std::string> chassisPath =
                                            *fanSensorChassis;
                                        sdbusplus::message::object_path
                                            pathChassis(chassisPath[0]);
                                        const std::string& chassisName =
                                            pathChassis.filename();
                                        if (chassisName != chassisId)
                                        {
                                            // The Fan does't belong to the
                                            // chassisId
                                            return;
                                        }
                                        // Clear resourceNotFound response
                                        asyncResp->res.clear();
                                        callback(path, connectionName);
                                    },
                                    "xyz.openbmc_project.ObjectMapper",
                                    tempPath, "org.freedesktop.DBus.Properties",
                                    "Get", "xyz.openbmc_project.Association",
                                    "endpoints");
                            }
                            if (fanName != fanId)
                            {
                                continue;
                            }
                        }
                    },
                    "xyz.openbmc_project.ObjectMapper", fanSensorPath,
                    "org.freedesktop.DBus.Properties", "Get",
                    "xyz.openbmc_project.Association", "endpoints");
            }
        }
    };

    // Get the fan Collection
    crow::connections::systemBus->async_method_call(
        respHandler, "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0,
        std::array<const char*, 1>{"xyz.openbmc_project.Inventory.Item.Fan"});
    BMCWEB_LOG_DEBUG << "getValidFanId exit";
}

inline void requestRoutesFanCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/Fans/")
        .privileges({{"Login"}})
        // TODO: Use automated PrivilegeRegistry
        // Need to wait for Redfish to release a new registry
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisId) {
                auto getChassisId =
                    [asyncResp,
                     chassisId](const std::optional<std::string>& chassisPath) {
                        if (!chassisPath)
                        {
                            BMCWEB_LOG_ERROR << "Not a valid chassis ID"
                                             << chassisId;
                            messages::resourceNotFound(asyncResp->res,
                                                       "Chassis", chassisId);
                            return;
                        }
                        getValidFan(asyncResp, chassisId);
                    };
                redfish::chassis_utils::getValidChassisID(
                    asyncResp, chassisId, std::move(getChassisId));
            });
}

inline void requestRoutesFan(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/Fans/<str>/")
        .privileges({{"Login"}})
        // TODO: Use automated PrivilegeRegistry
        // Need to wait for Redfish to release a new registry
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisId, const std::string& fanId) {
                auto getChassisId =
                    [asyncResp, chassisId,
                     fanId](const std::optional<std::string>& chassisPath) {
                        if (!chassisPath)
                        {
                            BMCWEB_LOG_ERROR << "Not a valid chassis ID"
                                             << chassisId;
                            messages::resourceNotFound(asyncResp->res,
                                                       "Chassis", chassisId);
                            return;
                        }
                        auto getFanId =
                            [asyncResp, chassisId,
                             fanId](const std::string& validFanPath,
                                    const std::string& validFanService) {
                                std::string newPath = "/redfish/v1/Chassis/" +
                                                      chassisId +
                                                      "/ThermalSubsystem/Fans/";
                                asyncResp->res.jsonValue["@odata.type"] =
                                    "#Fan.v1_0_0.Fan";
                                asyncResp->res.jsonValue["Name"] = fanId;
                                asyncResp->res.jsonValue["Id"] = fanId;
                                asyncResp->res.jsonValue["@odata.id"] =
                                    newPath + fanId + "/";
                                getFanInventoryItem(asyncResp, chassisId, fanId,
                                                    validFanPath,
                                                    validFanService);
                            };
                        // Verify that the fan has the correct chassis and
                        // whether fan has a chassis association
                        getValidfanId(asyncResp, chassisId, fanId,
                                      std::move(getFanId));
                    };
                redfish::chassis_utils::getValidChassisID(
                    asyncResp, chassisId, std::move(getChassisId));
            });
}

} // namespace redfish