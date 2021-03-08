#pragma once

#include <node.hpp>
#include <utils/chassis_utils.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{
class ThermalSubsystem : public Node
{
  public:
    ThermalSubsystem(App& app) :
        Node(app, "/redfish/v1/Chassis/<str>/ThermalSubsystem/", std::string())
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

        auto getChassisPath =
            [asyncResp,
             chassisId](const std::optional<std::string>& chassisPath) {
                if (!chassisPath)
                {
                    BMCWEB_LOG_ERROR << "Not a valid chassis ID" << chassisId;
                    messages::resourceNotFound(asyncResp->res, "Chassis",
                                               chassisId);
                    return;
                }
                asyncResp->res.jsonValue = {
                    {"@odata.type",
                     "#ThermalSubsystem.v1_0_0.ThermalSubsystem"},
                    {"Name", "Thermal Subsystem for Chassis"},
                    {"Id", chassisId}};
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Chassis/" + chassisId + "/ThermalSubsystem";

                asyncResp->res.jsonValue["ThermalMetrics"] = {
                    {"@odata.id", "/redfish/v1/Chassis/" + chassisId +
                                      "/ThermalSubsystem/ThermalMetrics"}};

                asyncResp->res.jsonValue["Status"] = {{"State", "Enabled"},
                                                      {"Health", "OK"}};
            };
        redfish::chassis_utils::getValidChassisID(asyncResp, chassisId,
                                                  std::move(getChassisPath));
    }
};
} // namespace redfish