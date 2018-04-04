#pragma once

#include <nlohmann/json.hpp>
#include <pam_authenticate.hpp>
#include <sessions.hpp>
#include <webassets.hpp>
#include <random>
#include <crow/app.h>
#include <crow/http_request.h>
#include <crow/http_response.h>
#include <boost/container/flat_map.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace crow {

namespace PersistentData {

class Middleware {
  // todo(ed) should read this from a fixed location somewhere, not CWD
  static constexpr const char* filename = "bmcweb_persistent_data.json";
  int json_revision = 1;

 public:
  struct context {};

  Middleware() { read_data(); }

  ~Middleware() {
    if (PersistentData::SessionStore::getInstance().needs_write()) {
      write_data();
    }
  }

  void before_handle(crow::request& req, response& res, context& ctx) {}

  void after_handle(request& req, response& res, context& ctx) {}

  // TODO(ed) this should really use protobuf, or some other serialization
  // library, but adding another dependency is somewhat outside the scope of
  // this application for the moment
  void read_data() {
    std::ifstream persistent_file(filename);
    int file_revision = 0;
    if (persistent_file.is_open()) {
      // call with exceptions disabled
      auto data = nlohmann::json::parse(persistent_file, nullptr, false);
      if (!data.is_discarded()) {
        auto jRevision = data.find("revision");
        auto jUuid = data.find("system_uuid");
        auto jSessions = data.find("sessions");

        file_revision = 0;
        if (jRevision != data.end()) {
          if (jRevision->is_number_integer()) {
            file_revision = jRevision->get<int>();
          }
        }

        system_uuid = "";
        if (jUuid != data.end()) {
          if (jUuid->is_string()) {
            system_uuid = jUuid->get<std::string>();
          }
        }

        if (jSessions != data.end()) {
          if (jSessions->is_object()) {
            for (const auto& elem : *jSessions) {
              std::shared_ptr<UserSession> newSession =
                  std::make_shared<UserSession>();

              if (newSession->fromJson(elem)) {
                SessionStore::getInstance().auth_tokens.emplace(
                    newSession->unique_id, newSession);
              }
            }
          }
        }
      }
    }
    bool need_write = false;

    if (system_uuid.empty()) {
      system_uuid = boost::uuids::to_string(boost::uuids::random_generator()());
      need_write = true;
    }
    if (file_revision < json_revision) {
      need_write = true;
    }
    // write revision changes or system uuid changes immediately
    if (need_write) {
      write_data();
    }
  }

  void write_data() {
    std::ofstream persistent_file(filename);
    nlohmann::json data;
    data["sessions"] = PersistentData::SessionStore::getInstance().auth_tokens;
    data["system_uuid"] = system_uuid;
    data["revision"] = json_revision;
    persistent_file << data;
  }

  std::string system_uuid;
};

}  // namespace PersistentData
}  // namespace crow
