#pragma once

#include <crow/app.h>

#include <filesystem>
#include <nlohmann/json.hpp>

namespace crow
{
namespace obmc_vm
{

template <typename... Middlewares> void requestRoutes(Crow<Middlewares...>& app)
{
    BMCWEB_ROUTE(app, "/vm/0")
        .methods(
            "GET"_method)([](const crow::Request& req, crow::Response& res) {
            static constexpr auto config = "/etc/nbd-proxy/config.json";

            std::ifstream file(config);
            if (!file.good())
            {
                BMCWEB_LOG_ERROR
                    << "Error reading virtual media configuration file: "
                    << config;
                res.result(boost::beast::http::status::internal_server_error);
                res.end();
                return;
            }

            nlohmann::json data = nlohmann::json::parse(file, nullptr, false);
            if (data.is_discarded())
            {
                BMCWEB_LOG_ERROR << "Error parsing the json file for "
                                    "virtual media configurations";
                res.result(boost::beast::http::status::internal_server_error);
                res.end();
                return;
            }
            nlohmann::json::iterator j = data.find("configurations");
            if (j == data.end())
            {
                BMCWEB_LOG_ERROR << "No configurations found";
                res.result(boost::beast::http::status::internal_server_error);
                res.end();
                return;
            }

            nlohmann::json jsonData;
            for (const auto& item : j->items())
            {
                if (!item.value().is_object())
                {
                    BMCWEB_LOG_ERROR << "No configuration data found";
                    res.result(
                        boost::beast::http::status::internal_server_error);
                    res.end();
                    return;
                }

                nlohmann::json::iterator m = item.value().find("metadata");
                if (m == item.value().end())
                {
                    BMCWEB_LOG_ERROR << "No configuration metadata found";
                    res.result(
                        boost::beast::http::status::internal_server_error);
                    res.end();
                    return;
                }

                jsonData[item.key()] = std::move(*m);
            }
            res.jsonValue = {{"data", std::move(jsonData)},
                             {"message", "200 OK"},
                             {"status", "ok"}};
            res.end();
        });
}

} // namespace obmc_vm
} // namespace crow
