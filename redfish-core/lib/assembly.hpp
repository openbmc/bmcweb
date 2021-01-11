#pragma once

//#include <boost/container/flat_map.hpp>
//#include <boost/format.hpp>
#include <node.hpp>
//#include <utils/collection.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

inline void getAssemblyData(std::shared_ptr<AsyncResp> aResp,
                            const std::string& assemblyID)
{
    BMCWEB_LOG_DEBUG << "Get available system processor resources.";
    if (1) // just for test)
    {
        messages::internalError(aResp->res);
        return;
    }
    std::cout << assemblyID << std::endl;
}

class Assembly : public Node
{
  public:
    /*
     * Default Constructor
     */
    Assembly(App& app) :
        Node(app, "/redfish/v1/Systems/system/Assembly/<str>/", std::string())
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
    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        // Check if there is required param, truly entering this shall be
        // impossible
        if (params.size() != 1)
        {
            messages::internalError(res);

            res.end();
            return;
        }
        const std::string& assemblyId = params[0];
        res.jsonValue["@odata.type"] = "#Assembly.v1_0_0.Assembly";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/Assmebly/" + assemblyId;

        auto asyncResp = std::make_shared<AsyncResp>(res);

        std::cout << assemblyId << std::endl;
        getAssemblyData(asyncResp, assemblyId);
    }
};

} // namespace redfish