#pragma once

#include <node.hpp>
#include <utils/chassis_utils.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

inline void getPowerSupplyAsset(const std::shared_ptr<AsyncResp>& asyncResp,
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
                BMCWEB_LOG_DEBUG << "Can't get PowerSupply asset!";
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
                        // illegal property
                        messages::internalError(asyncResp->res);
                        continue;
                    }
                    asyncResp->res.jsonValue[propertyName] = *value;
                }
            }
        },
        connectionName, path, "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Inventory.Decorator.Asset");
}

inline void getPowerSupplyState(const std::shared_ptr<AsyncResp>& asyncResp,
                                const std::string& connectionName,
                                const std::string& path)
{

    asyncResp->res.jsonValue["Status"]["State"] = "Absent";

    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const std::variant<bool> state) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "Can't get PowerSupply state!";
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

inline void getPowerSupplyHealth(const std::shared_ptr<AsyncResp>& asyncResp,
                                 const std::string& connectionName,
                                 const std::string& path)
{

    asyncResp->res.jsonValue["Status"]["Health"] = "Critical";

    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const std::variant<bool> health) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "Can't get PowerSupply health!";
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

inline void getPowerSupplyInfo(const std::shared_ptr<AsyncResp>& asyncResp,
                               const std::string& powerSupplyID)
{
    BMCWEB_LOG_DEBUG << "Get information for PowerSupply = " << powerSupplyID;

    crow::connections::systemBus->async_method_call(
        [asyncResp, powerSupplyID](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "D-Bus response error on GetSubTree " << ec;
                return;
            }

            std::string objectPowerSupplyID;
            for (const auto& object : subtree)
            {
                sdbusplus::message::object_path path(object.first);
                objectPowerSupplyID = path.filename();

                if (objectPowerSupplyID == powerSupplyID)
                {
                    if (object.second.size() != 1)
                    {
                        BMCWEB_LOG_DEBUG
                            << "Error getting PowerSupply D-Bus object!";
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    const std::string& path = object.first;
                    const std::string& connectionName = object.second[0].first;

                    // Get power supply asset
                    getPowerSupplyAsset(asyncResp, connectionName, path);

                    // Get power supply state
                    getPowerSupplyState(asyncResp, connectionName, path);

                    // Get power supply health
                    getPowerSupplyHealth(asyncResp, connectionName, path);
                }
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", int32_t(0),
        std::array<const char*, 1>{
            "xyz.openbmc_project.Inventory.Item.PowerSupply"});
}

inline void getEfficiencyRatings(const std::shared_ptr<AsyncResp>& asyncResp,
                                 const std::string& powerSupplyID)
{
    BMCWEB_LOG_DEBUG << "Get properties for PowerSupply= " << powerSupplyID;
    // Gets the Power Supply Attributes such as EfficiencyPercent
    crow::connections::systemBus->async_method_call(
        [asyncResp, powerSupplyID](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                subtree) {
            if (ec)
            {
                BMCWEB_LOG_ERROR
                    << "Get PowerSupply attributes respHandler DBus error "
                    << ec;
                return;
            }

            // Currently we only support 1 power supply attribute, use this for
            // all the power supplies.
            if (subtree[0].second.empty())
            {
                BMCWEB_LOG_DEBUG << "Get Power Supply Attributes error!";
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

                    nlohmann::json& tempArray =
                        asyncResp->res.jsonValue["EfficiencyRatings"];
                    tempArray.push_back({});
                    nlohmann::json& propertyData = tempArray.back();

                    const uint32_t* value =
                        std::get_if<uint32_t>(&deratingFactor);
                    if (value == nullptr)
                    {
                        BMCWEB_LOG_DEBUG << "Failed to find EfficiencyPercent "
                                            "value for PowerSupplies";
                        messages::internalError(asyncResp->res);
                        return;
                    }

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

inline void getPowerSupplies(const std::shared_ptr<AsyncResp>& asyncResp,
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
                BMCWEB_LOG_DEBUG << "D-Bus response error on GetSubTree " << ec;
                return;
            }

            nlohmann::json& powerSupplyList =
                asyncResp->res.jsonValue["Members"];
            powerSupplyList = nlohmann::json::array();

            std::string objectPowerSupplyID;
            for (const auto& object : subtree)
            {
                sdbusplus::message::object_path path(object.first);
                objectPowerSupplyID = path.filename();

                powerSupplyList.push_back(
                    {{"@odata.id", "/redfish/v1/Chassis/" + chassisID +
                                       "/PowerSubsystem/PowerSupplies/" +
                                       objectPowerSupplyID}});
            }
            asyncResp->res.jsonValue["Members@odata.count"] =
                powerSupplyList.size();
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", int32_t(0),
        std::array<const char*, 1>{
            "xyz.openbmc_project.Inventory.Item.PowerSupply"});
}

class PowerSupplyCollection : public Node
{
  public:
    /*
     * Default Constructor
     */
    PowerSupplyCollection(App& app) :
        Node(app, "/redfish/v1/Chassis/<str>/PowerSubsystem/PowerSupplies/",
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

                getPowerSupplies(asyncResp, chassisID);
            };
        redfish::chassis_utils::getValidChassisID(asyncResp, chassisID,
                                                  std::move(getChassisID));
    }
};

class PowerSupply : public Node
{
  public:
    /*
     * Default Constructor
     */
    PowerSupply(App& app) :
        Node(app,
             "/redfish/v1/Chassis/<str>/PowerSubsystem/PowerSupplies/<str>/",
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

        const std::string& chassisID = params[0];
        const std::string& powerSupplyID = params[1];

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

                asyncResp->res.jsonValue["@odata.type"] =
                    "#PowerSupply.v1_0_0.PowerSupply";
                asyncResp->res.jsonValue["Name"] = powerSupplyID;
                asyncResp->res.jsonValue["Id"] = powerSupplyID;
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Chassis/" + chassisID +
                    "/PowerSubsystem/PowerSupplies/" + powerSupplyID;

                // Get power supply asset and status
                getPowerSupplyInfo(asyncResp, powerSupplyID);

                // Get power supply efficiency ratings
                getEfficiencyRatings(asyncResp, powerSupplyID);
            };
        redfish::chassis_utils::getValidChassisID(asyncResp, chassisID,
                                                  std::move(getChassisID));
    }
};

} // namespace redfish
