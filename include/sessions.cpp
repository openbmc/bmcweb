#include "sessions.hpp"

namespace persistent_data
{

std::shared_ptr<UserSession> UserSession::fromJson(const nlohmann::json& j)
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
            continue;
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
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
        else if (element.key() == "client_id")
        {
            userSession->clientId = *thisValue;
        }
#endif
        else if (element.key() == "client_ip")
        {
            userSession->clientIp = *thisValue;
        }

        else
        {
            BMCWEB_LOG_ERROR
                << "Got unexpected property reading persistent file: "
                << element.key();
            continue;
        }
    }
    // If any of these fields are missing, we can't restore the session, as
    // we don't have enough information.  These 4 fields have been present
    // in every version of this file in bmcwebs history, so any file, even
    // on upgrade, should have these present
    if (userSession->uniqueId.empty() || userSession->username.empty() ||
        userSession->sessionToken.empty() || userSession->csrfToken.empty())
    {
        BMCWEB_LOG_DEBUG << "Session missing required security "
                            "information, refusing to restore";
        return nullptr;
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

void AuthConfigMethods::fromJson(const nlohmann::json& j)
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

std::shared_ptr<UserSession> SessionStore::generateUserSession(
    const std::string_view username, const std::string_view clientIp,
    const std::string_view clientId,
    PersistenceType persistence,
    bool isConfigureSelfOnly)
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

    bmcweb::OpenSSLGenerator gen;

    for (char& sessionChar : sessionToken)
    {
        sessionChar = alphanum[dist(gen)];
        if (gen.error())
        {
            return nullptr;
        }
    }
    // Only need csrf tokens for cookie based auth, token doesn't matter
    std::string csrfToken;
    csrfToken.resize(sessionTokenSize, '0');
    for (char& csrfChar : csrfToken)
    {
        csrfChar = alphanum[dist(gen)];
        if (gen.error())
        {
            return nullptr;
        }
    }

    std::string uniqueId;
    uniqueId.resize(10, '0');
    for (char& uidChar : uniqueId)
    {
        uidChar = alphanum[dist(gen)];
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
    SessionStore::loginSessionByToken(const std::string_view token)
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

std::shared_ptr<UserSession> SessionStore::getSessionByUid(const std::string_view uid)
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

void SessionStore::removeSession(const std::shared_ptr<UserSession>& session)
{
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
    crow::ibm_mc_lock::Lock::getInstance().releaseLock(session->uniqueId);
#endif
    authTokens.erase(session->sessionToken);
    needWrite = true;
}

std::vector<const std::string*> SessionStore::getUniqueIds(
        bool getAll,
        const PersistenceType& type)
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

void SessionStore::updateAuthMethodsConfig(const AuthConfigMethods& config)
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

AuthConfigMethods& SessionStore::getAuthMethodsConfig()
{
    return authMethodsConfig;
}

bool SessionStore::needsWrite()
{
    return needWrite;
}
int64_t SessionStore::getTimeoutInSeconds() const
{
    return std::chrono::seconds(timeoutInSeconds).count();
}

void SessionStore::updateSessionTimeout(std::chrono::seconds newTimeoutInSeconds)
{
    timeoutInSeconds = newTimeoutInSeconds;
    needWrite = true;
}

SessionStore& SessionStore::getInstance()
{
    static SessionStore sessionStore;
    return sessionStore;
}

void SessionStore::applySessionTimeouts()
{
    auto timeNow = std::chrono::steady_clock::now();
    if (timeNow - lastTimeoutUpdate > std::chrono::seconds(1))
    {
        lastTimeoutUpdate = timeNow;
        auto authTokensIt = authTokens.begin();
        while (authTokensIt != authTokens.end())
        {
            if (timeNow - authTokensIt->second->lastUpdated >=
                timeoutInSeconds)
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

}