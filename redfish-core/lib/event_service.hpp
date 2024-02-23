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
#include "app.hpp"
#include "event_service_manager.hpp"
#include "http/utility.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "snmp_trap_event_clients.hpp"

#include <boost/beast/http/fields.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/parse.hpp>
#include <sdbusplus/unpack_properties.hpp>
#include <utils/dbus_utils.hpp>

#include <charconv>
#include <memory>
#include <ranges>
#include <span>
#include <string>

namespace redfish
{

static constexpr const std::array<const char*, 2> supportedEvtFormatTypes = {
    eventFormatType, metricReportFormatType};
static constexpr const std::array<const char*, 3> supportedRegPrefixes = {
    "Base", "OpenBMC", "TaskEvent"};
static constexpr const std::array<const char*, 3> supportedRetryPolicies = {
    "TerminateAfterRetries", "SuspendRetries", "RetryForever"};

static constexpr const std::array<const char*, 1> supportedResourceTypes = {
    "Task"};

inline void requestRoutesEventService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/EventService/")
        .privileges(redfish::privileges::getEventService)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }

        asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/EventService";
        asyncResp->res.jsonValue["@odata.type"] =
            "#EventService.v1_5_0.EventService";
        asyncResp->res.jsonValue["Id"] = "EventService";
        asyncResp->res.jsonValue["Name"] = "Event Service";
        asyncResp->res.jsonValue["ServerSentEventUri"] =
            "/redfish/v1/EventService/SSE";

        asyncResp->res.jsonValue["Subscriptions"]["@odata.id"] =
            "/redfish/v1/EventService/Subscriptions";
        asyncResp->res
            .jsonValue["Actions"]["#EventService.SubmitTestEvent"]["target"] =
            "/redfish/v1/EventService/Actions/EventService.SubmitTestEvent";

        const persistent_data::EventServiceConfig eventServiceConfig =
            persistent_data::EventServiceStore::getInstance()
                .getEventServiceConfig();

        asyncResp->res.jsonValue["Status"]["State"] =
            (eventServiceConfig.enabled ? "Enabled" : "Disabled");
        asyncResp->res.jsonValue["ServiceEnabled"] = eventServiceConfig.enabled;
        asyncResp->res.jsonValue["DeliveryRetryAttempts"] =
            eventServiceConfig.retryAttempts;
        asyncResp->res.jsonValue["DeliveryRetryIntervalSeconds"] =
            eventServiceConfig.retryTimeoutInterval;
        asyncResp->res.jsonValue["EventFormatTypes"] = supportedEvtFormatTypes;
        asyncResp->res.jsonValue["RegistryPrefixes"] = supportedRegPrefixes;
        asyncResp->res.jsonValue["ResourceTypes"] = supportedResourceTypes;

        nlohmann::json::object_t supportedSSEFilters;
        supportedSSEFilters["EventFormatType"] = true;
        supportedSSEFilters["MessageId"] = true;
        supportedSSEFilters["MetricReportDefinition"] = true;
        supportedSSEFilters["RegistryPrefix"] = true;
        supportedSSEFilters["OriginResource"] = false;
        supportedSSEFilters["ResourceType"] = false;

        asyncResp->res.jsonValue["SSEFilterPropertiesSupported"] =
            std::move(supportedSSEFilters);
    });

    BMCWEB_ROUTE(app, "/redfish/v1/EventService/")
        .privileges(redfish::privileges::patchEventService)
        .methods(boost::beast::http::verb::patch)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        std::optional<bool> serviceEnabled;
        std::optional<uint32_t> retryAttemps;
        std::optional<uint32_t> retryInterval;

        if (!json_util::readJsonPatch(
                req, asyncResp->res, "ServiceEnabled", serviceEnabled,
                "DeliveryRetryAttempts", retryAttemps,
                "DeliveryRetryIntervalSeconds", retryInterval))
        {
            return;
        }

        persistent_data::EventServiceConfig eventServiceConfig =
            persistent_data::EventServiceStore::getInstance()
                .getEventServiceConfig();

        if (serviceEnabled)
        {
            eventServiceConfig.enabled = *serviceEnabled;
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
                eventServiceConfig.retryAttempts = *retryAttemps;
            }
        }

        if (retryInterval)
        {
            // Supported range [5 - 180]
            if ((*retryInterval < 5) || (*retryInterval > 180))
            {
                messages::queryParameterOutOfRange(
                    asyncResp->res, std::to_string(*retryInterval),
                    "DeliveryRetryIntervalSeconds", "[5-180]");
            }
            else
            {
                eventServiceConfig.retryTimeoutInterval = *retryInterval;
            }
        }

        EventServiceManager::getInstance().setEventServiceConfig(
            eventServiceConfig);
    });
}

inline void requestRoutesSubmitTestEvent(App& app)
{
    BMCWEB_ROUTE(
        app, "/redfish/v1/EventService/Actions/EventService.SubmitTestEvent/")
        .privileges(redfish::privileges::postEventService)
        .methods(boost::beast::http::verb::post)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        if (!EventServiceManager::getInstance().sendTestEventLog())
        {
            messages::serviceDisabled(asyncResp->res,
                                      "/redfish/v1/EventService/");
            return;
        }
        asyncResp->res.result(boost::beast::http::status::no_content);
    });
}

inline void doSubscriptionCollection(
    const boost::system::error_code& ec,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const dbus::utility::ManagedObjectType& resp)
{
    if (ec)
    {
        if (ec.value() == EBADR || ec.value() == EHOSTUNREACH)
        {
            // This is an optional process so just return if it isn't there
            return;
        }

        BMCWEB_LOG_ERROR("D-Bus response error on GetManagedObjects {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }
    nlohmann::json& memberArray = asyncResp->res.jsonValue["Members"];
    for (const auto& objpath : resp)
    {
        sdbusplus::message::object_path path(objpath.first);
        const std::string snmpId = path.filename();
        if (snmpId.empty())
        {
            BMCWEB_LOG_ERROR("The SNMP client ID is wrong");
            messages::internalError(asyncResp->res);
            return;
        }

        getSnmpSubscriptionList(asyncResp, snmpId, memberArray);
    }
}

inline void requestRoutesEventDestinationCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/EventService/Subscriptions/")
        .privileges(redfish::privileges::getEventDestinationCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        asyncResp->res.jsonValue["@odata.type"] =
            "#EventDestinationCollection.EventDestinationCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/EventService/Subscriptions";
        asyncResp->res.jsonValue["Name"] = "Event Destination Collections";

        nlohmann::json& memberArray = asyncResp->res.jsonValue["Members"];

        std::vector<std::string> subscripIds =
            EventServiceManager::getInstance().getAllIDs();
        memberArray = nlohmann::json::array();
        asyncResp->res.jsonValue["Members@odata.count"] = subscripIds.size();

        for (const std::string& id : subscripIds)
        {
            nlohmann::json::object_t member;
            member["@odata.id"] = boost::urls::format(
                "/redfish/v1/EventService/Subscriptions/{}" + id);
            memberArray.emplace_back(std::move(member));
        }
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code& ec,
                        const dbus::utility::ManagedObjectType& resp) {
            doSubscriptionCollection(ec, asyncResp, resp);
        },
            "xyz.openbmc_project.Network.SNMP",
            "/xyz/openbmc_project/network/snmp/manager",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    });

    BMCWEB_ROUTE(app, "/redfish/v1/EventService/Subscriptions/")
        .privileges(redfish::privileges::postEventDestinationCollection)
        .methods(boost::beast::http::verb::post)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
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
        std::optional<std::string> eventFormatType2;
        std::optional<std::string> retryPolicy;
        std::optional<std::vector<std::string>> msgIds;
        std::optional<std::vector<std::string>> regPrefixes;
        std::optional<std::vector<std::string>> resTypes;
        std::optional<std::vector<nlohmann::json>> headers;
        std::optional<std::vector<nlohmann::json>> mrdJsonArray;

        if (!json_util::readJsonPatch(
                req, asyncResp->res, "Destination", destUrl, "Context", context,
                "Protocol", protocol, "SubscriptionType", subscriptionType,
                "EventFormatType", eventFormatType2, "HttpHeaders", headers,
                "RegistryPrefixes", regPrefixes, "MessageIds", msgIds,
                "DeliveryRetryPolicy", retryPolicy, "MetricReportDefinitions",
                mrdJsonArray, "ResourceTypes", resTypes))
        {
            return;
        }

        // https://stackoverflow.com/questions/417142/what-is-the-maximum-length-of-a-url-in-different-browsers
        static constexpr const uint16_t maxDestinationSize = 2000;
        if (destUrl.size() > maxDestinationSize)
        {
            messages::stringValueTooLong(asyncResp->res, "Destination",
                                         maxDestinationSize);
            return;
        }

        if (regPrefixes && msgIds)
        {
            if (!regPrefixes->empty() && !msgIds->empty())
            {
                messages::propertyValueConflict(asyncResp->res, "MessageIds",
                                                "RegistryPrefixes");
                return;
            }
        }

        boost::system::result<boost::urls::url> url =
            boost::urls::parse_absolute_uri(destUrl);
        if (!url)
        {
            BMCWEB_LOG_WARNING("Failed to validate and split destination url");
            messages::propertyValueFormatError(asyncResp->res, destUrl,
                                               "Destination");
            return;
        }
        url->normalize();
        crow::utility::setProtocolDefaults(*url, protocol);
        crow::utility::setPortDefaults(*url);

        if (url->path().empty())
        {
            url->set_path("/");
        }

        if (url->has_userinfo())
        {
            messages::propertyValueFormatError(asyncResp->res, destUrl,
                                               "Destination");
            return;
        }

        /* Check snmpTrap port.
        1. Allow empty port, eg:
        "snmp://9.41.166.76:".
        2. Invalid ports and out of range ports are not allowed, eg:
        "snmp://9.41.166.76:ab"
        "snmp://9.41.166.76:-40"
        "snmp://9.41.166.76:65536".
        */
        if (protocol == "snmp")
        {
            if (!url.value().port().empty())
            {
                uint16_t portTmp = 0;
                // Check the port
                auto ret = std::from_chars(url.value().port().data(),
                                           url.value().port().data() +
                                               url.value().port().size(),
                                           portTmp);
                if (ret.ec != std::errc())
                {
                    messages::propertyValueFormatError(asyncResp->res, destUrl,
                                                       "Destination");
                    return;
                }
            }
            else
            {
                size_t pos = destUrl.find(':');
                if (pos != std::string::npos)
                {
                    std::string tmp_str = std::string(destUrl).substr(pos + 1);
                    size_t pos2 = tmp_str.find(':');
                    if (pos2 != std::string::npos)
                    {
                        std::string port_str = tmp_str.substr(pos2 + 1);
                        if (!port_str.empty())
                        {
                            messages::propertyValueFormatError(
                                asyncResp->res, destUrl, "Destination");
                            return;
                        }
                    }
                }
            }
        }

        if (protocol == "SNMPv2c")
        {
            if (context)
            {
                messages::propertyValueConflict(asyncResp->res, "Context",
                                                "Protocol");
                return;
            }
            if (eventFormatType2)
            {
                messages::propertyValueConflict(asyncResp->res,
                                                "EventFormatType", "Protocol");
                return;
            }
            if (retryPolicy)
            {
                messages::propertyValueConflict(asyncResp->res, "RetryPolicy",
                                                "Protocol");
                return;
            }
            if (msgIds)
            {
                messages::propertyValueConflict(asyncResp->res, "MessageIds",
                                                "Protocol");
                return;
            }
            if (regPrefixes)
            {
                messages::propertyValueConflict(asyncResp->res,
                                                "RegistryPrefixes", "Protocol");
                return;
            }
            if (resTypes)
            {
                messages::propertyValueConflict(asyncResp->res, "ResourceTypes",
                                                "Protocol");
                return;
            }
            if (headers)
            {
                messages::propertyValueConflict(asyncResp->res, "HttpHeaders",
                                                "Protocol");
                return;
            }
            if (mrdJsonArray)
            {
                messages::propertyValueConflict(
                    asyncResp->res, "MetricReportDefinitions", "Protocol");
                return;
            }
            if (url->scheme() != "snmp")
            {
                messages::propertyValueConflict(asyncResp->res, "Destination",
                                                "Protocol");
                return;
            }

            addSnmpTrapClient(asyncResp, url->host_address(),
                              url->port_number());
            return;
        }

        std::shared_ptr<Subscription> subValue =
            std::make_shared<Subscription>(*url, app.ioContext());

        subValue->destinationUrl = std::move(*url);

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

        if (eventFormatType2)
        {
            if (std::ranges::find(supportedEvtFormatTypes, *eventFormatType2) ==
                supportedEvtFormatTypes.end())
            {
                messages::propertyValueNotInList(
                    asyncResp->res, *eventFormatType2, "EventFormatType");
                return;
            }
            subValue->eventFormatType = *eventFormatType2;
        }
        else
        {
            // If not specified, use default "Event"
            subValue->eventFormatType = "Event";
        }

        if (context)
        {
            // This value is selected arbitrarily.
            constexpr const size_t maxContextSize = 256;
            if (context->size() > maxContextSize)
            {
                messages::stringValueTooLong(asyncResp->res, "Context",
                                             maxContextSize);
                return;
            }
            subValue->customText = *context;
        }

        if (headers)
        {
            size_t cumulativeLen = 0;

            for (const nlohmann::json& headerChunk : *headers)
            {
                std::string hdr{headerChunk.dump(
                    -1, ' ', true, nlohmann::json::error_handler_t::replace)};
                cumulativeLen += hdr.length();

                // This value is selected to mirror http_connection.hpp
                constexpr const uint16_t maxHeaderSizeED = 8096;
                if (cumulativeLen > maxHeaderSizeED)
                {
                    messages::arraySizeTooLong(asyncResp->res, "HttpHeaders",
                                               maxHeaderSizeED);
                    return;
                }
                for (const auto& item : headerChunk.items())
                {
                    const std::string* value =
                        item.value().get_ptr<const std::string*>();
                    if (value == nullptr)
                    {
                        messages::propertyValueFormatError(
                            asyncResp->res, item.value(),
                            "HttpHeaders/" + item.key());
                        return;
                    }
                    subValue->httpHeaders.set(item.key(), *value);
                }
            }
        }

        if (regPrefixes)
        {
            for (const std::string& it : *regPrefixes)
            {
                if (std::ranges::find(supportedRegPrefixes, it) ==
                    supportedRegPrefixes.end())
                {
                    messages::propertyValueNotInList(asyncResp->res, it,
                                                     "RegistryPrefixes");
                    return;
                }
            }
            subValue->registryPrefixes = *regPrefixes;
        }

        if (resTypes)
        {
            for (const std::string& it : *resTypes)
            {
                if (std::ranges::find(supportedResourceTypes, it) ==
                    supportedResourceTypes.end())
                {
                    messages::propertyValueNotInList(asyncResp->res, it,
                                                     "ResourceTypes");
                    return;
                }
            }
            subValue->resourceTypes = *resTypes;
        }

        if (msgIds)
        {
            std::vector<std::string> registryPrefix;

            // If no registry prefixes are mentioned, consider all
            // supported prefixes
            if (subValue->registryPrefixes.empty())
            {
                registryPrefix.assign(supportedRegPrefixes.begin(),
                                      supportedRegPrefixes.end());
            }
            else
            {
                registryPrefix = subValue->registryPrefixes;
            }

            for (const std::string& id : *msgIds)
            {
                bool validId = false;

                // Check for Message ID in each of the selected Registry
                for (const std::string& it : registryPrefix)
                {
                    const std::span<const redfish::registries::MessageEntry>
                        registry =
                            redfish::registries::getRegistryFromPrefix(it);

                    if (std::ranges::any_of(
                            registry,
                            [&id](const redfish::registries::MessageEntry&
                                      messageEntry) {
                        return id == messageEntry.first;
                    }))
                    {
                        validId = true;
                        break;
                    }
                }

                if (!validId)
                {
                    messages::propertyValueNotInList(asyncResp->res, id,
                                                     "MessageIds");
                    return;
                }
            }

            subValue->registryMsgIds = *msgIds;
        }

        if (retryPolicy)
        {
            if (std::ranges::find(supportedRetryPolicies, *retryPolicy) ==
                supportedRetryPolicies.end())
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

        if (mrdJsonArray)
        {
            for (nlohmann::json& mrdObj : *mrdJsonArray)
            {
                std::string mrdUri;

                if (!json_util::readJson(mrdObj, asyncResp->res, "@odata.id",
                                         mrdUri))

                {
                    return;
                }
                subValue->metricReportDefinitions.emplace_back(mrdUri);
            }
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
    });
}

inline void requestRoutesEventDestination(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/EventService/Subscriptions/<str>/")
        .privileges(redfish::privileges::getEventDestination)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& param) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }

        if (param.starts_with("snmp"))
        {
            getSnmpTrapClient(asyncResp, param);
            return;
        }

        std::shared_ptr<Subscription> subValue =
            EventServiceManager::getInstance().getSubscription(param);
        if (subValue == nullptr)
        {
            asyncResp->res.result(boost::beast::http::status::not_found);
            return;
        }
        const std::string& id = param;

        asyncResp->res.jsonValue["@odata.type"] =
            "#EventDestination.v1_8_0.EventDestination";
        asyncResp->res.jsonValue["Protocol"] = "Redfish";
        asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
            "/redfish/v1/EventService/Subscriptions/{}", id);
        asyncResp->res.jsonValue["Id"] = id;
        asyncResp->res.jsonValue["Name"] = "Event Destination " + id;
        asyncResp->res.jsonValue["Destination"] = subValue->destinationUrl;
        asyncResp->res.jsonValue["Context"] = subValue->customText;
        asyncResp->res.jsonValue["SubscriptionType"] =
            subValue->subscriptionType;
        asyncResp->res.jsonValue["HttpHeaders"] = nlohmann::json::array();
        asyncResp->res.jsonValue["EventFormatType"] = subValue->eventFormatType;
        asyncResp->res.jsonValue["RegistryPrefixes"] =
            subValue->registryPrefixes;
        asyncResp->res.jsonValue["ResourceTypes"] = subValue->resourceTypes;

        asyncResp->res.jsonValue["MessageIds"] = subValue->registryMsgIds;
        asyncResp->res.jsonValue["DeliveryRetryPolicy"] = subValue->retryPolicy;

        nlohmann::json::array_t mrdJsonArray;
        for (const auto& mdrUri : subValue->metricReportDefinitions)
        {
            nlohmann::json::object_t mdr;
            mdr["@odata.id"] = mdrUri;
            mrdJsonArray.emplace_back(std::move(mdr));
        }
        asyncResp->res.jsonValue["MetricReportDefinitions"] = mrdJsonArray;
    });
    BMCWEB_ROUTE(app, "/redfish/v1/EventService/Subscriptions/<str>/")
        // The below privilege is wrong, it should be ConfigureManager OR
        // ConfigureSelf
        // https://github.com/openbmc/bmcweb/issues/220
        //.privileges(redfish::privileges::patchEventDestination)
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::patch)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& param) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        std::shared_ptr<Subscription> subValue =
            EventServiceManager::getInstance().getSubscription(param);
        if (subValue == nullptr)
        {
            asyncResp->res.result(boost::beast::http::status::not_found);
            return;
        }

        std::optional<std::string> context;
        std::optional<std::string> retryPolicy;
        std::optional<std::vector<nlohmann::json>> headers;

        if (!json_util::readJsonPatch(req, asyncResp->res, "Context", context,
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
            boost::beast::http::fields fields;
            for (const nlohmann::json& headerChunk : *headers)
            {
                for (const auto& it : headerChunk.items())
                {
                    const std::string* value =
                        it.value().get_ptr<const std::string*>();
                    if (value == nullptr)
                    {
                        messages::propertyValueFormatError(
                            asyncResp->res, it.value(),
                            "HttpHeaders/" + it.key());
                        return;
                    }
                    fields.set(it.key(), *value);
                }
            }
            subValue->httpHeaders = fields;
        }

        if (retryPolicy)
        {
            if (std::ranges::find(supportedRetryPolicies, *retryPolicy) ==
                supportedRetryPolicies.end())
            {
                messages::propertyValueNotInList(asyncResp->res, *retryPolicy,
                                                 "DeliveryRetryPolicy");
                return;
            }
            subValue->retryPolicy = *retryPolicy;
        }

        EventServiceManager::getInstance().updateSubscriptionData();
    });
    BMCWEB_ROUTE(app, "/redfish/v1/EventService/Subscriptions/<str>/")
        // The below privilege is wrong, it should be ConfigureManager OR
        // ConfigureSelf
        // https://github.com/openbmc/bmcweb/issues/220
        //.privileges(redfish::privileges::deleteEventDestination)
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::delete_)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& param) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }

        if (param.starts_with("snmp"))
        {
            deleteSnmpTrapClient(asyncResp, param);
            EventServiceManager::getInstance().deleteSubscription(param);
            return;
        }

        if (!EventServiceManager::getInstance().isSubscriptionExist(param))
        {
            asyncResp->res.result(boost::beast::http::status::not_found);
            return;
        }
        EventServiceManager::getInstance().deleteSubscription(param);
    });
}

} // namespace redfish
