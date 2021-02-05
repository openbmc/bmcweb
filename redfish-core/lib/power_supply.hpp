#pragma once

#include <app.hpp>
#include <registries/privilege_registry.hpp>
#include <utils/chassis_utils.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

inline void
    getPowerSupplyAsset(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
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
            if (ec.value() == EBADR)
            {
                return;
            }
            BMCWEB_LOG_ERROR << "Can't get PowerSupply asset!";
            messages::internalError(asyncResp->res);
            return;
        }
        for (const std::pair<std::string, std::variant<std::string>>& property :
             propertiesList)
        {
            const std::string& propertyName = property.first;

            if ((propertyName == "PartNumber") ||
                (propertyName == "SerialNumber") || (propertyName == "Model") ||
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

inline void getPowerSupplyFirmwareVersion(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& connectionName, const std::string& path)
{

    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, connectionName, path,
        "xyz.openbmc_project.Software.Version", "Version",
        [asyncResp](const boost::system::error_code ec,
                    const std::string& value) {
        if (ec)
        {
            if (ec.value() == EBADR)
            {
                return;
            }

            BMCWEB_LOG_ERROR << "Can't get PowerSupply firmware version!";
            messages::internalError(asyncResp->res);
            return;
        }

        asyncResp->res.jsonValue["FirmwareVersion"] = value;
        });
}

inline void
    getPowerSupplyLocation(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
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
            if (ec.value() == EBADR)
            {
                return;
            }
            BMCWEB_LOG_ERROR << "Can't get PowerSupply location!";
            messages::internalError(asyncResp->res);
            return;
        }
        for (const std::pair<std::string, std::variant<std::string>>& property :
             propertiesList)
        {
            const std::string& propertyName = property.first;

            if (propertyName == "LocationCode")
            {
                const std::string* value =
                    std::get_if<std::string>(&property.second);
                if (value == nullptr)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                asyncResp->res
                    .jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
                    *value;
            }
        }
        },
        connectionName, path, "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Inventory.Decorator.LocationCode");
}

inline void
    getPowerSupplyState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& connectionName,
                        const std::string& path)
{
    // Set the default state to Absent
    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, connectionName, path,
        "xyz.openbmc_project.Inventory.Item", "Present",
        [asyncResp](const boost::system::error_code ec, const bool state) {
        if (ec)
        {
            if (ec.value() == EBADR)
            {
                return;
            }

            BMCWEB_LOG_ERROR << "Can't get PowerSupply state!";
            messages::internalError(asyncResp->res);
            return;
        }

        if (!state)
        {
            asyncResp->res.jsonValue["Status"]["State"] = "Absent";
        }
        });
}

inline void
    getPowerSupplyHealth(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& connectionName,
                         const std::string& path)
{
    // Set the default Health to OK
    asyncResp->res.jsonValue["Status"]["Health"] = "OK";

    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, connectionName, path,
        "xyz.openbmc_project.State.Decorator.OperationalStatus", "Functional",
        [asyncResp](const boost::system::error_code ec, const bool health) {
        if (ec)
        {
            if (ec.value() == EBADR)
            {
                return;
            }

            BMCWEB_LOG_ERROR << "Can't get PowerSupply health!";
            messages::internalError(asyncResp->res);
            return;
        }

        if (!health)
        {
            asyncResp->res.jsonValue["Status"]["Health"] = "Critical";
        }
        });
}

inline void
    getEfficiencyRatings(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    // Gets the Power Supply Attributes such as EfficiencyPercent.
    // Currently we only support one power supply EfficiencyPercent, use
    // this for all the power supplies.
    crow::connections::systemBus->async_method_call(
        [asyncResp](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                subtree) {
        if (ec)
        {
            if (ec.value() == EBADR)
            {
                return;
            }
            BMCWEB_LOG_ERROR
                << "Get PowerSupply attributes respHandler DBus error " << ec;
            messages::internalError(asyncResp->res);
            return;
        }
        if (subtree.empty())
        {
            BMCWEB_LOG_DEBUG << "Can't find Power Supply efficiency ratings!";
            return;
        }
        if (subtree[0].second.empty())
        {
            BMCWEB_LOG_ERROR << "Get Power Supply efficiency ratings error!";
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& psAttributesPath = subtree[0].first;
        const std::string& connection = subtree[0].second[0].first;

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const std::variant<uint32_t>& deratingFactor) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Get PowerSupply DeratingFactor "
                                    "respHandler DBus error "
                                 << ec;
                messages::internalError(asyncResp->res);
                return;
            }

            const uint32_t* value = std::get_if<uint32_t>(&deratingFactor);
            if (value == nullptr)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            nlohmann::json& tempArray =
                asyncResp->res.jsonValue["EfficiencyRatings"];
            tempArray.push_back({});
            nlohmann::json& propertyData = tempArray.back();

            propertyData["EfficiencyPercent"] = *value;
            },
            connection, psAttributesPath, "org.freedesktop.DBus.Properties",
            "Get", "xyz.openbmc_project.Control.PowerSupplyAttributes",
            "DeratingFactor");
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project", 0,
        std::array<const char*, 1>{
            "xyz.openbmc_project.Control.PowerSupplyAttributes"});
}

inline void
    getPowerSupplies(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& chassisID)
{
    BMCWEB_LOG_DEBUG << "Get power supply list associated to chassis = "
                     << chassisID;

    asyncResp->res.jsonValue = {
        {"@odata.type", "#PowerSupplyCollection.PowerSupplyCollection"},
        {"@odata.id",
         "/redfish/v1/Chassis/" + chassisID + "/PowerSubsystem/PowerSupplies"},
        {"Name", "Power Supply Collection"},
        {"Description",
         "The collection of PowerSupply resource instances " + chassisID}};

    crow::connections::systemBus->async_method_call(
        [asyncResp, chassisID](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                subtree) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "D-Bus response error on GetSubTree " << ec;
            messages::internalError(asyncResp->res);
            return;
        }

        nlohmann::json& powerSupplyList = asyncResp->res.jsonValue["Members"];
        powerSupplyList = nlohmann::json::array();

        std::string powerSuppliesPath = "/redfish/v1/Chassis/" + chassisID +
                                        "/PowerSubsystem/PowerSupplies/";
        for (const auto& object : subtree)
        {
            // The association of this PowerSupply is used to determine
            // whether it belongs to this ChassisID
            crow::connections::systemBus->async_method_call(
                [asyncResp, chassisID, &powerSupplyList, powerSuppliesPath,
                 object](
                    const boost::system::error_code ec,
                    const std::variant<std::vector<std::string>>& endpoints) {
                if (ec)
                {
                    if (ec.value() == EBADR)
                    {
                        // This PowerSupply have no chassis association.
                        return;
                    }

                    BMCWEB_LOG_ERROR << "DBUS response error";
                    messages::internalError(asyncResp->res);
                    return;
                }

                const std::vector<std::string>* powerSupplyChassis =
                    std::get_if<std::vector<std::string>>(&(endpoints));

                if (powerSupplyChassis != nullptr)
                {
                    if ((*powerSupplyChassis).size() != 1)
                    {
                        BMCWEB_LOG_ERROR << "PowerSupply association error! ";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    std::vector<std::string> chassisPath = *powerSupplyChassis;
                    sdbusplus::message::object_path path(chassisPath[0]);
                    std::string chassisName = path.filename();
                    if (chassisName != chassisID)
                    {
                        // The PowerSupply does't belong to the
                        // chassisID
                        return;
                    }

                    sdbusplus::message::object_path pathPS(object.first);
                    const std::string objectPowerSupplyID = pathPS.filename();

                    powerSupplyList.push_back(
                        {{"@odata.id",
                          powerSuppliesPath + objectPowerSupplyID}});
                    asyncResp->res.jsonValue["Members@odata.count"] =
                        powerSupplyList.size();
                }
                },
                "xyz.openbmc_project.ObjectMapper", object.first + "/chassis",
                "org.freedesktop.DBus.Properties", "Get",
                "xyz.openbmc_project.Association", "endpoints");
        }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", int32_t(0),
        std::array<const char*, 1>{
            "xyz.openbmc_project.Inventory.Item.PowerSupply"});
}

template <typename Callback>
inline void
    getValidPowerSupplyID(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& chassisID,
                          const std::string& powerSupplyID, Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "getValidPowerSupplyID enter";

    auto respHandler =
        [callback{std::forward<Callback>(callback)}, asyncResp, chassisID,
         powerSupplyID](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                subtree) {
        BMCWEB_LOG_DEBUG << "getValidPowerSupplyID respHandler enter";

        if (ec)
        {
            BMCWEB_LOG_ERROR << "getValidPowerSupplyID respHandler DBUS error: "
                             << ec;
            messages::internalError(asyncResp->res);
            return;
        }

        // Set the default value to resourceNotFound, and if we confirm that
        // powerSupplyID is correct, the error response will be cleared.
        messages::resourceNotFound(asyncResp->res, "PowerSupply",
                                   powerSupplyID);

        for (const auto& object : subtree)
        {
            // The association of this PowerSupply is used to determine
            // whether it belongs to this ChassisID
            crow::connections::systemBus->async_method_call(
                [callback{std::move(callback)}, asyncResp, chassisID,
                 powerSupplyID, object](
                    const boost::system::error_code ec,
                    const std::variant<std::vector<std::string>>& endpoints) {
                if (ec)
                {
                    if (ec.value() == EBADR)
                    {
                        // This PowerSupply have no chassis association.
                        return;
                    }

                    BMCWEB_LOG_ERROR << "DBUS response error";
                    messages::internalError(asyncResp->res);
                    return;
                }

                const std::vector<std::string>* powerSupplyChassis =
                    std::get_if<std::vector<std::string>>(&(endpoints));

                if (powerSupplyChassis != nullptr)
                {
                    if ((*powerSupplyChassis).size() != 1)
                    {
                        BMCWEB_LOG_ERROR << "PowerSupply association error! ";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    std::vector<std::string> chassisPath = *powerSupplyChassis;
                    sdbusplus::message::object_path path(chassisPath[0]);
                    std::string chassisName = path.filename();
                    if (chassisName != chassisID)
                    {
                        // The PowerSupply does't belong to the
                        // chassisID
                        return;
                    }

                    sdbusplus::message::object_path pathPS(object.first);
                    const std::string powerSupplyName = pathPS.filename();
                    if (powerSupplyName.empty())
                    {
                        BMCWEB_LOG_ERROR << "Failed to find powerSupplyName in "
                                         << object.first;
                        return;
                    }

                    std::string validPowerSupplyPath;

                    if (powerSupplyName == powerSupplyID)
                    {
                        // Clear resourceNotFound response
                        asyncResp->res.clear();

                        if (object.second.size() != 1)
                        {
                            BMCWEB_LOG_ERROR << "Error getting PowerSupply "
                                                "D-Bus object!";
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        const std::string& path = object.first;
                        const std::string& connectionName =
                            object.second[0].first;

                        callback(path, connectionName);
                    }
                }
                },
                "xyz.openbmc_project.ObjectMapper", object.first + "/chassis",
                "org.freedesktop.DBus.Properties", "Get",
                "xyz.openbmc_project.Association", "endpoints");
        }
    };

    // Get the PowerSupply Collection
    crow::connections::systemBus->async_method_call(
        respHandler, "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0,
        std::array<const char*, 1>{
            "xyz.openbmc_project.Inventory.Item.PowerSupply"});
    BMCWEB_LOG_DEBUG << "getValidPowerSupplyID exit";
}

inline void requestRoutesPowerSupplyCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/PowerSubsystem/PowerSupplies/")
        .privileges(redfish::privileges::getPowerSupplyCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisID) {
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

            getPowerSupplies(asyncResp, chassisID);
        };
        redfish::chassis_utils::getValidChassisID(asyncResp, chassisID,
                                                  std::move(getChassisID));
        });
}

inline void requestRoutesPowerSupply(App& app)
{
    BMCWEB_ROUTE(
        app, "/redfish/v1/Chassis/<str>/PowerSubsystem/PowerSupplies/<str>/")
        .privileges(redfish::privileges::getPowerSupply)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisID, const std::string& powerSupplyID) {
        auto getChassisID =
            [asyncResp, chassisID,
             powerSupplyID](const std::optional<std::string>& validChassisID) {
            if (!validChassisID)
            {
                BMCWEB_LOG_ERROR << "Not a valid chassis ID:" << chassisID;
                messages::resourceNotFound(asyncResp->res, "Chassis",
                                           chassisID);
                return;
            }

            // Get the PowerSupply information using the path and
            // service obtained by getValidPowerSupplyID function
            auto getPowerSupplyID =
                [asyncResp, chassisID,
                 powerSupplyID](const std::string& validPowerSupplyPath,
                                const std::string& validPowerSupplyService) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#PowerSupply.v1_0_0.PowerSupply";
                asyncResp->res.jsonValue["Name"] = powerSupplyID;
                asyncResp->res.jsonValue["Id"] = powerSupplyID;
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Chassis/" + chassisID +
                    "/PowerSubsystem/PowerSupplies/" + powerSupplyID;

                getPowerSupplyAsset(asyncResp, validPowerSupplyService,
                                    validPowerSupplyPath);

                getPowerSupplyLocation(asyncResp, validPowerSupplyService,
                                       validPowerSupplyPath);

                getPowerSupplyState(asyncResp, validPowerSupplyService,
                                    validPowerSupplyPath);

                getPowerSupplyHealth(asyncResp, validPowerSupplyService,
                                     validPowerSupplyPath);

                getPowerSupplyFirmwareVersion(
                    asyncResp, validPowerSupplyService, validPowerSupplyPath);

                getEfficiencyRatings(asyncResp);
            };
            // Get the correct Path and Service that match the input
            // parameters
            getValidPowerSupplyID(asyncResp, chassisID, powerSupplyID,
                                  std::move(getPowerSupplyID));
        };
        redfish::chassis_utils::getValidChassisID(asyncResp, chassisID,
                                                  std::move(getChassisID));
        });
}

} // namespace redfish
