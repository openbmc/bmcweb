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
#include "event_service_mgr.hpp"

namespace redfish
{

static constexpr const std::array<const char*, 2> supportedEvtTypes = {
    "Event", "MetricReport"};
static constexpr const std::array<const char*, 2> supportedRegPrefixes = {
    "Base", "OpenBMC"};
static constexpr const std::array<const char*, 3> supportedRetryPolicies = {
    "TerminateAfterRetries", "SuspendRetries", "RetryForever"};

EventSrvManager evtSrvManager;

class EventService : public Node
{
  public:
    EventService(CrowApp& app) : Node(app, "/redfish/v1/EventService/")
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

        asyncResp->res.jsonValue["ServiceEnabled"] = evtSrvManager.enabled;
        asyncResp->res.jsonValue["DeliveryRetryAttempts"] =
            evtSrvManager.retryAttempts;
        asyncResp->res.jsonValue["DeliveryRetryIntervalSeconds"] =
            evtSrvManager.retryTimeoutInterval;
        asyncResp->res.jsonValue["EventFormatTypes"] = supportedEvtTypes;
        asyncResp->res.jsonValue["RegistryPrefixes"] = supportedRegPrefixes;
        asyncResp->res.jsonValue["ResourceTypes"] = nlohmann::json::array();

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
            evtSrvManager.enabled = *serviceEnabled;
        }

        if (retryAttemps)
        {
            // Supported range [1-3]
            if ((*retryAttemps < 1) || (*retryAttemps > 3))
            {
                messages::queryParameterOutOfRange(
                    asyncResp->res, std::to_string(*retryAttemps),
                    "DeliveryRetryAttempts", "[1-3]");
            }
            else
            {
                evtSrvManager.retryAttempts = *retryAttemps;
            }
        }

        if (retryInterval)
        {
            // Supported range [30 - 180]
            if ((*retryInterval < 30) || (*retryInterval > 180))
            {
                messages::queryParameterOutOfRange(
                    asyncResp->res, std::to_string(*retryInterval),
                    "DeliveryRetryIntervalSeconds", "[30-180]");
            }
            else
            {
                evtSrvManager.retryTimeoutInterval = *retryInterval;
            }
        }
    }
};

class EventDestinationCollection : public Node
{
  public:
    EventDestinationCollection(CrowApp& app) :
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

        std::vector<std::string> subscripIds = evtSrvManager.getAllIDs();
        memberArray = nlohmann::json::array();
        asyncResp->res.jsonValue["Members@odata.count"] = subscripIds.size();

        for (const std::string& id : subscripIds)
        {
            memberArray.push_back(
                {{"@odata.id",
                  "/redfish/v1/EventService/Subscriptions/" + id}});
        }
    }

    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        std::string destUrl;
        std::string protocol;
        std::optional<std::string> context;
        std::optional<std::string> eventType;
        std::optional<std::string> retryPolicy;
        std::optional<std::vector<std::string>> msgIds;
        std::optional<std::vector<std::string>> regPrefixes;
        std::optional<std::vector<nlohmann::json>> headers;
        std::optional<std::vector<nlohmann::json>> originResources;
        std::optional<std::vector<nlohmann::json>> metricReportDefinitions;

        if (!json_util::readJson(
                req, res, "Destination", destUrl, "Context", context,
                "Protocol", protocol, "EventFormatType", eventType,
                "HttpHeaders", headers, "RegistryPrefixes", regPrefixes,
                "MessageIds", msgIds, "DeliveryRetryPolicy", retryPolicy,
                "OriginResources", originResources, "MetricReportDefinitions",
                metricReportDefinitions))
        {
            return;
        }

        const std::regex urlRegex("(http|https)://([^/ :]+):?([^/ ]*)(/?[^ "
                                  "#?]*)\\x3f?([^ #]*)#?([^ ]*)");
        std::cmatch matchTok;
        if (!std::regex_match(destUrl.c_str(), matchTok, urlRegex))
        {
            messages::propertyValueNotInList(asyncResp->res, destUrl,
                                             "Destination");
            return;
        }
        std::string uriProto =
            std::string(matchTok[1].first, matchTok[1].second);
        std::string host = std::string(matchTok[2].first, matchTok[2].second);
        std::string port = std::string(matchTok[3].first, matchTok[3].second);
        std::string path = std::string(matchTok[4].first, matchTok[4].second);
        std::shared_ptr<Subscription> subValue =
            std::make_shared<Subscription>(host, port, path, uriProto);

        subValue->destinationUrl = destUrl;
        subValue->subscriptionType = "Event";
        if (protocol != "Redfish")
        {
            messages::propertyValueNotInList(asyncResp->res, protocol,
                                             "Protocol");
            return;
        }
        subValue->protocol = protocol;

        if (eventType)
        {
            if (std::find(supportedEvtTypes.begin(), supportedEvtTypes.end(),
                          *eventType) == supportedEvtTypes.end())
            {
                messages::propertyValueNotInList(asyncResp->res, *eventType,
                                                 "EventFormatType");
                return;
            }
            subValue->eventFormatType = *eventType;
        }
        else
        {
            // If not specified, use default "Event"
            subValue->eventFormatType.assign({"Event"});
        }

        if (context)
        {
            subValue->customText = *context;
        }

        if (headers)
        {
            subValue->httpHeaders = *headers;
        }

        if (regPrefixes)
        {
            for (const std::string& it : *regPrefixes)
            {
                if (std::find(supportedRegPrefixes.begin(),
                              supportedRegPrefixes.end(),
                              it) == supportedRegPrefixes.end())
                {
                    messages::propertyValueNotInList(asyncResp->res, it,
                                                     "RegistryPrefixes");
                    return;
                }
            }
            subValue->registryPrefixes = *regPrefixes;
        }

        if (msgIds)
        {
            // Do we need to loop-up MessageRegistry and validate
            // data for authenticity??? Not mandate, i believe.
            subValue->registryMsgIds = *msgIds;
        }

        if (retryPolicy)
        {
            if (std::find(supportedRetryPolicies.begin(),
                          supportedRetryPolicies.end(),
                          *retryPolicy) == supportedRetryPolicies.end())
            {
                messages::propertyValueNotInList(asyncResp->res, *retryPolicy,
                                                 "DeliveryRetryPolicy");
                return;
            }
            subValue->retryPolicy = *retryPolicy;
        }
        else
        {
            // Default "TerminateAfterRetries"
            subValue->retryPolicy = "TerminateAfterRetries";
        }

        if (metricReportDefinitions)
        {
            // TODO Validate
            subValue->metricReportDefinitions = *metricReportDefinitions;
        }

        if (originResources)
        {
            subValue->originResources = *originResources;
        }

        std::string id = evtSrvManager.addSubscription(subValue);

        messages::created(asyncResp->res);
        asyncResp->res.addHeader(
            "Location", "/redfish/v1/EventService/Subscriptions/" + id);
    }
};

class EventDestination : public Node
{
  public:
    EventDestination(CrowApp& app) :
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

        std::shared_ptr<Subscription> subValue =
            evtSrvManager.getSubscription(params[0]);
        if (subValue == nullptr)
        {
            res.result(boost::beast::http::status::not_found);
            res.end();
            return;
        }
        const std::string& id = params[0];

        res.jsonValue = {
            {"@odata.type", "#EventDestination.v1_6_0.EventDestination"},
            {"@odata.context",
             "/redfish/v1/$metadata#EventDestination.EventDestination"},
            {"Protocol", "Redfish"}};
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/EventService/Subscriptions/" + id;
        asyncResp->res.jsonValue["Id"] = id;
        asyncResp->res.jsonValue["Name"] = "Event Subscription " + id;
        asyncResp->res.jsonValue["Destination"] = subValue->destinationUrl;
        asyncResp->res.jsonValue["Context"] = subValue->customText;
        asyncResp->res.jsonValue["SubscriptionType"] =
            subValue->subscriptionType;
        asyncResp->res.jsonValue["HttpHeaders"] = subValue->httpHeaders;
        asyncResp->res.jsonValue["EventFormatType"] = subValue->eventFormatType;
        asyncResp->res.jsonValue["RegistryPrefixes"] =
            subValue->registryPrefixes;
        asyncResp->res.jsonValue["OriginResources"] = subValue->originResources;
        asyncResp->res.jsonValue["ResourceTypes"] = subValue->resourceTypes;
        asyncResp->res.jsonValue["MessageIds"] = subValue->registryMsgIds;
        asyncResp->res.jsonValue["MetricReportDefinitions"] =
            subValue->metricReportDefinitions;
        asyncResp->res.jsonValue["DeliveryRetryPolicy"] = subValue->retryPolicy;
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

        std::shared_ptr<Subscription> subValue =
            evtSrvManager.getSubscription(params[0]);
        if (subValue == nullptr)
        {
            res.result(boost::beast::http::status::not_found);
            res.end();
            return;
        }

        std::optional<std::string> context;
        std::optional<std::string> retryPolicy;
        std::optional<std::vector<nlohmann::json>> headers;

        if (!json_util::readJson(req, res, "Context", context,
                                 "DeliveryRetryPolicy", retryPolicy,
                                 "HttpHeaders", headers))
        {
            return;
        }

        if (context)
        {
            subValue->customText = *context;
        }

        if (headers)
        {
            subValue->httpHeaders = *headers;
        }

        if (retryPolicy)
        {
            if (std::find(supportedRetryPolicies.begin(),
                          supportedRetryPolicies.end(),
                          *retryPolicy) == supportedRetryPolicies.end())
            {
                messages::propertyValueNotInList(asyncResp->res, *retryPolicy,
                                                 "DeliveryRetryPolicy");
                return;
            }
            subValue->retryPolicy = *retryPolicy;
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

        if (!evtSrvManager.isSubscriptionExist(params[0]))
        {
            res.result(boost::beast::http::status::not_found);
            res.end();
            return;
        }
        evtSrvManager.deleteSubscription(params[0]);
    }
};

} // namespace redfish
