#pragma once

#include <boost/algorithm/string/split.hpp>
#include <node.hpp>
#include <utils/chassis_utils.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

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
                 "/redfish/v1/Chassis/" + chassisId + "/ThermalSubsystem/Fans"},
                {"Name", "Fan Collection"},
                {"Description",
                 "The collection of Fan resource instances " + chassisId}};
            crow::connections::systemBus->async_method_call(
                [asyncResp, chassisId](
                    const boost::system::error_code ec,
                    const std::vector<
                        std::pair<std::string,
                                  std::vector<std::pair<
                                      std::string, std::vector<std::string>>>>>&
                        subtree) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG << "DBUS response error";
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    for (const auto& realobject : subtree)
                    {
                        sdbusplus::message::object_path path(realobject.first);
                        const std::string& realLeaf = path.filename();
                        if (realLeaf.empty())
                        {
                            continue;
                        }
                        const std::string& realpath = realobject.first;
                        const std::string& connectionName =
                            realobject.second[0].first;

                        crow::connections::systemBus->async_method_call(
                            [asyncResp, chassisId, realpath, connectionName](
                                const boost::system::error_code ec,
                                const std::vector<std::tuple<
                                    std::string, std::string, std::string>>&
                                    values) {
                                if (ec)
                                {
                                    BMCWEB_LOG_DEBUG << "DBUS response error";
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                nlohmann::json& fanList =
                                    asyncResp->res.jsonValue["Members"];
                                fanList = nlohmann::json::array();

                                for (const auto& value : values)
                                {
                                    const auto& [str1, str2, str3] = value;
                                    const std::string& objPath = str3;
                                    std::vector<std::string> split;
                                    // Reserve space for
                                    // /xyz/openbmc_project/sensors/<name>/<subname>
                                    split.reserve(6);
                                    boost::algorithm::split(
                                        split, objPath, boost::is_any_of("/"));
                                    if (split.size() < 6)
                                    {
                                        BMCWEB_LOG_ERROR << "Got path that "
                                                            "isn't long enough "
                                                         << objPath;
                                        continue;
                                    }
                                    // These indexes aren't intuitive, as
                                    // boost::split puts an empty string at the
                                    // beginning
                                    const std::string& fanType = split[4];
                                    const std::string& fanName = split[5];
                                    std::string leaf;

                                    if (fanType == "fan" ||
                                        fanType == "fan_tach" ||
                                        fanType == "fan_pwm")
                                    {
                                        leaf = fanName;
                                        std::string newPath =
                                            "/redfish/v1/Chassis/" + chassisId +
                                            "/ThermalSubsystem/Fans/" + leaf;
                                        fanList.push_back(
                                            {{"@odata.id",
                                              std::move(newPath)}});
                                    }
                                    else
                                    {
                                        BMCWEB_LOG_ERROR << "This is not fan "
                                                         << fanType;
                                        continue;
                                    }
                                    asyncResp->res
                                        .jsonValue["Members@odata.count"] =
                                        fanList.size();
                                }
                            },
                            connectionName, realpath,
                            "org.freedesktop.DBus.Properties", "Get",
                            "xyz.openbmc_project.Association.Definitions",
                            "Associations");
                    }
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                "/xyz/openbmc_project/inventory", int32_t(0),
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

        auto getChassisPath =
            [asyncResp, chassisId,
             fanId](const std::optional<std::string>& chassisPath) {
                if (!chassisPath)
                {
                    BMCWEB_LOG_ERROR << "Not a valid chassis ID" << chassisId;
                    messages::resourceNotFound(asyncResp->res, "Chassis",
                                               chassisId);
                    return;
                }

                asyncResp->res.jsonValue["@odata.type"] = "#Fan.v1_0_0.Fan";
                asyncResp->res.jsonValue["Name"] = fanId;
                asyncResp->res.jsonValue["Id"] = fanId;
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Chassis/" + chassisId +
                    "ThermalSubsystem/Fans/" + fanId;
            };
        redfish::chassis_utils::getValidChassisID(asyncResp, chassisId,
                                                  std::move(getChassisPath));
    }
};

} // namespace redfish