#pragma once

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
                          const std::string& chassisId,
                          const std::string& fanId, Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "checkFanId enter";
    const std::array<const char*, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Fan"};

    auto respHandler = [callback{std::move(callback)}, asyncResp, chassisId,
                        fanId](const boost::system::error_code ec,
                               const std::vector<std::string>& fanPaths) {
        BMCWEB_LOG_DEBUG << "getValidFanID respHandler enter";
        if (ec)
        {
            BMCWEB_LOG_ERROR << "getValidFanID respHandler DBUS error: " << ec;
            messages::internalError(asyncResp->res);
            return;
        }

        std::optional<std::string> validFanId;
        std::string fanName;
        for (const std::string& fan : fanPaths)
        {
            sdbusplus::message::object_path path(fan);
            fanName = path.filename();
            if (fanName.empty())
            {
                BMCWEB_LOG_ERROR << "Failed to find '/' in " << fan;
                continue;
            }
            if (fanName == fanId)
            {
                validFanId = fanId;
                break;
            }
        }
        callback(validFanId);
    };

    // Get the fan Collection
    crow::connections::systemBus->async_method_call(
        respHandler, "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/inventory", 0, interfaces);
    BMCWEB_LOG_DEBUG << "checkFanId exit";
}

inline void getFanState(const std::shared_ptr<AsyncResp>& asyncResp,
                        const std::string& connectionName,
                        const std::string& path)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const std::variant<bool> state) {
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
            asyncResp->res.jsonValue["Status"]["State"] =
                *value ? "Absent" : "Enabled";
        },
        connectionName, path, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Inventory.Item", "Present");
}

inline void getFanHealth(const std::shared_ptr<AsyncResp>& asyncResp,
                         const std::string& connectionName,
                         const std::string& path)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const std::variant<bool> health) {
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
            asyncResp->res.jsonValue["Status"]["Health"] =
                *value ? "OK" : "Critical";
        },
        connectionName, path, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.State.Decorator.OperationalStatus", "Functional");
}

inline void getFanInfo(const std::shared_ptr<AsyncResp>& asyncResp,
                       const std::string& chassisId, const std::string& fanId)
{
    BMCWEB_LOG_DEBUG << "Get properties for getFan associated to chassis = "
                     << chassisId << fanId;
    crow::connections::systemBus->async_method_call(
        [asyncResp, fanId](
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
            for (const auto& object : subtree)
            {
                auto iter = object.first.rfind("/");
                if ((iter != std::string::npos) && (iter < object.first.size()))
                {
                    if (object.first.substr(iter + 1) == fanId)
                    {
                        if (object.second.size() != 1)
                        {
                            BMCWEB_LOG_DEBUG
                                << "Error getting Fan D-Bus object!";
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        const std::string& path = object.first;
                        const std::string& connectionName =
                            object.second[0].first;
                        if (connectionName.empty())
                        {
                            BMCWEB_LOG_DEBUG
                                << "Error getting Fan service name!";
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        // Get fan state
                        getFanState(asyncResp, connectionName, path);
                        // Get fan health
                        getFanHealth(asyncResp, connectionName, path);
                    }
                }
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", int32_t(0),
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
                {"@odata.id",
                 "/redfish/v1/Chassis/" + chassisId + "ThermalSubsystem/Fans"},
                {"Name", "Fan Collection"},
                {"Description",
                 "The collection of Fan resource instances " + chassisId}};
            crow::connections::systemBus->async_method_call(
                [asyncResp,
                 chassisId](const boost::system::error_code ec,
                            const std::vector<std::string>& objects) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG << "DBUS response error";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    nlohmann::json& fanList =
                        asyncResp->res.jsonValue["Members"];
                    fanList = nlohmann::json::array();

                    for (const auto& object : objects)
                    {
                        sdbusplus::message::object_path path(object);
                        std::string leaf = path.filename();
                        if (leaf.empty())
                        {
                            continue;
                        }
                        std::string newPath = "/redfish/v1/Chassis/" +
                                              chassisId +
                                              "/ThermalSubsystem/Fans/";
                        newPath += leaf;
                        fanList.push_back({{"@odata.id", std::move(newPath)}});
                    }
                    asyncResp->res.jsonValue["Members@odata.count"] =
                        fanList.size();
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
                "/xyz/openbmc_project/inventory", int32_t(0),
                std::array<const char*, 1>{
                    "xyz.openbmc_project.Inventory.Item.Fan"});
        };
        redfish::chassis_utils::getValidChassisID(asyncResp, chassisId,
                                                  std::move(getChassisPath));
    }
}; // namespace redfish

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
                asyncResp->res.jsonValue["@odata.type"] = "#Fan.v1_0_0.Fan";
                asyncResp->res.jsonValue["Name"] = fanId;
                asyncResp->res.jsonValue["Id"] = fanId;
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Chassis/" + chassisId +
                    "ThermalSubsystem/Fans/" + fanId;
                // Get fan asset and status
                getFanInfo(asyncResp, chassisId, fanId);
            };
            getValidFanID(asyncResp, chassisId, fanId, std::move(getFanPath));
        };
        redfish::chassis_utils::getValidChassisID(asyncResp, chassisId,
                                                  std::move(getChassisPath));
    }
};

} // namespace redfish
