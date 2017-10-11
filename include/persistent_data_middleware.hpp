#pragma once

#include <base64.hpp>
#include <nlohmann/json.hpp>
#include <pam_authenticate.hpp>
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
struct UserSession {
  std::string unique_id;
  std::string session_token;
  std::string username;
  std::string csrf_token;
};

void to_json(nlohmann::json& j, const UserSession& p) {
  j = nlohmann::json{{"unique_id", p.unique_id},
                     {"session_token", p.session_token},
                     {"username", p.username},
                     {"csrf_token", p.csrf_token}};
}

void from_json(const nlohmann::json& j, UserSession& p) {
  try {
    p.unique_id = j.at("unique_id").get<std::string>();
    p.session_token = j.at("session_token").get<std::string>();
    p.username = j.at("username").get<std::string>();
    p.csrf_token = j.at("csrf_token").get<std::string>();
  } catch (std::out_of_range) {
    // do nothing.  Session API incompatibility, leave sessions empty
  }
}

class Middleware {
  using SessionStore = boost::container::flat_map<std::string, UserSession>;
  // todo(ed) should read this from a fixed location somewhere, not CWD
  static constexpr const char* filename = "bmcweb_persistent_data.json";
  int json_revision = 1;

 public:
  struct context {
    SessionStore* auth_tokens;
  };

  Middleware() { read_data(); }

  void before_handle(crow::request& req, response& res, context& ctx) {
    ctx.auth_tokens = &auth_tokens;
  }

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
        file_revision = data["revision"].get<int>();
        auth_tokens = data["sessions"].get<SessionStore>();
        system_uuid = data["system_uuid"].get<std::string>();
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

    if (need_write) {
      write_data();
    }
  }

  void write_data() {
    std::ofstream persistent_file(filename);
    nlohmann::json data;
    data["sessions"] = auth_tokens;
    data["system_uuid"] = system_uuid;
    data["revision"] = json_revision;
    persistent_file << data;
  }

  UserSession generate_user_session(const std::string& username) {
    static constexpr std::array<char, 62> alphanum = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C',
        'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c',
        'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
        'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

    // entropy: 30 characters, 62 possibilies.  log2(62^30) = 178 bits of
    // entropy.  OWASP recommends at least 60
    // https://www.owasp.org/index.php/Session_Management_Cheat_Sheet#Session_ID_Entropy
    std::string session_token;
    session_token.resize(20, '0');
    std::uniform_int_distribution<int> dist(0, alphanum.size() - 1);
    for (int i = 0; i < session_token.size(); ++i) {
      session_token[i] = alphanum[dist(rd)];
    }
    // Only need csrf tokens for cookie based auth, token doesn't matter
    std::string csrf_token;
    csrf_token.resize(20, '0');
    for (int i = 0; i < csrf_token.size(); ++i) {
      csrf_token[i] = alphanum[dist(rd)];
    }

    std::string unique_id;
    unique_id.resize(10, '0');
    for (int i = 0; i < unique_id.size(); ++i) {
      unique_id[i] = alphanum[dist(rd)];
    }
    UserSession session{unique_id, session_token, username, csrf_token};
    auth_tokens.emplace(session_token, session);
    write_data();
    return session;
  }

  SessionStore auth_tokens;
  std::string system_uuid;
  std::random_device rd;
};

}  // namespaec PersistentData
}  // namespace crow
