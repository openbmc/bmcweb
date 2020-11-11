#pragma once

#include <app.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <http_request.hpp>
#include <http_response.hpp>
#include <nlohmann/json.hpp>
#include <pam_authenticate.hpp>
#include <sessions.hpp>

#include <filesystem>
#include <fstream>
#include <random>

namespace persistent_data
{

class ConfigFile
{
    uint64_t jsonRevision = 1;

  public:
    // todo(ed) should read this from a fixed location somewhere, not CWD
    static constexpr const char* filename = "bmcweb_persistent_data.json";

    ConfigFile()
    {
        readData();
    }

    ~ConfigFile()
    {
        // Make sure we aren't writing stale sessions
        persistent_data::SessionStore::getInstance().applySessionTimeouts();
        if (persistent_data::SessionStore::getInstance().needsWrite())
        {
            writeData();
        }
    }

    // TODO(ed) this should really use protobuf, or some other serialization
    // library, but adding another dependency is somewhat outside the scope of
    // this application for the moment
    void readData()
    {
        std::ifstream persistentFile(filename);
        uint64_t fileRevision = 0;
        if (persistentFile.is_open())
        {
            // call with exceptions disabled
            auto data = nlohmann::json::parse(persistentFile, nullptr, false);
            if (data.is_discarded())
            {
                BMCWEB_LOG_ERROR
                    << "Error parsing persistent data in json file.";
            }
            else
            {
                for (const auto& item : data.items())
                {
                    if (item.key() == "revision")
                    {
                        fileRevision = 0;

                        const uint64_t* uintPtr =
                            item.value().get_ptr<const uint64_t*>();
                        if (uintPtr == nullptr)
                        {
                            BMCWEB_LOG_ERROR << "Failed to read revision flag";
                        }
                        else
                        {
                            fileRevision = *uintPtr;
                        }
                    }
                    else if (item.key() == "system_uuid")
                    {
                        const std::string* jSystemUuid =
                            item.value().get_ptr<const std::string*>();
                        if (jSystemUuid != nullptr)
                        {
                            systemUuid = *jSystemUuid;
                        }
                    }
                    else if (item.key() == "auth_config")
                    {
                        SessionStore::getInstance()
                            .getAuthMethodsConfig()
                            .fromJson(item.value());
                    }
                    else if (item.key() == "sessions")
                    {
                        for (const auto& elem : item.value())
                        {
                            std::shared_ptr<UserSession> newSession =
                                UserSession::fromJson(elem);

                            if (newSession == nullptr)
                            {
                                BMCWEB_LOG_ERROR << "Problem reading session "
                                                    "from persistent store";
                                continue;
                            }

                            BMCWEB_LOG_DEBUG
                                << "Restored session: " << newSession->csrfToken
                                << " " << newSession->uniqueId << " "
                                << newSession->sessionToken;
                            SessionStore::getInstance().authTokens.emplace(
                                newSession->sessionToken, newSession);
                        }
                    }
                    else if (item.key() == "timeout")
                    {
                        const int64_t* jTimeout =
                            item.value().get_ptr<int64_t*>();
                        if (jTimeout == nullptr)
                        {
                            BMCWEB_LOG_DEBUG
                                << "Problem reading session timeout value";
                            continue;
                        }
                        std::chrono::seconds sessionTimeoutInseconds(*jTimeout);
                        BMCWEB_LOG_DEBUG << "Restored Session Timeout: "
                                         << sessionTimeoutInseconds.count();
                        SessionStore::getInstance().updateSessionTimeout(
                            sessionTimeoutInseconds);
                    }
                    else
                    {
                        // Do nothing in the case of extra fields.  We may have
                        // cases where fields are added in the future, and we
                        // want to at least attempt to gracefully support
                        // downgrades in that case, even if we don't officially
                        // support it
                    }
                }
            }
        }
        bool needWrite = false;

        if (systemUuid.empty())
        {
            systemUuid =
                boost::uuids::to_string(boost::uuids::random_generator()());
            needWrite = true;
        }
        if (fileRevision < jsonRevision)
        {
            needWrite = true;
        }
        // write revision changes or system uuid changes immediately
        if (needWrite)
        {
            writeData();
        }
    }

    void writeData()
    {
        std::ofstream persistentFile(filename);

        // set the permission of the file to 640
        std::filesystem::perms permission =
            std::filesystem::perms::owner_read |
            std::filesystem::perms::owner_write |
            std::filesystem::perms::group_read;
        std::filesystem::permissions(filename, permission);
        const auto& c = SessionStore::getInstance().getAuthMethodsConfig();
        nlohmann::json data{
            {"auth_config",
             {{"XToken", c.xtoken},
              {"Cookie", c.cookie},
              {"SessionToken", c.sessionToken},
              {"BasicAuth", c.basic},
              {"TLS", c.tls}}

            },
            {"system_uuid", systemUuid},
            {"revision", jsonRevision},
            {"timeout", SessionStore::getInstance().getTimeoutInSeconds()}};

        nlohmann::json& sessions = data["sessions"];
        sessions = nlohmann::json::array();
        for (const auto& p : SessionStore::getInstance().authTokens)
        {
            if (p.second->persistence !=
                persistent_data::PersistenceType::SINGLE_REQUEST)
            {
                sessions.push_back({
                    {"unique_id", p.second->uniqueId},
                    {"session_token", p.second->sessionToken},
                    {"username", p.second->username},
                    {"csrf_token", p.second->csrfToken},
                    {"client_ip", p.second->clientIp},
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
                    {"client_id", p.second->clientId},
#endif
                });
            }
        }
        persistentFile << data;
    }

    std::string systemUuid{""};
};

inline ConfigFile& getConfig()
{
    static ConfigFile f;
    return f;
}

} // namespace persistent_data
