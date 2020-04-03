#pragma once

#include <app.h>

#include <boost/algorithm/string.hpp>
#include <dbus_singleton.hpp>
#include <persistent_data_middleware.hpp>

#include <fstream>
#include <streambuf>
#include <string>
namespace crow
{
namespace redfish
{
template <typename... Middlewares>
void requestRoutes(Crow<Middlewares...>& app)
{
    BMCWEB_ROUTE(app, "/redfish/")
        .methods("GET"_method)(
            [](const crow::Request& req, crow::Response& res) {
                res.jsonValue = {{"v1", "/redfish/v1/"}};
                res.end();
            });
}
} // namespace redfish
} // namespace crow
