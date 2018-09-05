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

    timeout.expires_from_now(boost::posix_time::seconds(5));

    timeout.async_wait([&res](const boost::system::error_code& ec) {
        fwUpdateMatcher = nullptr;
        if (ec == asio::error::operation_aborted)
        {
            // expected, we were canceled before the timer completed.
            return;
        }
        BMCWEB_LOG_ERROR << "Timed out waiting for log event";

        if (ec)
        {
            BMCWEB_LOG_ERROR << "Async_wait failed " << ec;
            return;
        }

        res.result(boost::beast::http::status::internal_server_error);
        res.end();
    });

    std::function<void(sdbusplus::message::message&)> callback =
        [&res](sdbusplus::message::message& m) {
            BMCWEB_LOG_DEBUG << "Match fired";
            boost::system::error_code ec;
            timeout.cancel(ec);
            if (ec)
            {
                BMCWEB_LOG_ERROR << "error canceling timer " << ec;
            }
            std::string versionInfo;
            m.read(
                versionInfo); // Read in the object path that was just created

            std::size_t index = versionInfo.rfind('/');
            if (index != std::string::npos)
            {
                versionInfo.erase(0, index);
            }
            res.jsonValue = {{"data", std::move(versionInfo)},
                             {"message", "200 OK"},
                             {"status", "ok"}};
            BMCWEB_LOG_DEBUG << "ending response";
            res.end();
            fwUpdateMatcher = nullptr;
        };
    fwUpdateMatcher = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "interface='org.freedesktop.DBus.ObjectManager',type='signal',"
        "member='InterfacesAdded',path='/xyz/openbmc_project/logging'",
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
