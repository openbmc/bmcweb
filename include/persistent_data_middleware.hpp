#pragma once

#include <crow/app.h>
#include <crow/http_request.h>
#include <crow/http_response.h>

#include <boost/container/flat_map.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <nlohmann/json.hpp>
#include <pam_authenticate.hpp>
#include <random>
#include <sessions.hpp>
#include <webassets.hpp>

namespace crow
{

namespace persistent_data
{

class Middleware
{
    // todo(ed) should read this from a fixed location somewhere, not CWD
    static constexpr const char* filename = "bmcweb_persistent_data.json";
    int jsonRevision = 1;

  public:
    struct Context
    {
    };

    Middleware()
    {
        readData();
    }

    ~Middleware()
    {
        if (persistent_data::SessionStore::getInstance().needsWrite())
        {
            writeData();
        }
    }

    void beforeHandle(crow::Request& req, Response& res, Context& ctx)
    {
    }

    void afterHandle(Request& req, Response& res, Context& ctx)
    {
    }

    // TODO(ed) this should really use protobuf, or some other serialization
    // library, but adding another dependency is somewhat outside the scope of
    // this application for the moment
    void readData()
    {
        std::ifstream persistentFile(filename);
        int fileRevision = 0;
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
        nlohmann::json data{
            {"sessions", SessionStore::getInstance().authTokens},
            {"system_uuid", systemUuid},
            {"revision", jsonRevision}};
        persistentFile << data;
    }

    std::string systemUuid{""};
};

} // namespace persistent_data
} // namespace crow
