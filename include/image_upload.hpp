#pragma once

#include <dbus_singleton.hpp>
#include <cstdio>
#include <fstream>
#include <memory>
#include <crow/app.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace crow {
namespace image_upload {

std::unique_ptr<sdbusplus::bus::match::match> fwupdatematcher;

inline void uploadImageHandler(const crow::request& req, crow::response& res,
                               const std::string& filename) {
  // Only allow one FW update at a time
  if (fwupdatematcher != nullptr) {
    res.add_header("Retry-After", "30");
    res.result(boost::beast::http::status::service_unavailable);
    res.end();
    return;
  }
  // Make this const static so it survives outside this method
  static boost::asio::deadline_timer timeout(*req.io_service,
                                             boost::posix_time::seconds(5));

  timeout.expires_from_now(boost::posix_time::seconds(5));

  timeout.async_wait([&res](const boost::system::error_code& ec) {
    fwupdatematcher = nullptr;
    if (ec == asio::error::operation_aborted) {
      // expected, we were canceled before the timer completed.
      return;
    }
    CROW_LOG_ERROR << "Timed out waiting for log event";

    if (ec) {
      CROW_LOG_ERROR << "Async_wait failed " << ec;
      return;
    }

    res.result(boost::beast::http::status::internal_server_error);
    res.end();
  });

  std::function<void(sdbusplus::message::message&)> callback =
      [&res](sdbusplus::message::message& m) {
        CROW_LOG_DEBUG << "Match fired";
        boost::system::error_code ec;
        timeout.cancel(ec);
        if (ec) {
          CROW_LOG_ERROR << "error canceling timer " << ec;
        }
        std::string versionInfo;
        m.read(versionInfo);  // Read in the object path that was just created

        std::size_t index = versionInfo.rfind('/');
        if (index != std::string::npos) {
          versionInfo.erase(0, index);
        }
        res.json_value = {{"data", std::move(versionInfo)},
                          {"message", "200 OK"},
                          {"status", "ok"}};
        CROW_LOG_DEBUG << "ending response";
        res.end();
        fwupdatematcher = nullptr;
      };
  fwupdatematcher = std::make_unique<sdbusplus::bus::match::match>(
      *crow::connections::system_bus,
      "interface='org.freedesktop.DBus.ObjectManager',type='signal',"
      "member='InterfacesAdded',path='/xyz/openbmc_project/logging'",
      callback);

  std::string filepath(
      "/tmp/images/" +
      boost::uuids::to_string(boost::uuids::random_generator()()));
  CROW_LOG_DEBUG << "Writing file to " << filepath;
  std::ofstream out(filepath, std::ofstream::out | std::ofstream::binary |
                                  std::ofstream::trunc);
  out << req.body;
  out.close();
}

template <typename... Middlewares>
void request_routes(Crow<Middlewares...>& app) {
  CROW_ROUTE(app, "/upload/image/<str>")
      .methods("POST"_method,
               "PUT"_method)([](const crow::request& req, crow::response& res,
                                const std::string& filename) {
        uploadImageHandler(req, res, filename);
      });

  CROW_ROUTE(app, "/upload/image")
      .methods("POST"_method,
               "PUT"_method)([](const crow::request& req, crow::response& res) {
        uploadImageHandler(req, res, "");
      });
}
}  // namespace image_upload
}  // namespace crow
