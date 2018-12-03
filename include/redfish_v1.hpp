#pragma once

#include <boost/algorithm/string.hpp>
#include <dbus_singleton.hpp>
#include <fstream>
#include <streambuf>
#include <string>
#include <webserver_common.hpp>

namespace crow
{
namespace redfish
{
void requestRoutes(CrowApp& app)
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
