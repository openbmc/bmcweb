#pragma once

#include <boost/container/flat_map.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <csignal>
#include <dbus_singleton.hpp>
#include <nlohmann/json.hpp>
#include <pam_authenticate.hpp>
#include <random>
#include <sdbusplus/bus/match.hpp>

#include "logging.h"
#include "utility.h"

namespace crow
{

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
    std::chrono::time_point<std::chrono::steady_clock> lastUpdated;
    PersistenceType persistence;

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

class SessionStore
{
  public:
    std::shared_ptr<UserSession> generateUserSession(
        const std::string_view username,
        PersistenceType persistence = PersistenceType::TIMEOUT)
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
        for (size_t i = 0; i < sessionToken.size(); ++i)
        {
            sessionToken[i] = alphanum[dist(rd)];
        }
        // Only need csrf tokens for cookie based auth, token doesn't matter
        std::string csrfToken;
        csrfToken.resize(sessionTokenSize, '0');
        for (size_t i = 0; i < csrfToken.size(); ++i)
        {
            csrfToken[i] = alphanum[dist(rd)];
        }

        std::string uniqueId;
        uniqueId.resize(10, '0');
        for (size_t i = 0; i < uniqueId.size(); ++i)
        {
            uniqueId[i] = alphanum[dist(rd)];
        }

        auto session = std::make_shared<UserSession>(UserSession{
            uniqueId, sessionToken, std::string(username), csrfToken,
            std::chrono::steady_clock::now(), persistence});
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
    };

    // Persistent data middleware needs to be able to serialize our authTokens
    // structure, which is private
    friend Middleware;

    static SessionStore& getInstance()
    {
        static SessionStore sessionStore;
        return sessionStore;
    }

    SessionStore(const SessionStore&) = delete;
    SessionStore& operator=(const SessionStore&) = delete;

  private:
    SessionStore() : timeoutInMinutes(60)
    {
    }

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

    std::chrono::time_point<std::chrono::steady_clock> lastTimeoutUpdate;
    std::unordered_map<std::string, std::shared_ptr<UserSession>,
                       std::hash<std::string>,
                       crow::utility::ConstantTimeCompare>
        authTokens;
    std::random_device rd;
    bool needWrite{false};
    std::chrono::minutes timeoutInMinutes;
    AuthConfigMethods authMethodsConfig;
};

} // namespace persistent_data
} // namespace crow

// to_json(...) definition for objects of UserSession type
namespace nlohmann
{
template <>
struct adl_serializer<std::shared_ptr<crow::persistent_data::UserSession>>
{
    static void
        to_json(nlohmann::json& j,
                const std::shared_ptr<crow::persistent_data::UserSession>& p)
    {
        if (p->persistence !=
            crow::persistent_data::PersistenceType::SINGLE_REQUEST)
        {
            j = nlohmann::json{{"unique_id", p->uniqueId},
                               {"session_token", p->sessionToken},
                               {"username", p->username},
                               {"csrf_token", p->csrfToken}};
        }
    }
};

template <> struct adl_serializer<crow::persistent_data::AuthConfigMethods>
{
    static void to_json(nlohmann::json& j,
                        const crow::persistent_data::AuthConfigMethods& c)
    {
        j = nlohmann::json{{"XToken", c.xtoken},
                           {"Cookie", c.cookie},
                           {"SessionToken", c.sessionToken},
                           {"BasicAuth", c.basic},
                           {"TLS", c.tls}};
    }
};
} // namespace nlohmann
