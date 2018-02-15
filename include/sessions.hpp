#pragma once

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

enum class PersistenceType {
  TIMEOUT,        // User session times out after a predetermined amount of time
  SINGLE_REQUEST  // User times out once this request is completed.
};

struct UserSession {
  std::string unique_id;
  std::string session_token;
  std::string username;
  std::string csrf_token;
  std::chrono::time_point<std::chrono::steady_clock> last_updated;
  PersistenceType persistence;

  /**
   * @brief Fills object with data from UserSession's JSON representation
   *
   * This replaces nlohmann's from_json to ensure no-throw approach
   *
   * @param[in] j   JSON object from which data should be loaded
   *
   * @return true if data has been loaded properly, false otherwise
   */
  bool fromJson(const nlohmann::json& j) {
    auto jUid = j.find("unique_id");
    auto jToken = j.find("session_token");
    auto jUsername = j.find("username");
    auto jCsrf = j.find("csrf_token");

    // Verify existence
    if (jUid == j.end() || jToken == j.end() || jUsername == j.end() ||
        jCsrf == j.end()) {
      return false;
    }

    // Verify types
    if (!jUid->is_string() || !jToken->is_string() || !jUsername->is_string() ||
        !jCsrf->is_string()) {
      return false;
    }

    unique_id = jUid->get<std::string>();
    session_token = jToken->get<std::string>();
    username = jUsername->get<std::string>();
    csrf_token = jCsrf->get<std::string>();

    // For now, sessions that were persisted through a reboot get their timer
    // reset.  This could probably be overcome with a better understanding of
    // wall clock time and steady timer time, possibly persisting values with
    // wall clock time instead of steady timer, but the tradeoffs of all the
    // corner cases involved are non-trivial, so this is done temporarily
    last_updated = std::chrono::steady_clock::now();
    persistence = PersistenceType::TIMEOUT;

    return true;
  }
};

void to_json(nlohmann::json& j, const UserSession& p) {
  if (p.persistence != PersistenceType::SINGLE_REQUEST) {
    j = nlohmann::json{{"unique_id", p.unique_id},
                       {"session_token", p.session_token},
                       {"username", p.username},
                       {"csrf_token", p.csrf_token}};
  }
}

class Middleware;

class SessionStore {
 public:
  const UserSession& generate_user_session(
      const std::string& username,
      PersistenceType persistence = PersistenceType::TIMEOUT) {
    // TODO(ed) find a secure way to not generate session identifiers if
    // persistence is set to SINGLE_REQUEST
    static constexpr std::array<char, 62> alphanum = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C',
        'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c',
        'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
        'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

    // entropy: 30 characters, 62 possibilities.  log2(62^30) = 178 bits of
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

    const auto session_it = auth_tokens.emplace(
        session_token,
        std::move(UserSession{unique_id, session_token, username, csrf_token,
                              std::chrono::steady_clock::now(), persistence}));
    const UserSession& user = (session_it).first->second;
    // Only need to write to disk if session isn't about to be destroyed.
    need_write_ = persistence == PersistenceType::TIMEOUT;
    return user;
  }

  const UserSession* login_session_by_token(const std::string& token) {
    apply_session_timeouts();
    auto session_it = auth_tokens.find(token);
    if (session_it == auth_tokens.end()) {
      return nullptr;
    }
    UserSession& foo = session_it->second;
    foo.last_updated = std::chrono::steady_clock::now();
    return &foo;
  }

  const UserSession* get_session_by_uid(const std::string& uid) {
    apply_session_timeouts();
    // TODO(Ed) this is inefficient
    auto session_it = auth_tokens.begin();
    while (session_it != auth_tokens.end()) {
      if (session_it->second.unique_id == uid) {
        return &session_it->second;
      }
      session_it++;
    }
    return nullptr;
  }

  void remove_session(const UserSession* session) {
    auth_tokens.erase(session->session_token);
    need_write_ = true;
  }

  std::vector<const std::string*> get_unique_ids(
      bool getAll = true,
      const PersistenceType& type = PersistenceType::SINGLE_REQUEST) {
    apply_session_timeouts();

    std::vector<const std::string*> ret;
    ret.reserve(auth_tokens.size());
    for (auto& session : auth_tokens) {
      if (getAll || type == session.second.persistence) {
        ret.push_back(&session.second.unique_id);
      }
    }
    return ret;
  }

  bool needs_write() { return need_write_; }

  // Persistent data middleware needs to be able to serialize our auth_tokens
  // structure, which is private
  friend Middleware;

 private:
  void apply_session_timeouts() {
    std::chrono::minutes timeout(60);
    auto time_now = std::chrono::steady_clock::now();
    if (time_now - last_timeout_update > std::chrono::minutes(1)) {
      last_timeout_update = time_now;
      auto auth_tokens_it = auth_tokens.begin();
      while (auth_tokens_it != auth_tokens.end()) {
        if (time_now - auth_tokens_it->second.last_updated >= timeout) {
          auth_tokens_it = auth_tokens.erase(auth_tokens_it);
          need_write_ = true;
        } else {
          auth_tokens_it++;
        }
      }
    }
  }
  std::chrono::time_point<std::chrono::steady_clock> last_timeout_update;
  boost::container::flat_map<std::string, UserSession> auth_tokens;
  std::random_device rd;
  bool need_write_{false};
};

}  // namespaec PersistentData
}  // namespace crow
