#pragma once

#include <dbus_singleton.hpp>
#include <fstream>
#include <crow/app.h>

namespace crow {
namespace intel_oem {

template <typename... Middlewares>
void request_routes(Crow<Middlewares...>& app) {
  CROW_ROUTE(app, "/intel/firmwareupload")
      .methods(
          "POST"_method)([](const crow::request& req, crow::response& res) {
        std::string filepath("/tmp/fw_update_image");
        std::ofstream out(filepath, std::ofstream::out | std::ofstream::binary |
                                        std::ofstream::trunc);
        out << req.body;
        out.close();
        crow::connections::system_bus->async_method_call(
            [&](boost::system::error_code ec) {
              std::cout << "async_method_call callback\n";
              if (ec) {
                std::cerr << "error with async_method_call \n";
                res.json_value["status"] = "Upload failed";
              } else {
                res.json_value["status"] = "Upload Successful";
              }
              res.end();
            },
            "xyz.openbmc_project.fwupdate1.server",
            "/xyz/openbmc_project/fwupdate1", "xyz.openbmc_project.fwupdate1",
            "start", "file://" + filepath);

      });
}
}  // namespace intel_oem
}  // namespace crow
