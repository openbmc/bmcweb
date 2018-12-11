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
            boost::process::ipstream output;

            boost::process::child cmd("/usr/sbin/nbd-proxy", "--metadata",
                                      boost::process::std_out > output);

            // Wait for an arbitrary 0.5s
            std::error_code ec;
            const auto cmdDone =
                cmd.wait_for(std::chrono::milliseconds(500), ec);
            if (cmdDone)
            {
                int rc = cmd.exit_code();
                if (rc)
                {
                    BMCWEB_LOG_ERROR
                        << "Error getting the Virtual Media instances, rc="
                        << rc;
                    res.result(
                        boost::beast::http::status::internal_server_error);
                }
                else
                {
                    std::string outputStr;
                    std::getline(output, outputStr);
                    auto data =
                        nlohmann::json::parse(outputStr, nullptr, false);
                    if (data.is_discarded())
                    {
                        BMCWEB_LOG_ERROR
                            << "Error parsing metadata data into json file.";
                        res.result(
                            boost::beast::http::status::internal_server_error);
                    }
                    else
                    {
                        res.jsonValue = {{"message", "200 OK"},
                                         {"status", "ok"}};
                        auto& objects = res.jsonValue["data"];
                        for (auto& item : data.items())
                        {
                            objects.push_back({{item.key(), item.value()}});
                        }
                    }
                }
            }
            else
            {
                BMCWEB_LOG_ERROR << "Failed to get Virtual Media instances: "
                                 << ec;
                res.result(boost::beast::http::status::internal_server_error);
            }
            res.end();
        });
}

} // namespace obmc_vm
} // namespace crow
