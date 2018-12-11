#pragma once
#include <crow/app.h>

#include <boost/process.hpp>

namespace crow
{
namespace obmc_vm
{

template <typename... Middlewares> void requestRoutes(Crow<Middlewares...>& app)
{
    BMCWEB_ROUTE(app, "/vm/0")
        .methods(
            "GET"_method)([](const crow::Request& req, crow::Response& res) {
            boost::asio::io_service io;
            std::vector<char> buffer(1024);

            boost::process::child cmd(
                "/usr/sbin/nbd-proxy", "--metadata",
                boost::process::std_out > boost::asio::buffer(buffer), io);
            io.run();

            std::string line(buffer.data());
            res.jsonValue = {{"data", std::move(line)},
                             {"message", "200 OK"},
                             {"status", "ok"}};
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

} // namespace obmc_vm
} // namespace crow
