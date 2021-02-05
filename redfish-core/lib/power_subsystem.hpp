#pragma once

#include <node.hpp>
#include <utils/chassis_utils.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

inline void getPowerSubsystem(const std::shared_ptr<AsyncResp>& asyncResp,
                              const std::string& chassisID)
{
    BMCWEB_LOG_DEBUG
        << "Get properties for PowerSubsystem associated to chassis = "
        << chassisID;

    asyncResp->res.jsonValue = {
        {"@odata.type", "#PowerSubsystem.v1_0_0.PowerSubsystem"},
        {"Name", "Power Subsystem for Chassis"}};
    asyncResp->res.jsonValue["Id"] = "1";
    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Chassis/" + chassisID + "/PowerSubsystem";
    asyncResp->res.jsonValue["PowerSupplies"]["@odata.id"] =
        "/redfish/v1/Chassis/" + chassisID + "/PowerSubsystem/PowerSupplies";
}

class PowerSubsystem : public Node
{
  public:
    /*
     * Default Constructor
     */
    PowerSubsystem(App& app) :
        Node(app, "/redfish/v1/Chassis/<str>/PowerSubsystem/", std::string())
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

                getPowerSubsystem(asyncResp, chassisID);
            };
        redfish::chassis_utils::getValidChassisID(asyncResp, chassisID,
                                                  std::move(getChassisID));
    }
};

} // namespace redfish
