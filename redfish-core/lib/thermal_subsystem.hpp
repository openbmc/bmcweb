#pragma once

#include <include/async_resp.hpp>
#include <node.hpp>
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
        std::shared_ptr<bmcweb::AsyncResp> asyncResp =
            std::make_shared<bmcweb::AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string& chassisId = params[0];
        asyncResp->res.jsonValue = {
            {"@odata.type", "#ThermalSubsystem.v1_0_0.ThermalSubsystem"},
            {"Name", "Thermal Subsystem for Chassis"},
            {"Id", chassisId}};
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Chassis/" + chassisId + "/ThermalSubsystem";

        // TODO : Add new fan schema
        // asyncResp->res.jsonValue["Fans"] = {
        //     {"@odata.id",
        //      "/redfish/v1/Chassis/" + chassisId + "/ThermalSubsystem/Fans"}};
        // TODO : Add Thermal Metrics schema
        // asyncResp->res.jsonValue["ThermalMetrics"] = {
        //     {"@odata.id", "/redfish/v1/Chassis/" + chassisId +
        //                       "/ThermalSubsystem/ThermalMetrics"}};
        asyncResp->res.jsonValue["Status"] = {{"State", "Enabled"},
                                              {"Health", "OK"}};
    }
};
} // namespace redfish