#pragma once
#include <app.h>
#include <tinyxml2.h>

#include <async_resp.hpp>

namespace crow
{
namespace ibm_mc
{

template <typename... Middlewares> void requestRoutes(Crow<Middlewares...> &app)
{

    // allowed only for admin
    BMCWEB_ROUTE(app, "/ibm/v1/")
        .requires({"ConfigureComponents", "ConfigureManager"})
        .methods(
            "GET"_method)([](const crow::Request &req, crow::Response &res) {
            res.jsonValue["@odata.type"] = "#ibmServiceRoot.v1_0_0.ibmServiceRoot";
            res.jsonValue["@odata.id"] = "/ibm/v1/";
            res.jsonValue["Id"] = "IBM Rest RootService";
            res.jsonValue["Name"] = "IBM Service Root";
            res.jsonValue["ConfigFiles"] = {
                {"@odata.id", "/ibm/v1/Host/ConfigFiles"}};
            res.jsonValue["LockService"] = {
                {"@odata.id", "/ibm/v1/HMC/LockService"}};
            res.end();
        });
}

} // namespace ibm_mc
} // namespace crow
