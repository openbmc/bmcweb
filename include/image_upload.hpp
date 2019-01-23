#pragma once

#include <crow/app.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <cstdio>
#include <dbus_singleton.hpp>
#include <fstream>
#include <memory>

namespace crow
{
namespace image_upload
{

std::unique_ptr<sdbusplus::bus::match::match> fwUpdateMatcher;

inline void uploadImageHandler(const crow::Request& req, crow::Response& res,
                               const std::string& filename)
{
    // Only allow one FW update at a time
    if (fwUpdateMatcher != nullptr)
    {
        res.addHeader("Retry-After", "30");
        res.result(boost::beast::http::status::service_unavailable);
        res.end();
        return;
    }
    // Make this const static so it survives outside this method
    static boost::asio::deadline_timer timeout(*req.ioService,
                                               boost::posix_time::seconds(5));

    timeout.expires_from_now(boost::posix_time::seconds(10));

    timeout.async_wait([&res](const boost::system::error_code& ec) {
        fwUpdateMatcher = nullptr;
        if (ec == asio::error::operation_aborted)
        {
            // expected, we were canceled before the timer completed.
            return;
        }
        BMCWEB_LOG_ERROR << "Timed out waiting for Version interface";

        if (ec)
        {
            BMCWEB_LOG_ERROR << "Async_wait failed " << ec;
            return;
        }

        res.result(boost::beast::http::status::bad_request);
        res.jsonValue = {
            {"data",
             {{"description",
               "Version already exists or failed to be extracted"}}},
            {"message", "400 Bad Request"},
            {"status", "error"}};
        res.end();
    });

    std::function<void(sdbusplus::message::message&)> callback =
        [&res](sdbusplus::message::message& m) {
            BMCWEB_LOG_DEBUG << "Match fired";

            sdbusplus::message::object_path path;
            std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::variant<std::string>>>>>
                interfaces;
            m.read(path, interfaces);

            if (std::find_if(interfaces.begin(), interfaces.end(),
                             [](const auto& i) {
                                 return i.first ==
                                        "xyz.openbmc_project.Software.Version";
                             }) != interfaces.end())
            {
                boost::system::error_code ec;
                timeout.cancel(ec);
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "error canceling timer " << ec;
                }

                std::size_t index = path.str.rfind('/');
                if (index != std::string::npos)
                {
                    path.str.erase(0, index + 1);
                }
                res.jsonValue = {{"data", std::move(path.str)},
                                 {"message", "200 OK"},
                                 {"status", "ok"}};
                BMCWEB_LOG_DEBUG << "ending response";
                res.end();
                fwUpdateMatcher = nullptr;
            }
        };
    fwUpdateMatcher = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "interface='org.freedesktop.DBus.ObjectManager',type='signal',"
        "member='InterfacesAdded',path='/xyz/openbmc_project/software'",
        callback);

    std::string filepath(
        "/tmp/images/" +
        boost::uuids::to_string(boost::uuids::random_generator()()));
    BMCWEB_LOG_DEBUG << "Writing file to " << filepath;
    std::ofstream out(filepath, std::ofstream::out | std::ofstream::binary |
                                    std::ofstream::trunc);
    out << req.body;
    out.close();
}

template <typename... Middlewares> void requestRoutes(Crow<Middlewares...>& app)
{
    BMCWEB_ROUTE(app, "/upload/image/<str>")
        .methods("POST"_method,
                 "PUT"_method)([](const crow::Request& req, crow::Response& res,
                                  const std::string& filename) {
            uploadImageHandler(req, res, filename);
        });

    BMCWEB_ROUTE(app, "/upload/image")
        .methods("POST"_method, "PUT"_method)(
            [](const crow::Request& req, crow::Response& res) {
                uploadImageHandler(req, res, "");
            });
}
} // namespace image_upload
} // namespace crow
