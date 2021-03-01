#pragma once

#include <node.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

class ThermalSubsystem : public Node
{
  public:
    /*
     * Default Constructor
     */
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
    }
};

} // namespace redfish
