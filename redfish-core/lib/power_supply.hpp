#pragma once

#include <node.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

inline void getPowerSupply(const std::shared_ptr<AsyncResp>& asyncResp,
                           const std::string& chassisID,
                           const std::string& powerSupplyID)
{
    BMCWEB_LOG_DEBUG
        << "Get properties for getPowerSupply associated to chassis = "
        << chassisID;

    asyncResp->res.jsonValue["@odata.type"] = "#PowerSupply.v1_0_0.PowerSupply";
    asyncResp->res.jsonValue["Name"] = "Power Supply " + powerSupplyID;
    asyncResp->res.jsonValue["Id"] = powerSupplyID;
    asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Chassis/" + chassisID +
                                            "/PowerSubsystem/PowerSupplies/" +
                                            powerSupplyID;
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

        getPowerSupply(asyncResp, chassisID, powerSupplyID);
    }
};

} // namespace redfish
