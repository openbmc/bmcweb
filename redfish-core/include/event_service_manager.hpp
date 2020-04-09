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
#include <http_client.hpp>
#include <memory>
#include <utils/json_utils.hpp>
#include <variant>

namespace redfish
{
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
        host(inHost),
        port(inPort), path(inPath), uriProto(inUriProto)
    {
        conn = std::make_shared<crow::HttpClient>(
            crow::connections::systemBus->get_io_context(), host, port);
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
        conn->doConnectAndSend(path, msg);
    }

  private:
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
        // TODO: Read the persistent data from store and populate.
        // Populating with default.
        enabled = true;
        retryAttempts = 3;
        retryTimeoutInterval = 30; // seconds
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
        // TODO: subscriptionsMap & configData need to be
        // written to Persist store.
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
            BMCWEB_LOG_ERROR << "Filed to generate random number";
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
};

} // namespace redfish
