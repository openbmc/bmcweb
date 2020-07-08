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
#include "event_service_manager.hpp"

namespace redfish
{

static constexpr const std::array<const char*, 2> supportedEvtFormatTypes = {
    eventFormatType, metricReportFormatType};
static constexpr const std::array<const char*, 3> supportedRegPrefixes = {
    "Base", "OpenBMC", "Task"};
static constexpr const std::array<const char*, 3> supportedRetryPolicies = {
    "TerminateAfterRetries", "SuspendRetries", "RetryForever"};

static constexpr const uint8_t maxNoOfSubscriptions = 20;

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
            {"@odata.type", "#EventService.v1_5_0.EventService"},
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

        const auto& [enabled, retryAttempts, retryTimeoutInterval] =
            EventServiceManager::getInstance().getEventServiceConfig();

        asyncResp->res.jsonValue["Status"]["State"] =
            (enabled ? "Enabled" : "Disabled");
        asyncResp->res.jsonValue["ServiceEnabled"] = enabled;
        asyncResp->res.jsonValue["DeliveryRetryAttempts"] = retryAttempts;
        asyncResp->res.jsonValue["DeliveryRetryIntervalSeconds"] =
            retryTimeoutInterval;
        asyncResp->res.jsonValue["EventFormatTypes"] = supportedEvtFormatTypes;
        asyncResp->res.jsonValue["RegistryPrefixes"] = supportedRegPrefixes;

        nlohmann::json supportedSSEFilters = {
            {"EventFormatType", true},        {"MessageId", true},
            {"MetricReportDefinition", true}, {"RegistryPrefix", true},
            {"OriginResource", false},        {"ResourceType", false}};

        asyncResp->res.jsonValue["SSEFilterPropertiesSupported"] =
            supportedSSEFilters;
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

        auto [enabled, retryCount, retryTimeoutInterval] =
            EventServiceManager::getInstance().getEventServiceConfig();

        if (serviceEnabled)
        {
            enabled = *serviceEnabled;
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
                retryCount = *retryAttemps;
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
                retryTimeoutInterval = *retryInterval;
            }
        }

        EventServiceManager::getInstance().setEventServiceConfig(
            std::make_tuple(enabled, retryCount, retryTimeoutInterval));
    }
};

class SubmitTestEvent : public Node
{
  public:
    SubmitTestEvent(CrowApp& app) :
        Node(app,
             "/redfish/v1/EventService/Actions/EventService.SubmitTestEvent/")
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
    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>& params) override
    {
        EventServiceManager::getInstance().sendTestEventLog();
        res.result(boost::beast::http::status::no_content);
        res.end();
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
             "#EventDestinationCollection.EventDestinationCollection"},
            {"@odata.id", "/redfish/v1/EventService/Subscriptions"},
            {"Name", "Event Destination Collections"}};

        nlohmann::json& memberArray = asyncResp->res.jsonValue["Members"];

        std::vector<std::string> subscripIds =
            EventServiceManager::getInstance().getAllIDs();
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

        if (EventServiceManager::getInstance().getNumberOfSubscriptions() >=
            maxNoOfSubscriptions)
        {
            messages::eventSubscriptionLimitExceeded(asyncResp->res);
            return;
        }
        std::string destUrl;
        std::string protocol;
        std::optional<std::string> context;
        std::optional<std::string> subscriptionType;
        std::optional<std::string> eventFormatType;
        std::optional<std::string> retryPolicy;
        std::optional<std::vector<std::string>> msgIds;
        std::optional<std::vector<std::string>> regPrefixes;
        std::optional<std::vector<nlohmann::json>> headers;
        std::optional<std::vector<nlohmann::json>> metricReportDefinitions;

        if (!json_util::readJson(
                req, res, "Destination", destUrl, "Context", context,
                "Protocol", protocol, "SubscriptionType", subscriptionType,
                "EventFormatType", eventFormatType, "HttpHeaders", headers,
                "RegistryPrefixes", regPrefixes, "MessageIds", msgIds,
                "DeliveryRetryPolicy", retryPolicy, "MetricReportDefinitions",
                metricReportDefinitions))
        {
            return;
        }

        // Validate the URL using regex expression
        // Format: <protocol>://<host>:<port>/<uri>
        // protocol: http/https
        // host: Exclude ' ', ':', '#', '?'
        // port: Empty or numeric value with ':' separator.
        // uri: Start with '/' and Exclude '#', ' '
        //      Can include query params(ex: '/event?test=1')
        // TODO: Need to validate hostname extensively(as per rfc)
        const std::regex urlRegex(
            "(http|https)://([^/\\x20\\x3f\\x23\\x3a]+):?([0-9]*)(/"
            "([^\\x20\\x23\\x3f]*\\x3f?([^\\x20\\x23\\x3f])*)?)");
        std::cmatch match;
        if (!std::regex_match(destUrl.c_str(), match, urlRegex))
        {
            messages::propertyValueFormatError(asyncResp->res, destUrl,
                                               "Destination");
            return;
        }

        std::string uriProto = std::string(match[1].first, match[1].second);
        if (uriProto == "http")
        {
#ifndef BMCWEB_INSECURE_ENABLE_HTTP_PUSH_STYLE_EVENTING
            messages::propertyValueFormatError(asyncResp->res, destUrl,
                                               "Destination");
            return;
#endif
        }

        std::string host = std::string(match[2].first, match[2].second);
        std::string port = std::string(match[3].first, match[3].second);
        std::string path = std::string(match[4].first, match[4].second);
        if (port.empty())
        {
            if (uriProto == "http")
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

        std::shared_ptr<Subscription> subValue =
            std::make_shared<Subscription>(host, port, path, uriProto);

        subValue->destinationUrl = destUrl;

        if (subscriptionType)
        {
            if (*subscriptionType != "RedfishEvent")
            {
                messages::propertyValueNotInList(
                    asyncResp->res, *subscriptionType, "SubscriptionType");
                return;
            }
            subValue->subscriptionType = *subscriptionType;
        }
        else
        {
            subValue->subscriptionType = "RedfishEvent"; // Default
        }

        if (protocol != "Redfish")
        {
            messages::propertyValueNotInList(asyncResp->res, protocol,
                                             "Protocol");
            return;
        }
        subValue->protocol = protocol;

        if (eventFormatType)
        {
            if (std::find(supportedEvtFormatTypes.begin(),
                          supportedEvtFormatTypes.end(),
                          *eventFormatType) == supportedEvtFormatTypes.end())
            {
                messages::propertyValueNotInList(
                    asyncResp->res, *eventFormatType, "EventFormatType");
                return;
            }
            subValue->eventFormatType = *eventFormatType;
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
            subValue->metricReportDefinitions = *metricReportDefinitions;
        }

        std::string id =
            EventServiceManager::getInstance().addSubscription(subValue);
        if (id.empty())
        {
            messages::internalError(asyncResp->res);
            return;
        }

        messages::created(asyncResp->res);
        asyncResp->res.addHeader(
            "Location", "/redfish/v1/EventService/Subscriptions/" + id);
    }
};

class EventServiceSSE : public Node
{
  public:
    EventServiceSSE(CrowApp& app) :
        Node(app, "/redfish/v1/EventService/Subscriptions/SSE/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"ConfigureManager"}}},
            {boost::beast::http::verb::head, {{"ConfigureManager"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        if (EventServiceManager::getInstance().getNumberOfSubscriptions() >=
            maxNoOfSubscriptions)
        {
            messages::eventSubscriptionLimitExceeded(res);
            res.end();
            return;
        }

        std::shared_ptr<crow::Request::Adaptor> sseConn =
            std::make_shared<crow::Request::Adaptor>(std::move(req.socket()));
        std::shared_ptr<Subscription> subValue =
            std::make_shared<Subscription>(sseConn);

        // GET on this URI means, Its SSE subscriptionType.
        subValue->subscriptionType = "SSE";

        // Default values
        subValue->protocol = "Redfish";
        subValue->retryPolicy = "TerminateAfterRetries";

        char* filters = req.urlParams.get("$filter");
        if (filters == nullptr)
        {
            subValue->eventFormatType = "Event";
        }
        else
        {
            // Reading from query params.
            bool status = readSSEQueryParams(
                filters, subValue->eventFormatType, subValue->registryMsgIds,
                subValue->registryPrefixes, subValue->metricReportDefinitions);

            if (!status)
            {
                messages::invalidObject(res, filters);
                return;
            }

            if (!subValue->eventFormatType.empty())
            {
                if (std::find(supportedEvtFormatTypes.begin(),
                              supportedEvtFormatTypes.end(),
                              subValue->eventFormatType) ==
                    supportedEvtFormatTypes.end())
                {
                    messages::propertyValueNotInList(
                        res, subValue->eventFormatType, "EventFormatType");
                    return;
                }
            }
            else
            {
                // If nothing specified, using default "Event"
                subValue->eventFormatType.assign({"Event"});
            }

            if (!subValue->registryPrefixes.empty())
            {
                for (const std::string& it : subValue->registryPrefixes)
                {
                    if (std::find(supportedRegPrefixes.begin(),
                                  supportedRegPrefixes.end(),
                                  it) == supportedRegPrefixes.end())
                    {
                        messages::propertyValueNotInList(res, it,
                                                         "RegistryPrefixes");
                        return;
                    }
                }
            }
        }

        std::string id =
            EventServiceManager::getInstance().addSubscription(subValue, false);
        if (id.empty())
        {
            messages::internalError(res);
            res.end();
            return;
        }
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
            EventServiceManager::getInstance().getSubscription(params[0]);
        if (subValue == nullptr)
        {
            res.result(boost::beast::http::status::not_found);
            res.end();
            return;
        }
        const std::string& id = params[0];

        res.jsonValue = {
            {"@odata.type", "#EventDestination.v1_7_0.EventDestination"},
            {"Protocol", "Redfish"}};
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/EventService/Subscriptions/" + id;
        asyncResp->res.jsonValue["Id"] = id;
        asyncResp->res.jsonValue["Name"] = "Event Destination " + id;
        asyncResp->res.jsonValue["Destination"] = subValue->destinationUrl;
        asyncResp->res.jsonValue["Context"] = subValue->customText;
        asyncResp->res.jsonValue["SubscriptionType"] =
            subValue->subscriptionType;
        asyncResp->res.jsonValue["HttpHeaders"] = subValue->httpHeaders;
        asyncResp->res.jsonValue["EventFormatType"] = subValue->eventFormatType;
        asyncResp->res.jsonValue["RegistryPrefixes"] =
            subValue->registryPrefixes;
        asyncResp->res.jsonValue["MessageIds"] = subValue->registryMsgIds;
        asyncResp->res.jsonValue["DeliveryRetryPolicy"] = subValue->retryPolicy;
        asyncResp->res.jsonValue["MetricReportDefinitions"] =
            subValue->metricReportDefinitions;
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
            EventServiceManager::getInstance().getSubscription(params[0]);
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
            subValue->updateRetryPolicy();
        }

        EventServiceManager::getInstance().updateSubscriptionData();
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

        if (!EventServiceManager::getInstance().isSubscriptionExist(params[0]))
        {
            res.result(boost::beast::http::status::not_found);
            res.end();
            return;
        }
        EventServiceManager::getInstance().deleteSubscription(params[0]);
    }
};

} // namespace redfish
