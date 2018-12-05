#pragma once

#include "node.hpp"

namespace redfish
{

class VirtualMedia : public Node
{
  public:
    VirtualMedia(CrowApp &app) :
        Node(app, "/redfish/v1/Managers/bmc/VirtualMedia/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/bmc/VirtualMedia/";
        asyncResp->res.jsonValue["@odata.type"] =
            "#VirtualMediaCollection.VirtualMediaCollection";
        asyncResp->res.jsonValue["@odata.context"] =
            "/redfish/v1/"
            "$metadata#VirtualMediaCollection.VirtualMediaCollection";
        asyncResp->res.jsonValue["Name"] = "Virtual Media Collection";
        asyncResp->res.jsonValue["Description"] =
            "Collection of VirtualMedia instances for this Manager";
        nlohmann::json &members = asyncResp->res.jsonValue["Members"];
        members = nlohmann::json::array();
        asyncResp->res.jsonValue["Members@odata.count"] = members.size();
    }
};

} // namespace redfish
