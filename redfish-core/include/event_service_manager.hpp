/*
// Copyright (c) 2020 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once
#include "node.hpp"

#include <boost/container/flat_map.hpp>
#include <cstdlib>
#include <ctime>
#include <error_messages.hpp>
#include <fstream>
#include <http_client.hpp>
#include <memory>
#include <utils/json_utils.hpp>
#include <variant>

namespace redfish
{

static constexpr const char* eventServiceFile =
    "/var/lib/bmcweb/eventservice_config.json";

class Subscription
{
  public:
    std::string id;
    std::string destinationUrl;
    std::string protocol;
    std::string retryPolicy;
    std::string customText;
    std::string eventFormatType;
    std::string subscriptionType;
    std::vector<std::string> registryMsgIds;
    std::vector<std::string> registryPrefixes;
    std::vector<nlohmann::json> httpHeaders; // key-value pair

    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;
    Subscription(Subscription&&) = delete;
    Subscription& operator=(Subscription&&) = delete;

    Subscription(const std::string& inHost, const std::string& inPort,
                 const std::string& inPath, const std::string& inUriProto) :
        eventSeqNum(1),
        host(inHost), port(inPort), path(inPath), uriProto(inUriProto)
    {
        conn = std::make_shared<crow::HttpClient>(
            crow::connections::systemBus->get_io_context(), host, port, path);
    }
    ~Subscription()
    {
    }

    void sendEvent(const std::string& msg)
    {
        std::vector<std::pair<std::string, std::string>> reqHeaders;
        for (const auto& header : httpHeaders)
        {
            for (const auto& item : header.items())
            {
                std::string key = item.key();
                std::string val = item.value();
                reqHeaders.emplace_back(std::pair(key, val));
            }
        }
        conn->setHeaders(reqHeaders);
        conn->sendData(msg);
    }

    void sendTestEventLog()
    {
        nlohmann::json logEntryArray;
        logEntryArray.push_back({});
        nlohmann::json& logEntryJson = logEntryArray.back();

        logEntryJson = {{"EventId", "TestID"},
                        {"EventType", "Event"},
                        {"Severity", "OK"},
                        {"Message", "Generated test event"},
                        {"MessageId", "OpenBMC.0.1.TestEventLog"},
                        {"MessageArgs", nlohmann::json::array()},
                        {"EventTimestamp", crow::utility::dateTimeNow()},
                        {"Context", customText}};

        nlohmann::json msg = {{"@odata.type", "#Event.v1_4_0.Event"},
                              {"Id", std::to_string(eventSeqNum)},
                              {"Name", "Event Log"},
                              {"Events", logEntryArray}};

        this->sendEvent(msg.dump());
        this->eventSeqNum++;
    }

  private:
    uint64_t eventSeqNum;
    std::string host;
    std::string port;
    std::string path;
    std::string uriProto;
    std::shared_ptr<crow::HttpClient> conn;
};

class EventServiceManager
{
  private:
    EventServiceManager(const EventServiceManager&) = delete;
    EventServiceManager& operator=(const EventServiceManager&) = delete;
    EventServiceManager(EventServiceManager&&) = delete;
    EventServiceManager& operator=(EventServiceManager&&) = delete;

    EventServiceManager()
    {
        // Read the persistent data from store and populate
        // config & subscription info with default

        std::ifstream eventConfigFile(eventServiceFile);
        if (!eventConfigFile.good())
        {
            loadDefaultValues();
            BMCWEB_LOG_ERROR << "Cannot find eventService json file";
            return;
        }

        nlohmann::json configJson;
        nlohmann::json subscriptionJson;
        auto data = nlohmann::json::parse(eventConfigFile, nullptr, false);

        if (data.contains("Configuration"))
        {
            configJson = data["Configuration"];
        }
        else
        {
            BMCWEB_LOG_DEBUG << "Loading default configuration values";
            loadDefaultValues();
        }

        if (data.contains("Subscription"))
        {
            subscriptionJson = data["Subscription"];
        }

        try
        {
            enabled = configJson["ServiceEnabled"].get<bool>();
            retryAttempts = configJson["DeliveryRetryAttempts"].get<uint32_t>();
            retryTimeoutInterval =
                configJson["DeliveryRetryIntervalSeconds"].get<uint32_t>();

            for (const auto& subscriptionObj : subscriptionJson)
            {
                std::string host;
                std::string urlProto;
                std::string port;
                std::string path;
                bool status = validateAndSplitUrl(
                    subscriptionObj["Destination"], urlProto, host, port, path);

                if (!status)
                {
                    BMCWEB_LOG_ERROR
                        << "Failed to validate and split destination url";
                    continue;
                }
                else
                {
                    std::shared_ptr<Subscription> subValue =
                        std::make_shared<Subscription>(host, port, path,
                                                       urlProto);

                    subValue->destinationUrl =
                        subscriptionObj["Destination"].get<std::string>();
                    subValue->protocol =
                        subscriptionObj["Protocol"].get<std::string>();
                    subValue->retryPolicy =
                        subscriptionObj["DeliveryRetryPolicy"]
                            .get<std::string>();
                    subValue->customText =
                        subscriptionObj["Context"].get<std::string>();
                    subValue->eventFormatType =
                        subscriptionObj["EventFormatType"].get<std::string>();
                    subValue->subscriptionType =
                        subscriptionObj["SubscriptionType"].get<std::string>();
                    subValue->registryMsgIds =
                        subscriptionObj["MessageIds"]
                            .get<std::vector<std::string>>();
                    subValue->registryPrefixes =
                        subscriptionObj["RegistryPrefixes"]
                            .get<std::vector<std::string>>();
                    subValue->httpHeaders =
                        subscriptionObj["HttpHeaders"]
                            .get<std::vector<nlohmann::json>>();

                    std::string id = addSubscription(subValue);
                    if (id.empty())
                    {
                        BMCWEB_LOG_ERROR << "Failed to add subscription";
                    }
                }
            }
        }
        catch (const nlohmann::json::exception& e)
        {
            BMCWEB_LOG_ERROR << "Error parsing eventService json file";
            loadDefaultValues();
        }
        return;
    }

    boost::container::flat_map<std::string, std::shared_ptr<Subscription>>
        subscriptionsMap;

  public:
    bool enabled;
    uint32_t retryAttempts;
    uint32_t retryTimeoutInterval;

    static EventServiceManager& getInstance()
    {
        static EventServiceManager handler;
        return handler;
    }

    void updateSubscriptionData()
    {
        // Persist the config and subscription data.
        // subscriptionsMap & configData need to be
        // written to Persist store.

        nlohmann::json jsonData;

        nlohmann::json& configObj = jsonData["Configuration"];
        configObj["ServiceEnabled"] = enabled;
        configObj["DeliveryRetryAttempts"] = retryAttempts;
        configObj["DeliveryRetryIntervalSeconds"] = retryTimeoutInterval;

        nlohmann::json& subListArray = jsonData["Subscription"];
        subListArray = nlohmann::json::array();

        for (const auto& it : subscriptionsMap)
        {
            nlohmann::json entry;
            std::shared_ptr<Subscription> subValue = it.second;

            entry["Context"] = subValue->customText;
            entry["DeliveryRetryPolicy"] = subValue->retryPolicy;
            entry["Destination"] = subValue->destinationUrl;
            entry["EventFormatType"] = subValue->eventFormatType;
            entry["HttpHeaders"] = subValue->httpHeaders;
            entry["MessageIds"] = subValue->registryMsgIds;
            entry["Protocol"] = subValue->protocol;
            entry["RegistryPrefixes"] = subValue->registryPrefixes;
            entry["SubscriptionType"] = subValue->subscriptionType;

            subListArray.push_back(entry);
        }

        std::ofstream ofs;
        const std::string tmpFile{std::string(eventServiceFile) + "_tmp"};

        ofs.open(tmpFile, std::ios::out);

        const auto& writeData = jsonData.dump();

        ofs << writeData;
        BMCWEB_LOG_ERROR << "Data is successfully written to the file";

        // Closing the output stream
        ofs.close();

        if (std::rename(tmpFile.c_str(), eventServiceFile) != 0)
        {
            BMCWEB_LOG_ERROR << "Error in renaming temporary data file"
                             << tmpFile.c_str();
            return;
        }
        return;
    }

    std::shared_ptr<Subscription> getSubscription(const std::string& id)
    {
        auto obj = subscriptionsMap.find(id);
        if (obj == subscriptionsMap.end())
        {
            BMCWEB_LOG_ERROR << "No subscription exist with ID:" << id;
            return nullptr;
        }
        std::shared_ptr<Subscription> subValue = obj->second;
        return subValue;
    }

    std::string addSubscription(const std::shared_ptr<Subscription> subValue)
    {
        std::srand(static_cast<uint32_t>(std::time(0)));
        std::string id;

        int retry = 3;
        while (retry)
        {
            id = std::to_string(std::rand());
            auto inserted = subscriptionsMap.insert(std::pair(id, subValue));
            if (inserted.second)
            {
                break;
            }
            --retry;
        };

        if (retry <= 0)
        {
            BMCWEB_LOG_ERROR << "Failed to generate random number";
            return std::string("");
        }

        updateSubscriptionData();
        return id;
    }

    bool isSubscriptionExist(const std::string& id)
    {
        auto obj = subscriptionsMap.find(id);
        if (obj == subscriptionsMap.end())
        {
            return false;
        }
        return true;
    }

    void deleteSubscription(const std::string& id)
    {
        auto obj = subscriptionsMap.find(id);
        if (obj != subscriptionsMap.end())
        {
            subscriptionsMap.erase(obj);
            updateSubscriptionData();
        }
    }

    size_t getNumberOfSubscriptions()
    {
        return subscriptionsMap.size();
    }

    std::vector<std::string> getAllIDs()
    {
        std::vector<std::string> idList;
        for (const auto& it : subscriptionsMap)
        {
            idList.emplace_back(it.first);
        }
        return idList;
    }

    bool isDestinationExist(const std::string& destUrl)
    {
        for (const auto& it : subscriptionsMap)
        {
            std::shared_ptr<Subscription> entry = it.second;
            if (entry->destinationUrl == destUrl)
            {
                BMCWEB_LOG_ERROR << "Destination exist already" << destUrl;
                return true;
            }
        }
        return false;
    }

    void sendTestEventLog()
    {
        for (const auto& it : this->subscriptionsMap)
        {
            std::shared_ptr<Subscription> entry = it.second;
            entry->sendTestEventLog();
        }
    }

    void loadDefaultValues()
    {
        enabled = true;
        retryAttempts = 3;
        retryTimeoutInterval = 30; // seconds
    }

    bool validateAndSplitUrl(const std::string& destUrl, std::string& urlProto,
                             std::string& host, std::string& port,
                             std::string& path)
    {
        // Validate URL using regex expression
        // Format: <protocol>://<host>:<port>/<path>
        // protocol: http/https
        const std::regex urlRegex(
            "(http|https)://([^/\\x20\\x3f\\x23\\x3a]+):?([0-9]*)(/"
            "([^\\x20\\x23\\x3f]*\\x3f?([^\\x20\\x23\\x3f])*)?)");
        std::cmatch match;
        if (!std::regex_match(destUrl.c_str(), match, urlRegex))
        {
            BMCWEB_LOG_INFO << "Dest. url did not match ";
            return false;
        }

        urlProto = std::string(match[1].first, match[1].second);
        host = std::string(match[2].first, match[2].second);
        port = std::string(match[3].first, match[3].second);
        path = std::string(match[4].first, match[4].second);
        if (port.empty())
        {
            if (urlProto == "http")
            {
                port = "80";
            }
            else
            {
                port = "443";
            }
        }
        if (path.empty())
        {
            path = "/";
        }
        return true;
    }
};

} // namespace redfish
