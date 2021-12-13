#pragma once

#include <app.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <dbus_singleton.hpp>
#include <dbus_utility.hpp>

#include <cstdio>
#include <fstream>
#include <memory>

namespace crow
{
namespace image_upload
{

static std::unique_ptr<sdbusplus::bus::match::match> fwUpdateMatcher;

inline void
    uploadImageHandler(const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    // Only allow one FW update at a time
    if (fwUpdateMatcher != nullptr)
    {
        asyncResp->res.addHeader("Retry-After", "30");
        asyncResp->res.result(boost::beast::http::status::service_unavailable);
        return;
    }
    // Make this const static so it survives outside this method
    static boost::asio::steady_timer timeout(*req.ioService,
                                             std::chrono::seconds(5));

    timeout.expires_after(std::chrono::seconds(15));

    auto timeoutHandler = [asyncResp](const boost::system::error_code& ec) {
        fwUpdateMatcher = nullptr;
        if (ec == boost::asio::error::operation_aborted)
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

        asyncResp->res.result(boost::beast::http::status::bad_request);
        asyncResp->res.jsonValue = {
            {"data",
             {{"description",
               "Version already exists or failed to be extracted"}}},
            {"message", "400 Bad Request"},
            {"status", "error"}};
    };

    std::function<void(sdbusplus::message::message&)> callback =
        [asyncResp](sdbusplus::message::message& m) {
            BMCWEB_LOG_DEBUG << "Match fired";

            sdbusplus::message::object_path path;
            std::vector<std::pair<
                std::string, std::vector<std::pair<
                                 std::string, dbus::utility::DbusVariantType>>>>
                interfaces;
            m.read(path, interfaces);

            if (std::find_if(interfaces.begin(), interfaces.end(),
                             [](const auto& i) {
                                 return i.first ==
                                        "xyz.openbmc_project.Software.Version";
                             }) != interfaces.end())
            {
                timeout.cancel();
                std::string leaf = path.filename();
                if (leaf.empty())
                {
                    leaf = path.str;
                }

                asyncResp->res.jsonValue = {
                    {"data", leaf}, {"message", "200 OK"}, {"status", "ok"}};
                BMCWEB_LOG_DEBUG << "ending response";
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
    timeout.async_wait(timeoutHandler);
}

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/upload/image/<str>")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .methods(boost::beast::http::verb::post, boost::beast::http::verb::put)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string&) { uploadImageHandler(req, asyncResp); });

    BMCWEB_ROUTE(app, "/upload/image")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .methods(boost::beast::http::verb::post, boost::beast::http::verb::put)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                uploadImageHandler(req, asyncResp);
            });
}
} // namespace image_upload
} // namespace crow
