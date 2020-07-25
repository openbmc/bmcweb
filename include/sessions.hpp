#pragma once

#include "logging.h"
#include "utility.h"

#include <openssl/rand.h>

#include <boost/container/flat_map.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <dbus_singleton.hpp>
#include <nlohmann/json.hpp>
#include <pam_authenticate.hpp>
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
    static std::shared_ptr<UserSession> fromJson(const nlohmann::json& j)
    {
        std::shared_ptr<UserSession> userSession =
            std::make_shared<UserSession>();
        for (const auto& element : j.items())
        {
            const std::string* thisValue =
                element.value().get_ptr<const std::string*>();
            if (thisValue == nullptr)
            {
                BMCWEB_LOG_ERROR << "Error reading persistent store.  Property "
                                 << element.key() << " was not of type string";
                return nullptr;
            }
            if (element.key() == "unique_id")
            {
                userSession->uniqueId = *thisValue;
            }
            else if (element.key() == "session_token")
            {
                userSession->sessionToken = *thisValue;
            }
            else if (element.key() == "csrf_token")
            {
                userSession->csrfToken = *thisValue;
            }
            else if (element.key() == "username")
            {
                userSession->username = *thisValue;
            }
            else if (element.key() == "client_id")
            {
                userSession->clientId = *thisValue;
            }
            else if (element.key() == "client_ip")
            {
                userSession->clientIp = *thisValue;
            }

            else
            {
                BMCWEB_LOG_ERROR
                    << "Got unexpected property reading persistent file: "
                    << element.key();
                return nullptr;
            }
        }

        // For now, sessions that were persisted through a reboot get their idle
        // timer reset.  This could probably be overcome with a better
        // understanding of wall clock time and steady timer time, possibly
        // persisting values with wall clock time instead of steady timer, but
        // the tradeoffs of all the corner cases involved are non-trivial, so
        // this is done temporarily
        userSession->lastUpdated = std::chrono::steady_clock::now();
        userSession->persistence = PersistenceType::TIMEOUT;

        return userSession;
    }
};

struct AuthConfigMethods
{
    bool xtoken = true;
    bool cookie = true;
    bool sessionToken = true;
    bool basic = true;
    bool tls = false;

    void fromJson(const nlohmann::json& j)
    {
        for (const auto& element : j.items())
        {
            const bool* value = element.value().get_ptr<const bool*>();
            if (value == nullptr)
            {
                continue;
            }

            if (element.key() == "XToken")
            {
                xtoken = *value;
            }
            else if (element.key() == "Cookie")
            {
                cookie = *value;
            }
            else if (element.key() == "SessionToken")
            {
                sessionToken = *value;
            }
            else if (element.key() == "BasicAuth")
            {
                basic = *value;
            }
            else if (element.key() == "TLS")
            {
                tls = *value;
            }
        }
    }
};

class Middleware;

struct OpenSSLGenerator
{

    uint8_t operator()(void)
    {
        uint8_t index = 0;
        int rc = RAND_bytes(&index, sizeof(index));
        if (rc != opensslSuccess)
        {
            std::cerr << "Cannot get random number\n";
            err = true;
        }

        return index;
    }

    uint8_t max()
    {
        return std::numeric_limits<uint8_t>::max();
    }
    uint8_t min()
    {
        return std::numeric_limits<uint8_t>::min();
    }

    bool error()
    {
        return err;
    }

    // all generators require this variable
    using result_type = uint8_t;

  private:
    // RAND_bytes() returns 1 on success, 0 otherwise. -1 if bad function
    static constexpr int opensslSuccess = 1;
    bool err = false;
};

class SessionStore
{
  public:
    std::shared_ptr<UserSession> generateUserSession(
        const std::string_view username,
        PersistenceType persistence = PersistenceType::TIMEOUT,
        bool isConfigureSelfOnly = false, const std::string_view clientId = "",
        const std::string_view clientIp = "")
    {
        // TODO(ed) find a secure way to not generate session identifiers if
        // persistence is set to SINGLE_REQUEST
        static constexpr std::array<char, 62> alphanum = {
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C',
            'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
            'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c',
            'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
            'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

        std::string sessionToken;
        sessionToken.resize(sessionTokenSize, '0');
        std::uniform_int_distribution<size_t> dist(0, alphanum.size() - 1);

        OpenSSLGenerator gen;

        for (size_t i = 0; i < sessionToken.size(); ++i)
        {
            sessionToken[i] = alphanum[dist(gen)];
            if (gen.error())
            {
                return nullptr;
            }
        }
        // Only need csrf tokens for cookie based auth, token doesn't matter
        std::string csrfToken;
        csrfToken.resize(sessionTokenSize, '0');
        for (size_t i = 0; i < csrfToken.size(); ++i)
        {
            csrfToken[i] = alphanum[dist(gen)];
            if (gen.error())
            {
                return nullptr;
            }
        }

        std::string uniqueId;
        uniqueId.resize(10, '0');
        for (size_t i = 0; i < uniqueId.size(); ++i)
        {
            uniqueId[i] = alphanum[dist(gen)];
            if (gen.error())
            {
                return nullptr;
            }
        }
        auto session = std::make_shared<UserSession>(
            UserSession{uniqueId, sessionToken, std::string(username),
                        csrfToken, std::string(clientId), std::string(clientIp),
                        std::chrono::steady_clock::now(), persistence, false,
                        isConfigureSelfOnly});
        auto it = authTokens.emplace(std::make_pair(sessionToken, session));
        // Only need to write to disk if session isn't about to be destroyed.
        needWrite = persistence == PersistenceType::TIMEOUT;
        return it.first->second;
    }

    std::shared_ptr<UserSession>
        loginSessionByToken(const std::string_view token)
    {
        applySessionTimeouts();
        if (token.size() != sessionTokenSize)
        {
            return nullptr;
        }
        auto sessionIt = authTokens.find(std::string(token));
        if (sessionIt == authTokens.end())
        {
            return nullptr;
        }
        std::shared_ptr<UserSession> userSession = sessionIt->second;
        userSession->lastUpdated = std::chrono::steady_clock::now();
        return userSession;
    }

    std::shared_ptr<UserSession> getSessionByUid(const std::string_view uid)
    {
        applySessionTimeouts();
        // TODO(Ed) this is inefficient
        auto sessionIt = authTokens.begin();
        while (sessionIt != authTokens.end())
        {
            if (sessionIt->second->uniqueId == uid)
            {
                return sessionIt->second;
            }
            sessionIt++;
        }
        return nullptr;
    }

    void removeSession(std::shared_ptr<UserSession> session)
    {
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
        crow::ibm_mc_lock::Lock::getInstance().releaseLock(session->uniqueId);
#endif
        authTokens.erase(session->sessionToken);
        needWrite = true;
    }

    std::vector<const std::string*> getUniqueIds(
        bool getAll = true,
        const PersistenceType& type = PersistenceType::SINGLE_REQUEST)
    {
        applySessionTimeouts();

        std::vector<const std::string*> ret;
        ret.reserve(authTokens.size());
        for (auto& session : authTokens)
        {
            if (getAll || type == session.second->persistence)
            {
                ret.push_back(&session.second->uniqueId);
            }
        }
        return ret;
    }

    void updateAuthMethodsConfig(const AuthConfigMethods& config)
    {
        bool isTLSchanged = (authMethodsConfig.tls != config.tls);
        authMethodsConfig = config;
        needWrite = true;
        if (isTLSchanged)
        {
            // recreate socket connections with new settings
            std::raise(SIGHUP);
        }
    }

    AuthConfigMethods& getAuthMethodsConfig()
    {
        return authMethodsConfig;
    }

    bool needsWrite()
    {
        return needWrite;
    }
    int64_t getTimeoutInSeconds() const
    {
        return std::chrono::seconds(timeoutInMinutes).count();
    }

    static SessionStore& getInstance()
    {
        static SessionStore sessionStore;
        return sessionStore;
    }

    SessionStore(const SessionStore&) = delete;
    SessionStore& operator=(const SessionStore&) = delete;

    std::unordered_map<std::string, std::shared_ptr<UserSession>,
                       std::hash<std::string>,
                       crow::utility::ConstantTimeCompare>
        authTokens;

    std::chrono::time_point<std::chrono::steady_clock> lastTimeoutUpdate;
    bool needWrite{false};
    std::chrono::minutes timeoutInMinutes;
    AuthConfigMethods authMethodsConfig;

  private:
    SessionStore() : timeoutInMinutes(60)
    {}

    void applySessionTimeouts()
    {
        auto timeNow = std::chrono::steady_clock::now();
        if (timeNow - lastTimeoutUpdate > std::chrono::minutes(1))
        {
            lastTimeoutUpdate = timeNow;
            auto authTokensIt = authTokens.begin();
            while (authTokensIt != authTokens.end())
            {
                if (timeNow - authTokensIt->second->lastUpdated >=
                    timeoutInMinutes)
                {
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
                    crow::ibm_mc_lock::Lock::getInstance().releaseLock(
                        authTokensIt->second->uniqueId);
#endif
                    authTokensIt = authTokens.erase(authTokensIt);

                    needWrite = true;
                }
                else
                {
                    authTokensIt++;
                }
            }
        }
    }
};

} // namespace persistent_data

// to_json(...) definition for objects of UserSession type
namespace nlohmann
{
template <>
struct adl_serializer<std::shared_ptr<persistent_data::UserSession>>
{
    static void to_json(nlohmann::json& j,
                        const std::shared_ptr<persistent_data::UserSession>& p)
    {
        if (p->persistence != persistent_data::PersistenceType::SINGLE_REQUEST)
        {
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
            j = nlohmann::json{
                {"unique_id", p->uniqueId}, {"session_token", p->sessionToken},
                {"username", p->username},  {"csrf_token", p->csrfToken},
                {"client_id", p->clientId}, { "client_ip", p->clientIp }};
#else
            j = nlohmann::json{{"unique_id", p->uniqueId},
                               {"session_token", p->sessionToken},
                               {"username", p->username},
                               {"csrf_token", p->csrfToken},
                               {"client_ip", p->clientIp}};
#endif
        }
    }
};

template <>
struct adl_serializer<persistent_data::AuthConfigMethods>
{
    static void to_json(nlohmann::json& j,
                        const persistent_data::AuthConfigMethods& c)
    {
        j = nlohmann::json{{"XToken", c.xtoken},
                           {"Cookie", c.cookie},
                           {"SessionToken", c.sessionToken},
                           {"BasicAuth", c.basic},
                           {"TLS", c.tls}};
    }
};
} // namespace nlohmann
