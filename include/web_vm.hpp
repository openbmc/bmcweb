#pragma once
#include <crow/app.h>

#include <boost/process.hpp>

namespace crow
{
namespace vm
{

template <typename... Middlewares> void requestRoutes(Crow<Middlewares...>& app)
{
    BMCWEB_ROUTE(app, "/vmws")
        .methods(
            "GET"_method)([](const crow::Request& req, crow::Response& res) {
            boost::process::ipstream output;
            boost::process::child cmd("/usr/sbin/nbd-proxy", "--metadata",
                                      boost::process::std_out > output);
            std::string line;
            if (output && std::getline(output, line) && !line.empty())
            {
                res.jsonValue = {{"data", std::move(line)},
                                 {"message", "200 OK"},
                                 {"status", "ok"}};
            }
            cmd.wait();
            int rc = cmd.exit_code();
            if (rc)
            {
                BMCWEB_LOG_ERROR << "Failed to get Virtual Media instances, rc="
                                 << rc;
                res.result(boost::beast::http::status::internal_server_error);
            }
            res.end();
        });
}

} // namespace vm
} // namespace crow
