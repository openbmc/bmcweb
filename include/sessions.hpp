#pragma once

#include <boost/container/flat_map.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <dbus_singleton.hpp>
#include <nlohmann/json.hpp>
#include <pam_authenticate.hpp>
#include <random>
#include <sdbusplus/bus/match.hpp>

#include "logging.h"
#include "utility.h"
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
#include <IBM/locks.hpp>
#endif

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

constexpr char const* userService = "xyz.openbmc_project.User.Manager";
constexpr char const* userObjPath = "/xyz/openbmc_project/user";
constexpr char const* userAttrIface = "xyz.openbmc_project.User.Attributes";
constexpr char const* dbusPropertiesIface = "org.freedesktop.DBus.Properties";

struct UserRoleMap
{
    using GetManagedPropertyType =
        boost::container::flat_map<std::string,
                                   std::variant<std::string, bool>>;

    using InterfacesPropertiesType =
        boost::container::flat_map<std::string, GetManagedPropertyType>;

    using GetManagedObjectsType = std::vector<
        std::pair<sdbusplus::message::object_path, InterfacesPropertiesType>>;

    static UserRoleMap& getInstance()
    {
        static UserRoleMap userRoleMap;
        return userRoleMap;
    }

    UserRoleMap(const UserRoleMap&) = delete;
    UserRoleMap& operator=(const UserRoleMap&) = delete;

    std::string getUserRole(std::string_view name)
    {
        auto it = roleMap.find(std::string(name));
        if (it == roleMap.end())
        {
            BMCWEB_LOG_ERROR << "User name " << name
                             << " is not found in the UserRoleMap.";
            return "";
        }
        return it->second;
    }

    std::string
        extractUserRole(const InterfacesPropertiesType& interfacesProperties)
    {
        auto iface = interfacesProperties.find(userAttrIface);
        if (iface == interfacesProperties.end())
        {
            return {};
        }

        auto& properties = iface->second;
        auto property = properties.find("UserPrivilege");
        if (property == properties.end())
        {
            return {};
        }

        const std::string* role = std::get_if<std::string>(&property->second);
        if (role == nullptr)
        {
            BMCWEB_LOG_ERROR << "UserPrivilege property value is null";
            return {};
        }

        return *role;
    }

  private:
    void userAdded(sdbusplus::message::message& m)
    {
        sdbusplus::message::object_path objPath;
        InterfacesPropertiesType interfacesProperties;

        try
        {
            m.read(objPath, interfacesProperties);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            BMCWEB_LOG_ERROR << "Failed to parse user add signal."
                             << "ERROR=" << e.what()
                             << "REPLY_SIG=" << m.get_signature();
            return;
        }
        BMCWEB_LOG_DEBUG << "obj path = " << objPath.str;

        std::size_t lastPos = objPath.str.rfind("/");
        if (lastPos == std::string::npos)
        {
            return;
        };

        std::string name = objPath.str.substr(lastPos + 1);
        std::string role = this->extractUserRole(interfacesProperties);

        // Insert the newly added user name and the role
        auto res = roleMap.emplace(name, role);
        if (res.second == false)
        {
            BMCWEB_LOG_ERROR << "Insertion of the user=\"" << name
                             << "\" in the roleMap failed.";
            return;
        }
    }

    void userRemoved(sdbusplus::message::message& m)
    {
        sdbusplus::message::object_path objPath;

        try
        {
            m.read(objPath);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            BMCWEB_LOG_ERROR << "Failed to parse user delete signal.";
            BMCWEB_LOG_ERROR << "ERROR=" << e.what()
                             << "REPLY_SIG=" << m.get_signature();
            return;
        }

        BMCWEB_LOG_DEBUG << "obj path = " << objPath.str;

        std::size_t lastPos = objPath.str.rfind("/");
        if (lastPos == std::string::npos)
        {
            return;
        };

        // User name must be atleast 1 char in length.
        if ((lastPos + 1) >= objPath.str.length())
        {
            return;
        }

        std::string name = objPath.str.substr(lastPos + 1);

        roleMap.erase(name);
    }

    void userPropertiesChanged(sdbusplus::message::message& m)
    {
        std::string interface;
        GetManagedPropertyType changedProperties;
        m.read(interface, changedProperties);
        const std::string path = m.get_path();

        BMCWEB_LOG_DEBUG << "Object Path = \"" << path << "\"";

        std::size_t lastPos = path.rfind("/");
        if (lastPos == std::string::npos)
        {
            return;
        };

        // User name must be at least 1 char in length.
        if ((lastPos + 1) == path.length())
        {
            return;
        }

        std::string user = path.substr(lastPos + 1);

        BMCWEB_LOG_DEBUG << "User Name = \"" << user << "\"";

        auto index = changedProperties.find("UserPrivilege");
        if (index == changedProperties.end())
        {
            return;
        }

        const std::string* role = std::get_if<std::string>(&index->second);
        if (role == nullptr)
        {
            return;
        }
        BMCWEB_LOG_DEBUG << "Role = \"" << *role << "\"";

        auto it = roleMap.find(user);
        if (it == roleMap.end())
        {
            BMCWEB_LOG_ERROR << "User Name = \"" << user
                             << "\" is not found. But, received "
                                "propertiesChanged signal";
            return;
        }
        it->second = *role;
    }

    UserRoleMap() :
        userAddedSignal(
            *crow::connections::systemBus,
            sdbusplus::bus::match::rules::interfacesAdded(userObjPath),
            [this](sdbusplus::message::message& m) {
                BMCWEB_LOG_DEBUG << "User Added";
                this->userAdded(m);
            }),
        userRemovedSignal(
            *crow::connections::systemBus,
            sdbusplus::bus::match::rules::interfacesRemoved(userObjPath),
            [this](sdbusplus::message::message& m) {
                BMCWEB_LOG_DEBUG << "User Removed";
                this->userRemoved(m);
            }),
        userPropertiesChangedSignal(
            *crow::connections::systemBus,
            sdbusplus::bus::match::rules::path_namespace(userObjPath) +
                sdbusplus::bus::match::rules::type::signal() +
                sdbusplus::bus::match::rules::member("PropertiesChanged") +
                sdbusplus::bus::match::rules::interface(dbusPropertiesIface) +
                sdbusplus::bus::match::rules::argN(0, userAttrIface),
            [this](sdbusplus::message::message& m) {
                BMCWEB_LOG_DEBUG << "Properties Changed";
                this->userPropertiesChanged(m);
            })
    {
        crow::connections::systemBus->async_method_call(
            [this](boost::system::error_code ec,
                   GetManagedObjectsType& managedObjects) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "User manager call failed, ignoring";
                    return;
                }

                for (auto& managedObj : managedObjects)
                {
                    std::size_t lastPos = managedObj.first.str.rfind("/");
                    if (lastPos == std::string::npos)
                    {
                        continue;
                    };
                    std::string name = managedObj.first.str.substr(lastPos + 1);
                    std::string role = extractUserRole(managedObj.second);
                    roleMap.emplace(name, role);
                }
            },
            userService, userObjPath, "org.freedesktop.DBus.ObjectManager",
            "GetManagedObjects");
    }

    boost::container::flat_map<std::string, std::string> roleMap;
    sdbusplus::bus::match_t userAddedSignal;
    sdbusplus::bus::match_t userRemovedSignal;
    sdbusplus::bus::match_t userPropertiesChangedSignal;
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
    bool tls = true;

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
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
        crow::ibm_mc_lock::lock::getInstance().releaselock(session->uniqueId);
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
        authMethodsConfig = config;
        needWrite = true;
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
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
                    crow::ibm_mc_lock::lock::getInstance().releaselock(
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
