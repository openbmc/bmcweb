#pragma once

#include "node.hpp"

namespace redfish
{

class VirtualMedia : public Node
{
  public:
    VirtualMedia(CrowApp& app) : Node(app, "/redfish/v1/VirtualMedia/")
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
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        res.jsonValue["@odata.type"] = "#VirtualMedia.v1_1_1.VirtualMedia";
        res.jsonValue["@odata.id"] = "/redfish/v1/VirtualMedia";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#VirtualMedia.VirtualMedia";
        res.jsonValue["Id"] = "VirtualMedia";
        res.jsonValue["Description"] = "Virtual Media Service";
        res.jsonValue["Name"] = "Virtual Media";
        res.end();
    }
};

} // namespace redfish
