#pragma once

#include "filesystem.hpp"

#include <crow/app.h>

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

            if (!(std::filesystem::exists(config)))
            {
                BMCWEB_LOG_ERROR << "Missing virtual media configuration file: "
                                 << config;
                res.result(boost::beast::http::status::internal_server_error);
                res.end();
                return;
            }

            std::ifstream file(config);
            auto data = nlohmann::json::parse(file, nullptr, false);
            if (data.is_discarded())
            {
                BMCWEB_LOG_ERROR << "Error parsing the json file for "
                                    "virtual media configurations";
                res.result(boost::beast::http::status::internal_server_error);
                res.end();
                return;
            }

            if (data.find("configurations") == data.end())
            {
                BMCWEB_LOG_ERROR << "No configurations found";
                res.result(boost::beast::http::status::internal_server_error);
                res.end();
                return;
            }

            nlohmann::json jsonData;
            for (const auto& item : data["configurations"].items())
            {
                if (!(item.value().is_object()))
                {
                    BMCWEB_LOG_ERROR << "No configuration data found";
                    res.result(
                        boost::beast::http::status::internal_server_error);
                    res.end();
                    return;
                }

                if (item.value().find("metadata") == item.value().end())
                {
                    BMCWEB_LOG_ERROR << "No configuration metadata found";
                    res.result(
                        boost::beast::http::status::internal_server_error);
                    res.end();
                    return;
                }

                for (const auto& meta : item.value()["metadata"].items())
                {
                    jsonData[item.key()][meta.key()] = meta.value();
                }
            }
            res.jsonValue = {{"data", std::move(jsonData)},
                             {"message", "200 OK"},
                             {"status", "ok"}};
            res.end();
        });
}

} // namespace obmc_vm
} // namespace crow
