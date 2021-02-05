#pragma once

#include <node.hpp>
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
            asyncResp->res.jsonValue["Status"]["State"] =
                *value ? "Absent" : "Enabled";
        },
        connectionName, path, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Inventory.Item", "Present");
}

inline void getPowerSupplyHealth(const std::shared_ptr<AsyncResp>& asyncResp,
                                 const std::string& connectionName,
                                 const std::string& path)
{
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
            asyncResp->res.jsonValue["Status"]["Health"] =
                *value ? "OK" : "Critical";
        },
        connectionName, path, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.State.Decorator.OperationalStatus", "Functional");
}

inline void getPowerSupplyInfo(const std::shared_ptr<AsyncResp>& asyncResp,
                               const std::string& chassisID,
                               const std::string& powerSupplyID)
{
    BMCWEB_LOG_DEBUG
        << "Get properties for getPowerSupply associated to chassis = "
        << chassisID << powerSupplyID;

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
            if (subtree.size() == 0)
            {
                BMCWEB_LOG_DEBUG << "Can't find PowerSupply D-Bus object!";
                messages::resourceNotFound(asyncResp->res, "PowerSupply",
                                           powerSupplyID);
                return;
            }

            for (const auto& object : subtree)
            {
                auto iter = object.first.rfind("/");
                if ((iter != std::string::npos) && (iter < object.first.size()))
                {
                    if (object.first.substr(iter + 1) == powerSupplyID)
                    {
                        if (object.second.size() != 1)
                        {
                            BMCWEB_LOG_DEBUG
                                << "Error getting PowerSupply D-Bus object!";
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        const std::string& path = object.first;
                        const std::string& connectionName =
                            object.second[0].first;

                        if (connectionName.empty())
                        {
                            BMCWEB_LOG_DEBUG
                                << "Error getting PowerSupply service name!";
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        // Get power supply asset
                        getPowerSupplyAsset(asyncResp, connectionName, path);

                        // Get power supply state
                        getPowerSupplyState(asyncResp, connectionName, path);

                        // Get power supply health
                        getPowerSupplyHealth(asyncResp, connectionName, path);
                    }
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
                                 const std::string& chassisID,
                                 const std::string& powerSupplyID)
{
    BMCWEB_LOG_DEBUG
        << "Get properties for getPowerSupply associated to chassis = "
        << chassisID << powerSupplyID;
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
                messages::internalError(asyncResp->res);
                BMCWEB_LOG_ERROR
                    << "Get PowerSupply attributes respHandler DBus error "
                    << ec;
                return;
            }
            if (subtree.size() == 0)
            {
                BMCWEB_LOG_DEBUG << "Can't find Power Supply Attributes!";
                messages::resourceNotFound(asyncResp->res, "PowerSupply",
                                           powerSupplyID);
                return;
            }

            // Currently we only support 1 power supply attribute, use this for
            // all the power supplies.
            if (subtree[0].first.empty() || subtree[0].second.empty())
            {
                BMCWEB_LOG_DEBUG << "Get Power Supply Attributes error!";
                return;
            }

            const std::string& psAttributesPath = subtree[0].first;
            const std::string& connection = subtree[0].second[0].first;

            if (connection.empty())
            {
                BMCWEB_LOG_DEBUG << "Get Power Supply Attributes error error!";
                return;
            }

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
                    if (value != nullptr)
                    {
                        BMCWEB_LOG_DEBUG << "PS EfficiencyPercent value: "
                                         << *value;
                        propertyData["EfficiencyPercent"] = *value;
                    }
                    else
                    {
                        BMCWEB_LOG_DEBUG << "Failed to find EfficiencyPercent "
                                            "value for PowerSupplies";
                        messages::internalError(asyncResp->res);
                        return;
                    }
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

        asyncResp->res.jsonValue = {
            {"@odata.type", "#PowerSupplyCollection.PowerSupplyCollection"},
            {"@odata.id", "/redfish/v1/Chassis/" + chassisID +
                              "/PowerSubsystem/PowerSupplies"},
            {"Name", "Power Supply Collection"},
            {"Description",
             "The collection of PowerSupply resource instances " + chassisID}};

        crow::connections::systemBus->async_method_call(
            [asyncResp, chassisID](
                const boost::system::error_code ec,
                const std::vector<std::pair<
                    std::string, std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>>&
                    subtree) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "D-Bus response error on GetSubTree "
                                     << ec;
                    return;
                }
                if (subtree.size() == 0)
                {
                    BMCWEB_LOG_DEBUG << "Can't find PowerSupply D-Bus object!";
                    messages::resourceNotFound(asyncResp->res, "PowerSupply",
                                               chassisID);
                    return;
                }

                nlohmann::json& powerSupplyList =
                    asyncResp->res.jsonValue["Members"];
                powerSupplyList = nlohmann::json::array();

                for (const auto& object : subtree)
                {
                    auto iter = object.first.rfind("/");
                    if ((iter != std::string::npos) &&
                        (iter < object.first.size()))
                    {
                        powerSupplyList.push_back(
                            {{"@odata.id",
                              "/redfish/v1/Chassis/" + chassisID +
                                  "/PowerSubsystem/PowerSupplies/" +
                                  object.first.substr(iter + 1)}});
                    }
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

        asyncResp->res.jsonValue["@odata.type"] =
            "#PowerSupply.v1_0_0.PowerSupply";
        asyncResp->res.jsonValue["Name"] = powerSupplyID;
        asyncResp->res.jsonValue["Id"] = powerSupplyID;
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Chassis/" + chassisID +
            "/PowerSubsystem/PowerSupplies/" + powerSupplyID;

        // Get power supply asset and status
        getPowerSupplyInfo(asyncResp, chassisID, powerSupplyID);

        // Get power supply efficiency ratings
        getEfficiencyRatings(asyncResp, chassisID, powerSupplyID);
    }
};

} // namespace redfish
