#include "persistent_data.hpp"
#include <filesystem>
#include <nlohmann/json.hpp>
#include <fstream>
#include <random>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <event_service_store.hpp>

#include <boost/beast/http/fields.hpp>

#include "redfish_sessions.hpp"

namespace persistent_data
{

void ConfigFile::readData()
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
                else if (item.key() == "eventservice_config")
                {
                    EventServiceStore::getInstance()
                        .getEventServiceConfig()
                        .fromJson(item.value());
                }
                else if (item.key() == "subscriptions")
                {
                    for (const auto& elem : item.value())
                    {
                        std::shared_ptr<UserSubscription> newSubscription =
                            UserSubscription::fromJson(elem);

                        if (newSubscription == nullptr)
                        {
                            BMCWEB_LOG_ERROR
                                << "Problem reading subscription "
                                    "from persistent store";
                            continue;
                        }

                        BMCWEB_LOG_DEBUG << "Restored subscription: "
                                          << newSubscription->id << " "
                                          << newSubscription->customText;
                        EventServiceStore::getInstance()
                            .subscriptionsConfigMap.emplace(
                                newSubscription->id, newSubscription);
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

void ConfigFile::writeData()
{
    std::ofstream persistentFile(filename);

    // set the permission of the file to 640
    std::filesystem::perms permission =
        std::filesystem::perms::owner_read |
        std::filesystem::perms::owner_write |
        std::filesystem::perms::group_read;
    std::filesystem::permissions(filename, permission);
    const auto& c = SessionStore::getInstance().getAuthMethodsConfig();
    const auto& eventServiceConfig =
        EventServiceStore::getInstance().getEventServiceConfig();
    nlohmann::json data{
        {"auth_config",
          {{"XToken", c.xtoken},
          {"Cookie", c.cookie},
          {"SessionToken", c.sessionToken},
          {"BasicAuth", c.basic},
          {"TLS", c.tls}}

        },
        {"eventservice_config",
          {{"ServiceEnabled", eventServiceConfig.enabled},
          {"DeliveryRetryAttempts", eventServiceConfig.retryAttempts},
          {"DeliveryRetryIntervalSeconds",
            eventServiceConfig.retryTimeoutInterval}}

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
    nlohmann::json& subscriptions = data["subscriptions"];
    subscriptions = nlohmann::json::array();
    for (const auto& it :
          EventServiceStore::getInstance().subscriptionsConfigMap)
    {
        std::shared_ptr<UserSubscription> subValue = it.second;
        if (subValue->subscriptionType == "SSE")
        {
            BMCWEB_LOG_DEBUG
                << "The subscription type is SSE, so skipping.";
            continue;
        }
        nlohmann::json::object_t headers;
        for (const boost::beast::http::fields::value_type& header :
              subValue->httpHeaders)
        {
            // Note, these are technically copies because nlohmann doesn't
            // support key lookup by std::string_view.  At least the
            // following code can use move
            // https://github.com/nlohmann/json/issues/1529
            std::string name(header.name_string());
            headers[std::move(name)] = header.value();
        }

        subscriptions.push_back({
            {"Id", subValue->id},
            {"Context", subValue->customText},
            {"DeliveryRetryPolicy", subValue->retryPolicy},
            {"Destination", subValue->destinationUrl},
            {"EventFormatType", subValue->eventFormatType},
            {"HttpHeaders", std::move(headers)},
            {"MessageIds", subValue->registryMsgIds},
            {"Protocol", subValue->protocol},
            {"RegistryPrefixes", subValue->registryPrefixes},
            {"ResourceTypes", subValue->resourceTypes},
            {"SubscriptionType", subValue->subscriptionType},
            {"MetricReportDefinitions", subValue->metricReportDefinitions},
        });
    }
    persistentFile << data;
}

}