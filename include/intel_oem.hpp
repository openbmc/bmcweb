#pragma once

#include <dbus_singleton.hpp>
#include <fstream>
#include <crow/app.h>

namespace crow {
namespace intel_oem {

template <typename... Middlewares>
void request_routes(Crow<Middlewares...>& app) {
  CROW_ROUTE(app, "/intel/firmwareupload")
      .methods("POST"_method)([](const crow::request& req) {
        std::string filepath("/tmp/fw_update_image");
        std::ofstream out(filepath, std::ofstream::out | std::ofstream::binary |
                                        std::ofstream::trunc);
        out << req.body;
        out.close();

        auto m = dbus::message::new_call(
            {"xyz.openbmc_project.fwupdate1.server",
             "/xyz/openbmc_project/fwupdate1", "xyz.openbmc_project.fwupdate1"},
            "start");

        m.pack(std::string("file://") + filepath);
        crow::connections::system_bus->send(m);
        nlohmann::json j;
        j["status"] = "Upload Successful";
        return j;
      });
}
}  // namespace redfish
}  // namespace crow
