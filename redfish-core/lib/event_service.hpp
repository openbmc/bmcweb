/*
// Copyright (c) 2019 Intel Corporation
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
#include <error_messages.hpp>
#include <utils/json_utils.hpp>
#include <variant>

namespace redfish
{

static constexpr const uint32_t minDeliveryRetryAttempts = 1;
static constexpr const uint32_t maxDeliveryRetryAttempts = 3;
static constexpr const uint32_t minDeliveryRetryInterval = 30;
static constexpr const uint32_t maxDeliveryRetryInterval = 180;

struct EventSrvConfig
{
    bool enabled;
    uint32_t retryAttempts;
    uint32_t retryTimeoutInterval;
    std::vector<std::string> supportedEvtTypes;
    std::vector<std::string> supportedRegPrefixes;
};

struct EventSrvSubscription
{
    std::string destinationUri;
    std::string customText;
    std::string subscriptionType;
    std::string protocol;
    std::vector<std::string> httpHeaders; // key-value pair
    std::vector<std::string> registryMsgIds;
    std::vector<std::string> registryPrefixes;
    std::vector<std::string> eventFormatTypes;
};

EventSrvConfig configData;
std::map<std::string, EventSrvSubscription> subscriptionsList;

inline void initEventSrvStore()
{
    // TODO: Read the persistent data from store and populate
    // Poulating with default.
    configData.enabled = true;
    configData.retryAttempts = 3;
    configData.retryTimeoutInterval = 30;           // seconds
    configData.supportedEvtTypes.assign({"Event"}); // TODO: MetricReport
    configData.supportedRegPrefixes.assign({"Base", "OpenBMC"});
}

static std::string getEvtSubscriptionId()
{
    std::srand(std::time(nullptr));
    return std::to_string(std::rand());
}

class EventService : public Node
{
  public:
    EventService(CrowApp& app) : Node(app, "/redfish/v1/EventService/")
    {
        initEventSrvStore();

        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        res.jsonValue = {
            {"@odata.type", "#EventService.v1_3_0.EventService"},
            {"@odata.context",
             "/redfish/v1/$metadata#EventService.EventService"},
            {"Id", "EventService"},
            {"Name", "Event Service"},
            {"ServerSentEventUri",
             "/redfish/v1/EventService/Subscriptions/SSE"},
            {"Subscriptions",
             {{"@odata.id", "/redfish/v1/EventService/Subscriptions"}}},
            {"Actions",
             {{"#EventService.SubmitTestEvent",
               {{"target", "/redfish/v1/EventService/Actions/"
                           "EventService.SubmitTestEvent"}}}}},
            {"@odata.id", "/redfish/v1/EventService"}};

        asyncResp->res.jsonValue["ServiceEnabled"] = configData.enabled;
        asyncResp->res.jsonValue["DeliveryRetryAttempts"] =
            configData.retryAttempts;
        asyncResp->res.jsonValue["DeliveryRetryIntervalSeconds"] =
            configData.retryTimeoutInterval;
        asyncResp->res.jsonValue["EventFormatTypes"] =
            configData.supportedEvtTypes;
        asyncResp->res.jsonValue["RegistryPrefixes"] =
            configData.supportedRegPrefixes;

        res.end();
    }

    void doPatch(crow::Response& res, const crow::Request& req,
                 const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);

        std::optional<bool> serviceEnabled;
        std::optional<uint32_t> retryAttemps;
        std::optional<uint32_t> retryInterval;

        if (!json_util::readJson(req, res, "ServiceEnabled", serviceEnabled,
                                 "DeliveryRetryAttempts", retryAttemps,
                                 "DeliveryRetryIntervalSeconds", retryInterval))
        {
            return;
        }

        if (serviceEnabled)
        {
            configData.enabled = *serviceEnabled;
        }

        if (retryAttemps)
        {
            if ((*retryAttemps < minDeliveryRetryAttempts) ||
                (*retryAttemps > maxDeliveryRetryAttempts))
            {
                messages::invalidObject(asyncResp->res,
                                        "DeliveryRetryAttempts");
            }
            else
            {
                configData.retryAttempts = *retryAttemps;
            }
        }

        if (retryInterval)
        {
            if ((*retryInterval < minDeliveryRetryInterval) ||
                (*retryInterval > maxDeliveryRetryInterval))
            {
                messages::invalidObject(asyncResp->res,
                                        "DeliveryRetryIntervalSeconds");
            }
            else
            {
                configData.retryTimeoutInterval = *retryInterval;
            }
        }
    }
};

class SubscriptionCollection : public Node
{
  public:
    SubscriptionCollection(CrowApp& app) :
        Node(app, "/redfish/v1/EventService/Subscriptions/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);

        res.jsonValue = {
            {"@odata.type",
             "#EventDestinationCollection.v1_0_0.EventDestinationCollection"},
            {"@odata.context",
             "/redfish/v1/"
             "$metadata#EventDestinationCollection.EventDestinationCollection"},
            {"@odata.id", "/redfish/v1/EventService/Subscriptions"}};

        nlohmann::json& memberArray = asyncResp->res.jsonValue["Members"];
        memberArray = nlohmann::json::array();
        asyncResp->res.jsonValue["Members@odata.count"] =
            subscriptionsList.size();

        for (auto& it : subscriptionsList)
        {
            memberArray.push_back(
                {{"@odata.id",
                  "/redfish/v1/EventService/Subscriptions/" + it.first}});
        }

        res.end();
    }

    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        std::string destUri;
        std::string protocol;
        std::optional<std::string> context;
        std::optional<std::vector<std::string>> headers;
        std::optional<std::vector<std::string>> eventTypes;
        std::optional<std::vector<std::string>> regPrefixes;
        std::optional<std::vector<std::string>> msgIds;

        if (!json_util::readJson(req, res, "Destination", destUri, "Context",
                                 context, "Protocol", protocol,
                                 "EventFormatTypes", eventTypes, "HttpHeaders",
                                 headers, "RegistryPrefixes", regPrefixes,
                                 "MessageIds", msgIds))
        {
            return;
        }

        EventSrvSubscription subValue;

        // TODO: Validate URI
        subValue.destinationUri = destUri;
        subValue.subscriptionType = "Event";
        if (protocol != "Redfish")
        {
            messages::invalidObject(asyncResp->res, "Protocol");
            return;
        }
        subValue.protocol = protocol;

        if (eventTypes)
        {
            for (auto& it : *eventTypes)
            {
                if (std::find(configData.supportedEvtTypes.begin(),
                              configData.supportedEvtTypes.end(),
                              it) == configData.supportedEvtTypes.end())
                {
                    messages::propertyValueNotInList(asyncResp->res, it,
                                                     "EventFormatTypes");
                    return;
                }
            }
            subValue.eventFormatTypes = *eventTypes;
        }
        else
        {
            // If not specified, use default "Event"
            subValue.eventFormatTypes.assign({"Event"});
        }

        if (context)
        {
            subValue.customText = *context;
        }

        if (headers)
        {
            subValue.httpHeaders = *headers;
        }

        if (regPrefixes)
        {
            for (auto& it : *regPrefixes)
            {
                if (std::find(configData.supportedRegPrefixes.begin(),
                              configData.supportedRegPrefixes.end(),
                              it) == configData.supportedRegPrefixes.end())
                {
                    messages::propertyValueNotInList(asyncResp->res, it,
                                                     "RegistryPrefixes");
                    return;
                }
            }
            subValue.registryPrefixes = *regPrefixes;
        }

        if (msgIds)
        {
            // Do we need to loop-up MessageRegistry and validate
            // data for authenticity??? Not mandate, i believe.
            subValue.registryMsgIds = *msgIds;
        }

        std::string id = getEvtSubscriptionId();
        subscriptionsList.insert(std::pair(id, subValue));

        messages::created(asyncResp->res);
        asyncResp->res.addHeader(
            "Location", "/redfish/v1/EventService/Subscriptions/" + id);
    }
};

class ManagerSubscription : public Node
{
  public:
    ManagerSubscription(CrowApp& app) :
        Node(app, "/redfish/v1/EventService/Subscriptions/<str>/",
             std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& id = params[0];
        auto obj = subscriptionsList.find(id);
        if (obj == subscriptionsList.end())
        {
            res.result(boost::beast::http::status::not_found);
            res.end();
            return;
        }

        EventSrvSubscription& subValue = obj->second;

        res.jsonValue = {
            {"@odata.type", "#EventDestination.v1_5_0.EventDestination"},
            {"@odata.context",
             "/redfish/v1/$metadata#EventDestination.EventDestination"},
            {"Id", "1"},
            {"Name", "EventSubscription 1"},
            {"Protocol", "Redfish"},
            {"@odata.id", "/redfish/v1/EventService/Subscriptions/1"}};

        asyncResp->res.jsonValue["Destination"] = subValue.destinationUri;
        asyncResp->res.jsonValue["Context"] = subValue.customText;
        asyncResp->res.jsonValue["SubscriptionType"] =
            subValue.subscriptionType;
        asyncResp->res.jsonValue["HttpHeaders"] = subValue.httpHeaders;
        asyncResp->res.jsonValue["EventFormatTypes"] =
            subValue.eventFormatTypes;
        asyncResp->res.jsonValue["RegistryPrefixes"] =
            subValue.registryPrefixes;
        asyncResp->res.jsonValue["MessageIds"] = subValue.registryMsgIds;

        res.end();
    }

    void doPatch(crow::Response& res, const crow::Request& req,
                 const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& id = params[0];
        auto obj = subscriptionsList.find(id);
        if (obj == subscriptionsList.end())
        {
            res.result(boost::beast::http::status::not_found);
            res.end();
            return;
        }

        std::optional<std::string> destUri;
        std::optional<std::string> context;
        std::optional<std::vector<std::string>> headers;
        std::optional<std::vector<std::string>> eventTypes;
        std::optional<std::vector<std::string>> regPrefixes;
        std::optional<std::vector<std::string>> msgIds;

        if (!json_util::readJson(
                req, res, "Destination", destUri, "Context", context,
                "HttpHeaders", headers, "EventFormatTypes", eventTypes,
                "RegistryPrefixes", regPrefixes, "MessageIds", msgIds))
        {
            return;
        }

        EventSrvSubscription& subValue = obj->second;

        if (destUri)
        {
            // TODO: Validate URI
            subValue.destinationUri = *destUri;
        }

        if (context)
        {
            subValue.customText = *context;
        }

        if (headers)
        {
            subValue.httpHeaders = *headers;
        }

        if (eventTypes)
        {
            for (auto& it : *eventTypes)
            {
                if (std::find(configData.supportedEvtTypes.begin(),
                              configData.supportedEvtTypes.end(),
                              it) == configData.supportedEvtTypes.end())
                {
                    messages::propertyValueNotInList(asyncResp->res, it,
                                                     "EventFormatTypes");
                    return;
                }
            }
            subValue.eventFormatTypes = *eventTypes;
        }

        if (regPrefixes)
        {
            for (auto& it : *regPrefixes)
            {
                if (std::find(configData.supportedRegPrefixes.begin(),
                              configData.supportedRegPrefixes.end(),
                              it) == configData.supportedRegPrefixes.end())
                {
                    messages::propertyValueNotInList(asyncResp->res, it,
                                                     "RegistryPrefixes");
                    return;
                }
            }
            subValue.registryPrefixes = *regPrefixes;
        }

        if (msgIds)
        {
            subValue.registryMsgIds = *msgIds;
        }
    }

    void doDelete(crow::Response& res, const crow::Request& req,
                  const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);

        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& id = params[0];
        auto obj = subscriptionsList.find(id);
        if (obj == subscriptionsList.end())
        {
            res.result(boost::beast::http::status::not_found);
            res.end();
            return;
        }

        subscriptionsList.erase(obj);
    }
};

} // namespace redfish
