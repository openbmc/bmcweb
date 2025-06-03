// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "event_service_store.hpp"
#include "logging.hpp"
#include "ossl_random.hpp"
#include "sessions.hpp"
// NOLINTNEXTLINE(misc-include-cleaner)
#include "utility.hpp"

#include <boost/beast/core/file_base.hpp>
#include <boost/beast/core/file_posix.hpp>
#include <boost/beast/http/fields.hpp>
#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <utility>

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

    ConfigFile(const ConfigFile&) = delete;
    ConfigFile(ConfigFile&&) = delete;
    ConfigFile& operator=(const ConfigFile&) = delete;
    ConfigFile& operator=(ConfigFile&&) = delete;

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
                BMCWEB_LOG_ERROR("Error parsing persistent data in json file.");
            }
            else
            {
                const nlohmann::json::object_t* obj =
                    data.get_ptr<nlohmann::json::object_t*>();
                if (obj == nullptr)
                {
                    return;
                }
                for (const auto& item : *obj)
                {
                    if (item.first == "revision")
                    {
                        fileRevision = 0;

                        const uint64_t* uintPtr =
                            item.second.get_ptr<const uint64_t*>();
                        if (uintPtr == nullptr)
                        {
                            BMCWEB_LOG_ERROR("Failed to read revision flag");
                        }
                        else
                        {
                            fileRevision = *uintPtr;
                        }
                    }
                    else if (item.first == "system_uuid")
                    {
                        const std::string* jSystemUuid =
                            item.second.get_ptr<const std::string*>();
                        if (jSystemUuid != nullptr)
                        {
                            systemUuid = *jSystemUuid;
                        }
                    }
                    else if (item.first == "auth_config")
                    {
                        const nlohmann::json::object_t* jObj =
                            item.second
                                .get_ptr<const nlohmann::json::object_t*>();
                        if (jObj == nullptr)
                        {
                            continue;
                        }
                        SessionStore::getInstance()
                            .getAuthMethodsConfig()
                            .fromJson(*jObj);
                    }
                    else if (item.first == "sessions")
                    {
                        for (const auto& elem : item.second)
                        {
                            const nlohmann::json::object_t* jObj =
                                elem.get_ptr<const nlohmann::json::object_t*>();
                            if (jObj == nullptr)
                            {
                                continue;
                            }
                            std::shared_ptr<UserSession> newSession =
                                UserSession::fromJson(*jObj);

                            if (newSession == nullptr)
                            {
                                BMCWEB_LOG_ERROR("Problem reading session "
                                                 "from persistent store");
                                continue;
                            }

                            BMCWEB_LOG_DEBUG("Restored session: {} {} {}",
                                             newSession->csrfToken,
                                             newSession->uniqueId,
                                             newSession->sessionToken);
                            SessionStore::getInstance().authTokens.emplace(
                                newSession->sessionToken, newSession);
                        }
                    }
                    else if (item.first == "timeout")
                    {
                        const int64_t* jTimeout =
                            item.second.get_ptr<const int64_t*>();
                        if (jTimeout == nullptr)
                        {
                            BMCWEB_LOG_DEBUG(
                                "Problem reading session timeout value");
                            continue;
                        }
                        std::chrono::seconds sessionTimeoutInseconds(*jTimeout);
                        BMCWEB_LOG_DEBUG("Restored Session Timeout: {}",
                                         sessionTimeoutInseconds.count());
                        SessionStore::getInstance().updateSessionTimeout(
                            sessionTimeoutInseconds);
                    }
                    else if (item.first == "eventservice_config")
                    {
                        const nlohmann::json::object_t* esobj =
                            item.second
                                .get_ptr<const nlohmann::json::object_t*>();
                        if (esobj == nullptr)
                        {
                            BMCWEB_LOG_DEBUG(
                                "Problem reading EventService value");
                            continue;
                        }

                        EventServiceStore::getInstance()
                            .getEventServiceConfig()
                            .fromJson(*esobj);
                    }
                    else if (item.first == "subscriptions")
                    {
                        for (const auto& elem : item.second)
                        {
                            const nlohmann::json::object_t* subobj =
                                elem.get_ptr<const nlohmann::json::object_t*>();
                            if (subobj == nullptr)
                            {
                                continue;
                            }

                            std::optional<UserSubscription> newSub =
                                UserSubscription::fromJson(*subobj);

                            if (!newSub)
                            {
                                BMCWEB_LOG_ERROR(
                                    "Problem reading subscription from persistent store");
                                continue;
                            }

                            std::string id = newSub->id;
                            BMCWEB_LOG_DEBUG("Restored subscription: {} {}", id,
                                             newSub->customText);

                            EventServiceStore::getInstance()
                                .subscriptionsConfigMap.emplace(
                                    id, std::make_shared<UserSubscription>(
                                            std::move(*newSub)));
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
            systemUuid = bmcweb::getRandomUUID();
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
        std::filesystem::path path(filename);
        path = path.parent_path();
        if (!path.empty())
        {
            std::error_code ecDir;
            std::filesystem::create_directories(path, ecDir);
            if (ecDir)
            {
                BMCWEB_LOG_CRITICAL("Can't create persistent folders {}",
                                    ecDir.message());
                return;
            }
        }
        boost::beast::file_posix persistentFile;
        boost::system::error_code ec;
        persistentFile.open(filename, boost::beast::file_mode::write, ec);
        if (ec)
        {
            BMCWEB_LOG_CRITICAL("Unable to store persistent data to file {}",
                                ec.message());
            return;
        }

        // set the permission of the file to 640
        std::filesystem::perms permission =
            std::filesystem::perms::owner_read |
            std::filesystem::perms::owner_write |
            std::filesystem::perms::group_read;
        std::filesystem::permissions(filename, permission, ec);
        if (ec)
        {
            BMCWEB_LOG_CRITICAL("Failed to set filesystem permissions {}",
                                ec.message());
            return;
        }
        const AuthConfigMethods& c =
            SessionStore::getInstance().getAuthMethodsConfig();
        const auto& eventServiceConfig =
            EventServiceStore::getInstance().getEventServiceConfig();
        nlohmann::json::object_t data;
        nlohmann::json& authConfig = data["auth_config"];

        authConfig["XToken"] = c.xtoken;
        authConfig["Cookie"] = c.cookie;
        authConfig["SessionToken"] = c.sessionToken;
        authConfig["BasicAuth"] = c.basic;
        authConfig["TLS"] = c.tls;
        authConfig["TLSStrict"] = c.tlsStrict;
        authConfig["MTLSCommonNameParseMode"] =
            static_cast<int>(c.mTLSCommonNameParsingMode);

        nlohmann::json& eventserviceConfig = data["eventservice_config"];
        eventserviceConfig["ServiceEnabled"] = eventServiceConfig.enabled;
        eventserviceConfig["DeliveryRetryAttempts"] =
            eventServiceConfig.retryAttempts;
        eventserviceConfig["DeliveryRetryIntervalSeconds"] =
            eventServiceConfig.retryTimeoutInterval;

        data["system_uuid"] = systemUuid;
        data["revision"] = jsonRevision;
        data["timeout"] = SessionStore::getInstance().getTimeoutInSeconds();

        nlohmann::json& sessions = data["sessions"];
        sessions = nlohmann::json::array();
        for (const auto& p : SessionStore::getInstance().authTokens)
        {
            if (p.second->sessionType != persistent_data::SessionType::Basic &&
                p.second->sessionType !=
                    persistent_data::SessionType::MutualTLS)
            {
                nlohmann::json::object_t session;
                session["unique_id"] = p.second->uniqueId;
                session["session_token"] = p.second->sessionToken;
                session["username"] = p.second->username;
                session["csrf_token"] = p.second->csrfToken;
                session["client_ip"] = p.second->clientIp;
                const std::optional<std::string>& clientId = p.second->clientId;
                if (clientId)
                {
                    session["client_id"] = *clientId;
                }
                sessions.emplace_back(std::move(session));
            }
        }
        nlohmann::json& subscriptions = data["subscriptions"];
        subscriptions = nlohmann::json::array();
        for (const auto& it :
             EventServiceStore::getInstance().subscriptionsConfigMap)
        {
            if (it.second == nullptr)
            {
                continue;
            }
            const UserSubscription& subValue = *it.second;
            if (subValue.subscriptionType == "SSE")
            {
                BMCWEB_LOG_DEBUG("The subscription type is SSE, so skipping.");
                continue;
            }
            nlohmann::json::object_t headers;
            for (const boost::beast::http::fields::value_type& header :
                 subValue.httpHeaders)
            {
                // Note, these are technically copies because nlohmann doesn't
                // support key lookup by std::string_view.  At least the
                // following code can use move
                // https://github.com/nlohmann/json/issues/1529
                std::string name(header.name_string());
                headers[std::move(name)] = header.value();
            }

            nlohmann::json::object_t subscription;

            subscription["Id"] = subValue.id;
            subscription["Context"] = subValue.customText;
            subscription["DeliveryRetryPolicy"] = subValue.retryPolicy;
            subscription["SendHeartbeat"] = subValue.sendHeartbeat;
            subscription["HeartbeatIntervalMinutes"] =
                subValue.hbIntervalMinutes;
            subscription["Destination"] = subValue.destinationUrl;
            subscription["EventFormatType"] = subValue.eventFormatType;
            subscription["HttpHeaders"] = std::move(headers);
            subscription["MessageIds"] = subValue.registryMsgIds;
            subscription["Protocol"] = subValue.protocol;
            subscription["RegistryPrefixes"] = subValue.registryPrefixes;
            subscription["OriginResources"] = subValue.originResources;
            subscription["ResourceTypes"] = subValue.resourceTypes;
            subscription["SubscriptionType"] = subValue.subscriptionType;
            subscription["MetricReportDefinitions"] =
                subValue.metricReportDefinitions;
            subscription["VerifyCertificate"] = subValue.verifyCertificate;

            subscriptions.emplace_back(std::move(subscription));
        }
        std::string out = nlohmann::json(data).dump(
            -1, ' ', true, nlohmann::json::error_handler_t::replace);
        persistentFile.write(out.data(), out.size(), ec);
        if (ec)
        {
            BMCWEB_LOG_ERROR("Failed to write file {}", ec.message());
        }
    }

    std::string systemUuid;
};

inline ConfigFile& getConfig()
{
    static ConfigFile f;
    return f;
}

} // namespace persistent_data
