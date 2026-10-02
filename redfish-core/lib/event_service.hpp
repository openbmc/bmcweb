// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2020 Intel Corporation
#pragma once
#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "event_service_manager.hpp"
#include "event_service_store.hpp"
#include "generated/enums/event_destination.hpp"
#include "generated/enums/resource.hpp"
#include "http/utility.hpp"
#include "http_request.hpp"
#include "io_context_singleton.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries.hpp"
#include "registries/privilege_registry.hpp"
#include "snmp_trap_event_clients.hpp"
#include "subscription.hpp"
#include "utils/json_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/result.hpp>
#include <boost/url/format.hpp>
#include <boost/url/host_type.hpp>
#include <boost/url/ipv4_address.hpp>
#include <boost/url/ipv6_address.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/url.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace redfish
{

static constexpr const std::array<const char*, 2> supportedEvtFormatTypes = {
    eventFormatType, metricReportFormatType};
static constexpr const std::array<const char*, 4> supportedRegPrefixes = {
    "Base", "OpenBMC", "TaskEvent", "HeartbeatEvent"};
static constexpr const std::array<const char*, 3> supportedRetryPolicies = {
    "TerminateAfterRetries", "SuspendRetries", "RetryForever"};

static constexpr const std::array<const char*, 2> supportedResourceTypes = {
    "Task", "Heartbeat"};

inline void requestRoutesEventService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/EventService/")
        .privileges(redfish::privileges::getEventService)
        .methods(boost::beast::http::verb::get)
        // ast-grep-ignore: long-lambda
        ([&app](const crow::Request& req,
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
            asyncResp->res.jsonValue["Actions"]["#EventService.SubmitTestEvent"]
                                    ["target"] =
                "/redfish/v1/EventService/Actions/EventService.SubmitTestEvent";

            const persistent_data::EventServiceConfig eventServiceConfig =
                persistent_data::EventServiceStore::getInstance()
                    .getEventServiceConfig();

            asyncResp->res.jsonValue["Status"]["State"] =
                (eventServiceConfig.enabled ? resource::State::Enabled
                                            : resource::State::Disabled);
            asyncResp->res.jsonValue["ServiceEnabled"] =
                eventServiceConfig.enabled;
            asyncResp->res.jsonValue["DeliveryRetryAttempts"] =
                eventServiceConfig.retryAttempts;
            asyncResp->res.jsonValue["DeliveryRetryIntervalSeconds"] =
                eventServiceConfig.retryTimeoutInterval;
            asyncResp->res.jsonValue["EventFormatTypes"] =
                supportedEvtFormatTypes;
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
            // ast-grep-ignore: long-lambda
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                if (!redfish::setUpRedfishRoute(app, req, asyncResp))
                {
                    return;
                }
                std::optional<bool> serviceEnabled;
                std::optional<uint32_t> retryAttempts;
                std::optional<uint32_t> retryInterval;
                if (!json_util::readJsonPatch(                         //
                        req, asyncResp->res,                           //
                        "DeliveryRetryAttempts", retryAttempts,        //
                        "DeliveryRetryIntervalSeconds", retryInterval, //
                        "ServiceEnabled", serviceEnabled               //
                        ))
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

                if (retryAttempts)
                {
                    // Supported range [1-3]
                    if ((*retryAttempts < 1) || (*retryAttempts > 3))
                    {
                        messages::propertyValueOutOfRange(
                            asyncResp->res, *retryAttempts,
                            "DeliveryRetryAttempts");
                        return;
                    }

                    eventServiceConfig.retryAttempts = *retryAttempts;
                }

                if (retryInterval)
                {
                    // Supported range [5 - 180]
                    if ((*retryInterval < 5) || (*retryInterval > 180))
                    {
                        messages::propertyValueOutOfRange(
                            asyncResp->res, *retryInterval,
                            "DeliveryRetryIntervalSeconds");
                        return;
                    }

                    eventServiceConfig.retryTimeoutInterval = *retryInterval;
                }

                EventServiceManager::getInstance().setEventServiceConfig(
                    eventServiceConfig);
                asyncResp->res.result(boost::beast::http::status::no_content);
            });
}

inline void requestRoutesSubmitTestEvent(App& app)
{
    BMCWEB_ROUTE(
        app, "/redfish/v1/EventService/Actions/EventService.SubmitTestEvent/")
        .privileges(redfish::privileges::postEventService)
        .methods(boost::beast::http::verb::post)(
            // ast-grep-ignore: long-lambda
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                if (!redfish::setUpRedfishRoute(app, req, asyncResp))
                {
                    return;
                }

                // From the Redfish spec on EventId
                // A service can ignore this value and replace it with its own.
                // note that this parameter is intentionally ignored

                std::optional<std::string> eventId;
                TestEvent testEvent;
                // clang-format off
                if (!json_util::readJsonAction(
                        req, asyncResp->res,
                        "EventGroupId", testEvent.eventGroupId,
                        "EventId", eventId,
                        "EventTimestamp", testEvent.eventTimestamp,
                        "Message", testEvent.message,
                        "MessageArgs", testEvent.messageArgs,
                        "MessageId", testEvent.messageId,
                        "OriginOfCondition", testEvent.originOfCondition,
                        "Resolution", testEvent.resolution,
                        "Severity", testEvent.severity))
                {
                    return;
                }
                // clang-format on

                if (!EventServiceManager::getInstance().sendTestEventLog(
                        testEvent))
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
        sdbusplus::object_path path(objpath.first);
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

// True if the destination host is a loopback, unspecified or link-local
// address, or the name localhost. Subscriptions must not point bmcweb at its
// own services. A name whose last label is numeric, such as 127.1 or
// 0x7f000001, is an address in a spelling the resolver accepts, so it is
// refused too.
inline bool isBlockedDestinationHost(const boost::urls::url& url)
{
    auto isLocalV4 = [](const boost::urls::ipv4_address& addr) {
        auto bytes = addr.to_bytes();
        return addr.is_loopback() || addr.is_unspecified() ||
               (bytes[0] == 169 && bytes[1] == 254);
    };

    if (url.host_type() == boost::urls::host_type::ipv4)
    {
        return isLocalV4(url.host_ipv4_address());
    }
    if (url.host_type() == boost::urls::host_type::ipv6)
    {
        boost::urls::ipv6_address addr = url.host_ipv6_address();
        auto bytes = addr.to_bytes();
        if (addr.is_loopback() || addr.is_unspecified())
        {
            return true;
        }
        // fe80::/10
        if (bytes[0] == 0xfe && (bytes[1] & 0xc0) == 0x80)
        {
            return true;
        }
        if (addr.is_v4_mapped())
        {
            return isLocalV4(boost::urls::ipv4_address(
                std::array<unsigned char, 4>{bytes[12], bytes[13], bytes[14],
                                             bytes[15]}));
        }
        return false;
    }
    // Only an address or a host name can be delivered to
    if (url.host_type() != boost::urls::host_type::name || url.host().empty())
    {
        return true;
    }

    std::string host = url.host();
    std::ranges::transform(host, host.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    // A trailing dot is the DNS root and names the same host
    if (host.ends_with('.'))
    {
        host.pop_back();
    }
    if (host == "localhost" || host.ends_with(".localhost"))
    {
        return true;
    }

    std::string_view lastLabel = host;
    lastLabel = lastLabel.substr(lastLabel.rfind('.') + 1);
    bool isHex = lastLabel.starts_with("0x") && lastLabel.size() > 2;
    return !lastLabel.empty() &&
           std::ranges::all_of(isHex ? lastLabel.substr(2) : lastLabel,
                               [isHex](char c) {
                                   return (c >= '0' && c <= '9') ||
                                          (isHex && c >= 'a' && c <= 'f');
                               });
}

inline void requestRoutesEventDestinationCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/EventService/Subscriptions/")
        .privileges(redfish::privileges::getEventDestinationCollection)
        .methods(boost::beast::http::verb::get)(
            // ast-grep-ignore: long-lambda
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
                asyncResp->res.jsonValue["Name"] =
                    "Event Destination Collections";

                nlohmann::json& memberArray =
                    asyncResp->res.jsonValue["Members"];

                std::vector<std::string> subscripIds =
                    EventServiceManager::getInstance().getAllIDs();
                memberArray = nlohmann::json::array();
                asyncResp->res.jsonValue["Members@odata.count"] =
                    subscripIds.size();

                for (const std::string& id : subscripIds)
                {
                    nlohmann::json::object_t member;
                    member["@odata.id"] = boost::urls::format(
                        "/redfish/v1/EventService/Subscriptions/{}" + id);
                    memberArray.emplace_back(std::move(member));
                }
                dbus::utility::async_method_call(
                    asyncResp,
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
        .methods(boost::beast::http::verb::post)
        // ast-grep-ignore: long-lambda
        ([&app](const crow::Request& req,
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
            std::optional<bool> verifyCertificate;
            std::optional<std::string> context;
            std::optional<std::string> subscriptionType;
            std::optional<std::string> eventFormatType2;
            std::optional<std::string> retryPolicy;
            std::optional<bool> sendHeartbeat;
            std::optional<uint64_t> hbIntervalMinutes;
            std::optional<std::vector<std::string>> msgIds;
            std::optional<std::vector<std::string>> regPrefixes;
            std::optional<std::vector<std::string>> originResources;
            std::optional<std::vector<std::string>> resTypes;
            std::optional<std::vector<nlohmann::json::object_t>> headers;
            std::optional<std::vector<nlohmann::json::object_t>> mrdJsonArray;

            if (!json_util::readJsonPatch(                         //
                    req, asyncResp->res,                           //
                    "Context", context,                            //
                    "DeliveryRetryPolicy", retryPolicy,            //
                    "Destination", destUrl,                        //
                    "EventFormatType", eventFormatType2,           //
                    "HeartbeatIntervalMinutes", hbIntervalMinutes, //
                    "HttpHeaders", headers,                        //
                    "MessageIds", msgIds,                          //
                    "MetricReportDefinitions", mrdJsonArray,       //
                    "OriginResources", originResources,            //
                    "Protocol", protocol,                          //
                    "RegistryPrefixes", regPrefixes,               //
                    "ResourceTypes", resTypes,                     //
                    "SendHeartbeat", sendHeartbeat,                //
                    "SubscriptionType", subscriptionType,          //
                    "VerifyCertificate", verifyCertificate         //
                    ))
            {
                return;
            }
            // clang-format on

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
                    messages::propertyValueConflict(
                        asyncResp->res, "MessageIds", "RegistryPrefixes");
                    return;
                }
            }

            boost::system::result<boost::urls::url> url =
                boost::urls::parse_absolute_uri(destUrl);
            if (!url)
            {
                BMCWEB_LOG_WARNING(
                    "Failed to validate and split destination url");
                messages::propertyValueFormatError(asyncResp->res, destUrl,
                                                   "Destination");
                return;
            }
            url->normalize();

            // port_number returns zero if it is not a valid representable port
            if (url->has_port() && url->port_number() == 0)
            {
                BMCWEB_LOG_WARNING("{} is an invalid port in destination url",
                                   url->port());
                messages::propertyValueFormatError(asyncResp->res, destUrl,
                                                   "Destination");
                return;
            }

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
                    messages::propertyValueConflict(
                        asyncResp->res, "EventFormatType", "Protocol");
                    return;
                }
                if (retryPolicy)
                {
                    messages::propertyValueConflict(asyncResp->res,
                                                    "RetryPolicy", "Protocol");
                    return;
                }
                if (sendHeartbeat)
                {
                    messages::propertyValueConflict(
                        asyncResp->res, "SendHeartbeat", "Protocol");
                    return;
                }
                if (hbIntervalMinutes)
                {
                    messages::propertyValueConflict(
                        asyncResp->res, "HeartbeatIntervalMinutes", "Protocol");
                    return;
                }
                if (msgIds)
                {
                    messages::propertyValueConflict(asyncResp->res,
                                                    "MessageIds", "Protocol");
                    return;
                }
                if (regPrefixes)
                {
                    messages::propertyValueConflict(
                        asyncResp->res, "RegistryPrefixes", "Protocol");
                    return;
                }
                if (resTypes)
                {
                    messages::propertyValueConflict(
                        asyncResp->res, "ResourceTypes", "Protocol");
                    return;
                }
                if (headers)
                {
                    messages::propertyValueConflict(asyncResp->res,
                                                    "HttpHeaders", "Protocol");
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
                    messages::propertyValueConflict(asyncResp->res,
                                                    "Destination", "Protocol");
                    return;
                }

                addSnmpTrapClient(asyncResp, url->host_address(),
                                  url->port_number());
                return;
            }

            // The client only uses TLS for https, so any other scheme would be
            // sent in the clear.
            bool schemeAllowed = url->scheme() == "https";
            if constexpr (BMCWEB_INSECURE_PUSH_STYLE_NOTIFICATION)
            {
                schemeAllowed = schemeAllowed || url->scheme() == "http";
            }
            if (!schemeAllowed)
            {
                messages::propertyValueFormatError(asyncResp->res, destUrl,
                                                   "Destination");
                return;
            }

            if (isBlockedDestinationHost(*url))
            {
                messages::propertyValueFormatError(asyncResp->res, destUrl,
                                                   "Destination");
                return;
            }

            std::shared_ptr<Subscription> subValue =
                std::make_shared<Subscription>(
                    std::make_shared<persistent_data::UserSubscription>(), *url,
                    getIoContext());

            if (subscriptionType)
            {
                if (*subscriptionType != "RedfishEvent")
                {
                    messages::propertyValueNotInList(
                        asyncResp->res, *subscriptionType, "SubscriptionType");
                    return;
                }
                subValue->userSub->subscriptionType = *subscriptionType;
            }
            else
            {
                // Default
                subValue->userSub->subscriptionType = "RedfishEvent";
            }

            if (protocol != "Redfish")
            {
                messages::propertyValueNotInList(asyncResp->res, protocol,
                                                 "Protocol");
                return;
            }
            subValue->userSub->protocol = protocol;

            if (verifyCertificate)
            {
                subValue->userSub->verifyCertificate = *verifyCertificate;
            }

            if (eventFormatType2)
            {
                if (std::ranges::find(supportedEvtFormatTypes,
                                      *eventFormatType2) ==
                    supportedEvtFormatTypes.end())
                {
                    messages::propertyValueNotInList(
                        asyncResp->res, *eventFormatType2, "EventFormatType");
                    return;
                }
                subValue->userSub->eventFormatType = *eventFormatType2;
            }
            else
            {
                // If not specified, use default "Event"
                subValue->userSub->eventFormatType = "Event";
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
                subValue->userSub->customText = *context;
            }

            if (headers)
            {
                size_t cumulativeLen = 0;

                for (const nlohmann::json::object_t& headerChunk : *headers)
                {
                    for (const auto& item : headerChunk)
                    {
                        const std::string* value =
                            item.second.get_ptr<const std::string*>();
                        if (value == nullptr)
                        {
                            messages::propertyValueFormatError(
                                asyncResp->res, item.second,
                                "HttpHeaders/" + item.first);
                            return;
                        }
                        if (!persistent_data::isAllowedHeaderName(item.first) ||
                            !persistent_data::isSafeHeaderText(*value))
                        {
                            messages::propertyValueFormatError(
                                asyncResp->res, "(invalid characters)",
                                "HttpHeaders");
                            return;
                        }
                        // Adding a new json value is the size of the key, +
                        // the size of the value + 2 * 2 quotes for each, +
                        // the colon and space between. example:
                        // "key": "value"
                        cumulativeLen += item.first.size() + value->size() + 6;
                        // This value is selected to mirror http_connection.hpp
                        constexpr const uint16_t maxHeaderSizeED = 8096;
                        if (cumulativeLen > maxHeaderSizeED)
                        {
                            messages::arraySizeTooLong(
                                asyncResp->res, "HttpHeaders", maxHeaderSizeED);
                            return;
                        }
                        subValue->userSub->httpHeaders.set(item.first, *value);
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
                subValue->userSub->registryPrefixes = *regPrefixes;
            }

            if (originResources)
            {
                subValue->userSub->originResources = *originResources;
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
                subValue->userSub->resourceTypes = *resTypes;
            }

            if (msgIds)
            {
                std::vector<std::string> registryPrefix;

                // If no registry prefixes are mentioned, consider all
                // supported prefixes
                if (subValue->userSub->registryPrefixes.empty())
                {
                    registryPrefix.assign(supportedRegPrefixes.begin(),
                                          supportedRegPrefixes.end());
                }
                else
                {
                    registryPrefix = subValue->userSub->registryPrefixes;
                }

                for (const std::string& id : *msgIds)
                {
                    bool validId = false;

                    // Check for Message ID in each of the selected Registry
                    for (const std::string& it : registryPrefix)
                    {
                        const registries::MessageEntries registry =
                            redfish::registries::getRegistryMessagesFromPrefix(
                                it);

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

                subValue->userSub->registryMsgIds = *msgIds;
            }

            if (retryPolicy)
            {
                if (std::ranges::find(supportedRetryPolicies, *retryPolicy) ==
                    supportedRetryPolicies.end())
                {
                    messages::propertyValueNotInList(
                        asyncResp->res, *retryPolicy, "DeliveryRetryPolicy");
                    return;
                }
                subValue->userSub->retryPolicy = *retryPolicy;
            }
            else
            {
                // Default "TerminateAfterRetries"
                subValue->userSub->retryPolicy = "TerminateAfterRetries";
            }
            if (sendHeartbeat)
            {
                subValue->userSub->sendHeartbeat = *sendHeartbeat;
            }
            if (hbIntervalMinutes)
            {
                if (*hbIntervalMinutes < 1 || *hbIntervalMinutes > 65535)
                {
                    messages::propertyValueOutOfRange(
                        asyncResp->res, *hbIntervalMinutes,
                        "HeartbeatIntervalMinutes");
                    return;
                }
                subValue->userSub->hbIntervalMinutes = *hbIntervalMinutes;
            }

            if (mrdJsonArray)
            {
                for (nlohmann::json::object_t& mrdObj : *mrdJsonArray)
                {
                    std::string mrdUri;

                    if (!json_util::readJsonObject(mrdObj, asyncResp->res,
                                                   "@odata.id", mrdUri))

                    {
                        return;
                    }
                    subValue->userSub->metricReportDefinitions.emplace_back(
                        mrdUri);
                }
            }

            std::string id =
                EventServiceManager::getInstance().addPushSubscription(
                    subValue);
            if (id.empty())
            {
                messages::internalError(asyncResp->res);
                return;
            }

            messages::created(asyncResp->res);
            asyncResp->res.addHeader(
                "Location", "/redfish/v1/EventService/Subscriptions/" + id);

            // schedule a heartbeat
            if (subValue->userSub->sendHeartbeat)
            {
                subValue->scheduleNextHeartbeatEvent();
            }
        });
}

inline void handleEventDestinationDelete(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& param)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    EventServiceManager& event = EventServiceManager::getInstance();
    if (param.starts_with("snmp"))
    {
        deleteSnmpTrapClient(asyncResp, param);
        event.deleteSubscription(param);
        return;
    }

    if (!event.deleteSubscription(param))
    {
        messages::resourceNotFound(asyncResp->res, "EventDestination", param);
        return;
    }
    messages::success(asyncResp->res);
}

inline void requestRoutesEventDestination(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/EventService/Subscriptions/<str>/")
        .privileges(redfish::privileges::getEventDestination)
        .methods(boost::beast::http::verb::get)(
            // ast-grep-ignore: long-lambda
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
                    asyncResp->res.result(
                        boost::beast::http::status::not_found);
                    return;
                }
                const std::string& id = param;

                const persistent_data::UserSubscription& userSub =
                    *subValue->userSub;

                nlohmann::json& jVal = asyncResp->res.jsonValue;
                jVal["@odata.type"] =
                    "#EventDestination.v1_14_1.EventDestination";
                jVal["Protocol"] =
                    event_destination::EventDestinationProtocol::Redfish;
                jVal["@odata.id"] = boost::urls::format(
                    "/redfish/v1/EventService/Subscriptions/{}", id);
                jVal["Id"] = id;
                jVal["Name"] = "Event Destination " + id;
                jVal["Destination"] = userSub.destinationUrl;
                jVal["Context"] = userSub.customText;
                jVal["SubscriptionType"] = userSub.subscriptionType;
                jVal["HttpHeaders"] = nlohmann::json::array();
                jVal["EventFormatType"] = userSub.eventFormatType;
                jVal["RegistryPrefixes"] = userSub.registryPrefixes;
                jVal["ResourceTypes"] = userSub.resourceTypes;

                jVal["MessageIds"] = userSub.registryMsgIds;
                jVal["DeliveryRetryPolicy"] = userSub.retryPolicy;
                jVal["SendHeartbeat"] = userSub.sendHeartbeat;
                jVal["HeartbeatIntervalMinutes"] = userSub.hbIntervalMinutes;
                jVal["VerifyCertificate"] = userSub.verifyCertificate;

                nlohmann::json::array_t mrdJsonArray;
                for (const auto& mdrUri : userSub.metricReportDefinitions)
                {
                    nlohmann::json::object_t mdr;
                    mdr["@odata.id"] = mdrUri;
                    mrdJsonArray.emplace_back(std::move(mdr));
                }
                jVal["MetricReportDefinitions"] = mrdJsonArray;
            });
    BMCWEB_ROUTE(app, "/redfish/v1/EventService/Subscriptions/<str>/")
        // The below privilege is wrong, it should be ConfigureManager OR
        // ConfigureSelf
        // https://github.com/openbmc/bmcweb/issues/220
        //.privileges(redfish::privileges::patchEventDestination)
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::patch)(
            // ast-grep-ignore: long-lambda
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
                    messages::resourceNotFound(asyncResp->res,
                                               "EventDestination", param);
                    return;
                }

                std::optional<std::string> context;
                std::optional<std::string> retryPolicy;
                std::optional<bool> sendHeartbeat;
                std::optional<uint64_t> hbIntervalMinutes;
                std::optional<bool> verifyCertificate;
                std::optional<std::vector<nlohmann::json::object_t>> headers;

                if (!json_util::readJsonPatch(                         //
                        req, asyncResp->res,                           //
                        "Context", context,                            //
                        "DeliveryRetryPolicy", retryPolicy,            //
                        "HeartbeatIntervalMinutes", hbIntervalMinutes, //
                        "HttpHeaders", headers,                        //
                        "SendHeartbeat", sendHeartbeat,                //
                        "VerifyCertificate", verifyCertificate         //
                        ))
                {
                    return;
                }

                // Validate everything first so a rejected request changes
                // nothing.
                std::optional<boost::beast::http::fields> newHeaders;
                if (headers)
                {
                    boost::beast::http::fields fields;
                    for (const nlohmann::json::object_t& headerChunk : *headers)
                    {
                        for (const auto& it : headerChunk)
                        {
                            const std::string* value =
                                it.second.get_ptr<const std::string*>();
                            if (value == nullptr)
                            {
                                messages::propertyValueFormatError(
                                    asyncResp->res, it.second,
                                    "HttpHeaders/" + it.first);
                                return;
                            }
                            if (!persistent_data::isAllowedHeaderName(
                                    it.first) ||
                                !persistent_data::isSafeHeaderText(*value))
                            {
                                messages::propertyValueFormatError(
                                    asyncResp->res, "(invalid characters)",
                                    "HttpHeaders");
                                return;
                            }
                            fields.set(it.first, *value);
                        }
                    }
                    newHeaders = std::move(fields);
                }

                if (retryPolicy)
                {
                    if (std::ranges::find(supportedRetryPolicies,
                                          *retryPolicy) ==
                        supportedRetryPolicies.end())
                    {
                        messages::propertyValueNotInList(asyncResp->res,
                                                         *retryPolicy,
                                                         "DeliveryRetryPolicy");
                        return;
                    }
                }

                if (hbIntervalMinutes)
                {
                    if (*hbIntervalMinutes < 1 || *hbIntervalMinutes > 65535)
                    {
                        messages::propertyValueOutOfRange(
                            asyncResp->res, *hbIntervalMinutes,
                            "HeartbeatIntervalMinutes");
                        return;
                    }
                }

                if (context)
                {
                    subValue->userSub->customText = *context;
                }

                if (newHeaders)
                {
                    subValue->userSub->httpHeaders = std::move(*newHeaders);
                }

                if (retryPolicy)
                {
                    subValue->userSub->retryPolicy = *retryPolicy;
                }

                if (sendHeartbeat)
                {
                    subValue->userSub->sendHeartbeat = *sendHeartbeat;
                }
                if (hbIntervalMinutes)
                {
                    subValue->userSub->hbIntervalMinutes = *hbIntervalMinutes;
                }

                if (hbIntervalMinutes || sendHeartbeat)
                {
                    // if Heartbeat interval or send heart were changed, cancel
                    // the heartbeat timer if running and start a new heartbeat
                    // if needed
                    subValue->heartbeatParametersChanged();
                }

                if (verifyCertificate)
                {
                    subValue->userSub->verifyCertificate = *verifyCertificate;
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
            std::bind_front(handleEventDestinationDelete, std::ref(app)));
}

} // namespace redfish
