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
   * @return a shared pointer if data has been loaded properly, nullptr otherwise
   */
  static std::shared_ptr<UserSession> fromJson(const nlohmann::json& j) {
    std::shared_ptr<UserSession> userSession = std::make_shared<UserSession>();
    for (const auto& element : j.items()) {
      const std::string* thisValue =
          element.value().get_ptr<const std::string*>();
      if (thisValue == nullptr) {
        CROW_LOG_ERROR << "Error reading persistent store.  Property "
                       << element.key() << " was not of type string";
        return nullptr;
      }
      if (element.key() == "unique_id") {
        userSession->unique_id = *thisValue;
      } else if (element.key() == "session_token") {
        userSession->session_token = *thisValue;
      } else if (element.key() == "csrf_token") {
        userSession->csrf_token = *thisValue;
      } else if (element.key() == "username") {
        userSession->username = *thisValue;
      } else {
        CROW_LOG_ERROR << "Got unexpected property reading persistent file: "
                       << element.key();
        return nullptr;
      }
    }

    // For now, sessions that were persisted through a reboot get their idle
    // timer reset.  This could probably be overcome with a better understanding
    // of wall clock time and steady timer time, possibly persisting values with
    // wall clock time instead of steady timer, but the tradeoffs of all the
    // corner cases involved are non-trivial, so this is done temporarily
    userSession->last_updated = std::chrono::steady_clock::now();
    userSession->persistence = PersistenceType::TIMEOUT;

    return userSession;
  }
};

class Middleware;

class SessionStore {
 public:
  std::shared_ptr<UserSession> generate_user_session(
      const boost::string_view username,
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
    auto session = std::make_shared<UserSession>(
        UserSession{unique_id, session_token, std::string(username), csrf_token,
                    std::chrono::steady_clock::now(), persistence});
    auto it = auth_tokens.emplace(std::make_pair(session_token, session));
    // Only need to write to disk if session isn't about to be destroyed.
    need_write_ = persistence == PersistenceType::TIMEOUT;
    return it.first->second;
  }

  std::shared_ptr<UserSession> login_session_by_token(
      const boost::string_view token) {
    apply_session_timeouts();
    auto session_it = auth_tokens.find(std::string(token));
    if (session_it == auth_tokens.end()) {
      return nullptr;
    }
    std::shared_ptr<UserSession> user_session = session_it->second;
    user_session->last_updated = std::chrono::steady_clock::now();
    return user_session;
  }

  std::shared_ptr<UserSession> get_session_by_uid(
      const boost::string_view uid) {
    apply_session_timeouts();
    // TODO(Ed) this is inefficient
    auto session_it = auth_tokens.begin();
    while (session_it != auth_tokens.end()) {
      if (session_it->second->unique_id == uid) {
        return session_it->second;
      }
      session_it++;
    }
    return nullptr;
  }

  void remove_session(std::shared_ptr<UserSession> session) {
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
      if (getAll || type == session.second->persistence) {
        ret.push_back(&session.second->unique_id);
      }
    }
    return ret;
  }

  bool needs_write() { return need_write_; }
  int get_timeout_in_seconds() const {
    return std::chrono::seconds(timeout_in_minutes).count();
  };

  // Persistent data middleware needs to be able to serialize our auth_tokens
  // structure, which is private
  friend Middleware;

  static SessionStore& getInstance() {
    static SessionStore sessionStore;
    return sessionStore;
  }

  SessionStore(const SessionStore&) = delete;
  SessionStore& operator=(const SessionStore&) = delete;

 private:
  SessionStore() : timeout_in_minutes(60) {}

  void apply_session_timeouts() {
    auto time_now = std::chrono::steady_clock::now();
    if (time_now - last_timeout_update > std::chrono::minutes(1)) {
      last_timeout_update = time_now;
      auto auth_tokens_it = auth_tokens.begin();
      while (auth_tokens_it != auth_tokens.end()) {
        if (time_now - auth_tokens_it->second->last_updated >=
            timeout_in_minutes) {
          auth_tokens_it = auth_tokens.erase(auth_tokens_it);
          need_write_ = true;
        } else {
          auth_tokens_it++;
        }
      }
    }
  }
  std::chrono::time_point<std::chrono::steady_clock> last_timeout_update;
  boost::container::flat_map<std::string, std::shared_ptr<UserSession>>
      auth_tokens;
  std::random_device rd;
  bool need_write_{false};
  std::chrono::minutes timeout_in_minutes;
};

}  // namespace PersistentData
}  // namespace crow

// to_json(...) definition for objects of UserSession type
namespace nlohmann {
template <>
struct adl_serializer<std::shared_ptr<crow::PersistentData::UserSession>> {
  static void to_json(
      nlohmann::json& j,
      const std::shared_ptr<crow::PersistentData::UserSession>& p) {
    if (p->persistence !=
        crow::PersistentData::PersistenceType::SINGLE_REQUEST) {
      j = nlohmann::json{{"unique_id", p->unique_id},
                         {"session_token", p->session_token},
                         {"username", p->username},
                         {"csrf_token", p->csrf_token}};
    }
  }
};
}  // namespace nlohmann
