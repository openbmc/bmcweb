#pragma once

#include "logging.hpp"
#include "random.hpp"
#include "utility.hpp"

#include <openssl/rand.h>

#include <boost/container/flat_map.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <dbus_singleton.hpp>
#include <nlohmann/json.hpp>
#include <pam_authenticate.hpp>
#include <random.hpp>
#include <sdbusplus/bus/match.hpp>

#include <csignal>
#include <random>
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
#include <ibm/locks.hpp>
#endif

namespace persistent_data
{

// entropy: 20 characters, 62 possibilities.  log2(62^20) = 119 bits of
// entropy.  OWASP recommends at least 64
// https://cheatsheetseries.owasp.org/cheatsheets/Session_Management_Cheat_Sheet.html#session-id-entropy
constexpr std::size_t sessionTokenSize = 20;

enum class PersistenceType
{
    TIMEOUT, // User session times out after a predetermined amount of time
    SINGLE_REQUEST // User times out once this request is completed.
};

struct UserSession
{
    std::string uniqueId;
    std::string sessionToken;
    std::string username;
    std::string csrfToken;
    std::string clientId;
    std::string clientIp;
    std::chrono::time_point<std::chrono::steady_clock> lastUpdated;
    PersistenceType persistence;
    bool cookieAuth = false;
    bool isConfigureSelfOnly = false;

    // There are two sources of truth for isConfigureSelfOnly:
    //  1. When pamAuthenticateUser() returns PAM_NEW_AUTHTOK_REQD.
    //  2. D-Bus User.Manager.GetUserInfo property UserPasswordExpired.
    // These should be in sync, but the underlying condition can change at any
    // time.  For example, a password can expire or be changed outside of
    // bmcweb.  The value stored here is updated at the start of each
    // operation and used as the truth within bmcweb.

    /**
     * @brief Fills object with data from UserSession's JSON representation
     *
     * This replaces nlohmann's from_json to ensure no-throw approach
     *
     * @param[in] j   JSON object from which data should be loaded
     *
     * @return a shared pointer if data has been loaded properly, nullptr
     * otherwise
     */
    static std::shared_ptr<UserSession> fromJson(const nlohmann::json& j);
};

struct AuthConfigMethods
{

    #ifdef BMCWEB_ENABLE_BASIC_AUTHENTICATION
        bool basic = true;
    #else
        bool basic = false;
    #endif

    #ifdef BMCWEB_ENABLE_SESSION_AUTHENTICATION
        bool sessionToken = true;
    #else
        bool sessionToken = false;
    #endif

    #ifdef BMCWEB_ENABLE_XTOKEN_AUTHENTICATION
        bool xtoken = true;
    #else
        bool xtoken = false;
    #endif

    #ifdef BMCWEB_ENABLE_COOKIE_AUTHENTICATION
        bool cookie = true;
    #else
        bool cookie = false;
    #endif

    #ifdef BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION
        bool tls = true;
    #else
        bool tls = false;
    #endif

    void fromJson(const nlohmann::json& j);
};

class SessionStore
{
  public:
    std::shared_ptr<UserSession> generateUserSession(
        const std::string_view username, const std::string_view clientIp,
        const std::string_view clientId,
        PersistenceType persistence = PersistenceType::TIMEOUT,
        bool isConfigureSelfOnly = false);
    std::shared_ptr<UserSession>
        loginSessionByToken(const std::string_view token);
    std::shared_ptr<UserSession> getSessionByUid(const std::string_view uid);
    void removeSession(const std::shared_ptr<UserSession>& session);
    std::vector<const std::string*> getUniqueIds(
        bool getAll = true,
        const PersistenceType& type = PersistenceType::SINGLE_REQUEST);
    void updateAuthMethodsConfig(const AuthConfigMethods& config);
    AuthConfigMethods& getAuthMethodsConfig();
    bool needsWrite();
    int64_t getTimeoutInSeconds() const;
    void updateSessionTimeout(std::chrono::seconds newTimeoutInSeconds);
    static SessionStore& getInstance();
    void applySessionTimeouts();

    SessionStore(const SessionStore&) = delete;
    SessionStore& operator=(const SessionStore&) = delete;

    std::unordered_map<std::string, std::shared_ptr<UserSession>,
                       std::hash<std::string>,
                       crow::utility::ConstantTimeCompare>
        authTokens;

    std::chrono::time_point<std::chrono::steady_clock> lastTimeoutUpdate;
    bool needWrite{false};
    std::chrono::seconds timeoutInSeconds;
    AuthConfigMethods authMethodsConfig;

  private:
    SessionStore() : timeoutInSeconds(1800)
    {}
};

} // namespace persistent_data
