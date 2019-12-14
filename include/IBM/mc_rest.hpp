#pragma once
#include <app.h>
#include <tinyxml2.h>

#include <async_resp.hpp>

namespace crow
{
namespace openbmc_ibm_mc
{

template <typename... Middlewares> void requestRoutes(Crow<Middlewares...> &app)
{

    // allowed only for admin
    BMCWEB_ROUTE(app, "/ibm/v1/")
        .requires({"ConfigureComponents", "ConfigureManager"})
        .methods(
            "GET"_method)([](const crow::Request &req, crow::Response &res) {
            res.jsonValue["@odata.type"] = "#ServiceRoot.v1_0_0.ServiceRoot";
            res.jsonValue["@odata.id"] = "/ibm/v1/";
            res.jsonValue["@odata.context"] =
                "/ibm/v1/$metadata#ServiceRoot.ServiceRoot";
            res.jsonValue["Id"] = "IBM Rest RootService";
            res.jsonValue["Name"] = "IBM Rest Root Service";
            res.jsonValue["ConfigFiles"] = {
                {"@odata.id", "/ibm/v1/Host/ConfigFiles"}};
            res.jsonValue["LockService"] = {
                {"@odata.id", "/ibm/v1/HMC/LockService"}};
            res.end();
        });
}

} // namespace openbmc_ibm_mc
} // namespace crow
