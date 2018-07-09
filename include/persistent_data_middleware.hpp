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
      if (data.is_discarded()) {
        CROW_LOG_ERROR << "Error parsing persistent data in json file.";
      } else {
        for (const auto& item : data.items()) {
          if (item.key() == "revision") {
            file_revision = 0;

            const uint64_t* uintPtr = item.value().get_ptr<const uint64_t*>();
            if (uintPtr == nullptr) {
              CROW_LOG_ERROR << "Failed to read revision flag";
            } else {
              file_revision = *uintPtr;
            }
          } else if (item.key() == "system_uuid") {
            const std::string* jSystemUuid =
                item.value().get_ptr<const std::string*>();
            if (jSystemUuid != nullptr) {
              system_uuid = *jSystemUuid;
            }
          } else if (item.key() == "sessions") {
            for (const auto& elem : item.value()) {
              std::shared_ptr<UserSession> newSession =
                  UserSession::fromJson(elem);

              if (newSession == nullptr) {
                CROW_LOG_ERROR
                    << "Problem reading session from persistent store";
                continue;
              }

              CROW_LOG_DEBUG << "Restored session: " << newSession->csrf_token
                             << " " << newSession->unique_id << " "
                             << newSession->session_token;
              SessionStore::getInstance().auth_tokens.emplace(
                  newSession->session_token, newSession);
            }
          } else {
            // Do nothing in the case of extra fields.  We may have cases where
            // fields are added in the future, and we want to at least attempt
            // to gracefully support downgrades in that case, even if we don't
            // officially support it
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
    nlohmann::json data{
        {"sessions", PersistentData::SessionStore::getInstance().auth_tokens},
        {"system_uuid", system_uuid},
        {"revision", json_revision}};
    persistent_file << data;
  }

  std::string system_uuid{""};
};

}  // namespace PersistentData
}  // namespace crow
