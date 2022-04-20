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

#include <app.hpp>
#include <boost/beast/http/fields.hpp>
#include <http/utility.hpp>
#include <logging.hpp>
#include <query.hpp>
#include <registries/privilege_registry.hpp>

#include <span>

namespace redfish
{

static constexpr const std::array<const char*, 2> supportedEvtFormatTypes = {
    eventFormatType, metricReportFormatType};
static constexpr const std::array<const char*, 3> supportedRegPrefixes = {
    "Base", "OpenBMC", "TaskEvent"};
static constexpr const std::array<const char*, 3> supportedRetryPolicies = {
    "TerminateAfterRetries", "SuspendRetries", "RetryForever"};

#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
static constexpr const std::array<const char*, 2> supportedResourceTypes = {
    "IBMConfigFile", "Task"};
#else
static constexpr const std::array<const char*, 1> supportedResourceTypes = {
    "Task"};
#endif

static constexpr const uint8_t maxNoOfSubscriptions = 20;

using VariantMap =
    boost::container::flat_map<std::string, dbus::utility::DbusVariantType>;
using ManagedObjectsVectorType =
    std::vector<std::pair<sdbusplus::message::object_path,
                          boost::container::flat_map<std::string, VariantMap>>>;

inline void addSubscriptionDBusSNMPClient(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& host, const std::uint16_t& port)
{

    auto respHandler = [asyncResp](const boost::system::error_code ec) mutable {
        if (ec)
        {
            BMCWEB_LOG_ERROR
                << "addSubscriptionDBusSNMPClient respHandler DBUS error: "
                << ec;
            messages::internalError(asyncResp->res);
            return;
        }
        messages::created(asyncResp->res);
    };

    crow::connections::systemBus->async_method_call(
        respHandler, "xyz.openbmc_project.Network.SNMP",
        "/xyz/openbmc_project/network/snmp/manager",
        "xyz.openbmc_project.Network.Client.Create", "Client", host, port);
}

inline void getSNMPTrapData(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& id)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, id](const boost::system::error_code ec,
                        ManagedObjectsVectorType& subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error" << ec;
                messages::internalError(asyncResp->res);
                return;
            }

            std::string idNum = id.substr(id.find_first_of("0123456789"));
            std::string dbusNum;

            for (const auto& object : subtree)
            {
                nlohmann::json item;
                const std::string& path =
                    static_cast<const std::string&>(object.first);
                dbusNum = path.substr(path.find_first_of("0123456789"));
                if (dbusNum == idNum)
                {

                    asyncResp->res.jsonValue = {
                        {"@odata.type",
                         "#EventDestination.v1_7_0.EventDestination"},
                        {"Protocol", "SNMPv2c"}};

                    asyncResp->res.jsonValue["@odata.id"] =
                        "/redfish/v1/EventService/Subscriptions/" + id;

                    asyncResp->res.jsonValue["Id"] = id;

                    asyncResp->res.jsonValue["Name"] =
                        "Event Destination " + id;

                    auto& clientInterface =
                        object.second.at("xyz.openbmc_project.Network.Client");

                    asyncResp->res.jsonValue["SubscriptionType"] = "SNMPTrap";

                    asyncResp->res.jsonValue["Context"] = "";

                    asyncResp->res.jsonValue["Destination"] =
                        "snmp://" +
                        std::get<std::string>(clientInterface.at("Address")) +
                        ":" +
                        std::to_string(
                            std::get<uint16_t>(clientInterface.at("Port")));
                }
            }
        },
        "xyz.openbmc_project.Network.SNMP",
        "/xyz/openbmc_project/network/snmp/manager",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

inline void getSNMPTraps(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    ManagedObjectsVectorType& subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error" << ec;
                messages::internalError(asyncResp->res);
                return;
            }

            for (const auto& object : subtree)
            {
                nlohmann::json item;
                const std::string& path =
                    static_cast<const std::string&>(object.first);
                std::string dbusNum =
                    path.substr(path.find_first_of("0123456789"));

                (asyncResp->res.jsonValue["Members"])
                    .push_back(
                        {{"@odata.id",
                          "/redfish/v1/EventService/Subscriptions/SNMP-" +
                              dbusNum}});
            }
            asyncResp->res.jsonValue["Members@odata.count"] =
                asyncResp->res.jsonValue["Members"].size();
        },
        "xyz.openbmc_project.Network.SNMP",
        "/xyz/openbmc_project/network/snmp/manager",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

inline void deleteSNMPTrap(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& id)
{

    auto respHandler = [asyncResp](const boost::system::error_code ec) mutable {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "deleteSNMPTrap respHandler DBUS error: " << ec;
            messages::internalError(asyncResp->res);
            return;
        }
    };
    size_t last_index = id.find_last_not_of("0123456789");
    std::string dbusId = id.substr(last_index + 1);

    crow::connections::systemBus->async_method_call(
        respHandler, "xyz.openbmc_project.Network.SNMP",
        "/xyz/openbmc_project/network/snmp/manager/" + dbusId,
        "xyz.openbmc_project.Object.Delete", "Delete");
}

inline void requestRoutesEventService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/EventService/")
        .privileges(redfish::privileges::getEventService)
        .methods(boost::beast::http::verb::get)([&app](const crow::Request& req,
                                                       const std::shared_ptr<
                                                           bmcweb::AsyncResp>&
                                                           asyncResp) {
            if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
            {
                return;
            }
            asyncResp->res.jsonValue = {
                {"@odata.type", "#EventService.v1_5_0.EventService"},
                {"Id", "EventService"},
                {"Name", "Event Service"},
                {"Subscriptions",
                 {{"@odata.id", "/redfish/v1/EventService/Subscriptions"}}},
                {"Actions",
                 {{"#EventService.SubmitTestEvent",
                   {{"target",
                     "/redfish/v1/EventService/Actions/EventService.SubmitTestEvent"}}}}},
                {"@odata.id", "/redfish/v1/EventService"}};

            const persistent_data::EventServiceConfig eventServiceConfig =
                persistent_data::EventServiceStore::getInstance()
                    .getEventServiceConfig();

            asyncResp->res.jsonValue["Status"]["State"] =
                (eventServiceConfig.enabled ? "Enabled" : "Disabled");
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

            nlohmann::json supportedSSEFilters = {
                {"EventFormatType", true},        {"MessageId", true},
                {"MetricReportDefinition", true}, {"RegistryPrefix", true},
                {"OriginResource", false},        {"ResourceType", false}};

            asyncResp->res.jsonValue["SSEFilterPropertiesSupported"] =
                supportedSSEFilters;
        });

    BMCWEB_ROUTE(app, "/redfish/v1/EventService/")
        .privileges(redfish::privileges::patchEventService)
        .methods(boost::beast::http::verb::patch)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
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
                    // Supported range [30 - 180]
                    if ((*retryInterval < 30) || (*retryInterval > 180))
                    {
                        messages::queryParameterOutOfRange(
                            asyncResp->res, std::to_string(*retryInterval),
                            "DeliveryRetryIntervalSeconds", "[30-180]");
                    }
                    else
                    {
                        eventServiceConfig.retryTimeoutInterval =
                            *retryInterval;
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
                if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
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

inline void requestRoutesEventDestinationCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/EventService/Subscriptions/")
        .privileges(redfish::privileges::getEventDestinationCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
                {
                    return;
                }
                asyncResp->res.jsonValue = {
                    {"@odata.type",
                     "#EventDestinationCollection.EventDestinationCollection"},
                    {"@odata.id", "/redfish/v1/EventService/Subscriptions"},
                    {"Name", "Event Destination Collections"}};

                nlohmann::json& memberArray =
                    asyncResp->res.jsonValue["Members"];

                std::vector<std::string> subscripIds =
                    EventServiceManager::getInstance().getAllIDs();
                memberArray = nlohmann::json::array();
                asyncResp->res.jsonValue["Members@odata.count"] =
                    subscripIds.size();

                for (const std::string& id : subscripIds)
                {
                    memberArray.push_back(
                        {{"@odata.id",
                          "/redfish/v1/EventService/Subscriptions/" + id}});
                }
                getSNMPTraps(asyncResp);
            });
    BMCWEB_ROUTE(app, "/redfish/v1/EventService/Subscriptions/")
        .privileges(redfish::privileges::postEventDestinationCollection)
        .methods(
            boost::beast::http::verb::
                post)([&app](
                          const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
            if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
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
                    req, asyncResp->res, "Destination", destUrl, "Context",
                    context, "Protocol", protocol, "SubscriptionType",
                    subscriptionType, "EventFormatType", eventFormatType2,
                    "HttpHeaders", headers, "RegistryPrefixes", regPrefixes,
                    "MessageIds", msgIds, "DeliveryRetryPolicy", retryPolicy,
                    "MetricReportDefinitions", mrdJsonArray, "ResourceTypes",
                    resTypes))
            {
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

            std::string host;
            std::string urlProto;
            uint16_t port = 0;
            std::string path;

            if (!crow::utility::validateAndSplitUrl(destUrl, urlProto, host,
                                                    port, path))
            {
                BMCWEB_LOG_WARNING
                    << "Failed to validate and split destination url";
                messages::propertyValueFormatError(asyncResp->res, destUrl,
                                                   "Destination");
                return;
            }

            if (path.empty())
            {
                path = "/";
            }

            if ((protocol != "Redfish") && (protocol != "SNMPv2c"))
            {
                messages::propertyValueNotInList(asyncResp->res, protocol,
                                                 "Protocol");
                return;
            }

            if (((protocol == "Redfish") &&
                 !((urlProto == "http") || (urlProto == "https"))) ||
                (((protocol == "SNMPv2c") && !(urlProto == "snmp"))))
            {
                messages::internalError(asyncResp->res);
                return;
            }

            std::shared_ptr<Subscription> subValue =
                std::make_shared<Subscription>(host, port, path, urlProto);

            if (urlProto == "snmp")
            {
                addSubscriptionDBusSNMPClient(asyncResp, host, port);
            }

            subValue->destinationUrl = destUrl;

            if (subscriptionType)
            {
                if ((*subscriptionType != "RedfishEvent") &&
                    (*subscriptionType != "SNMPTrap"))
                {
                    messages::propertyValueNotInList(
                        asyncResp->res, *subscriptionType, "SubscriptionType");
                    return;
                }
                subValue->subscriptionType = *subscriptionType;
            }
            else
            {
                if ((urlProto == "http") || (urlProto == "https"))
                {
                    subValue->subscriptionType = "RedfishEvent";
                }
                else if (urlProto == "snmp")
                {
                    subValue->subscriptionType = "SNMPTrap";
                }
            }

            subValue->protocol = protocol;

            if (eventFormatType2)
            {
                if (std::find(supportedEvtFormatTypes.begin(),
                              supportedEvtFormatTypes.end(),
                              *eventFormatType2) ==
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
                subValue->customText = *context;
            }

            if (headers)
            {
                for (const nlohmann::json& headerChunk : *headers)
                {
                    for (const auto& item : headerChunk.items())
                    {
                        const std::string* value =
                            item.value().get_ptr<const std::string*>();
                        if (value == nullptr)
                        {
                            messages::propertyValueFormatError(
                                asyncResp->res, item.value().dump(2, 1),
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

            if (resTypes)
            {
                for (const std::string& it : *resTypes)
                {
                    if (std::find(supportedResourceTypes.begin(),
                                  supportedResourceTypes.end(),
                                  it) == supportedResourceTypes.end())
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

                        if (std::any_of(
                                registry.begin(), registry.end(),
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
                if (std::find(supportedRetryPolicies.begin(),
                              supportedRetryPolicies.end(),
                              *retryPolicy) == supportedRetryPolicies.end())
                {
                    messages::propertyValueNotInList(
                        asyncResp->res, *retryPolicy, "DeliveryRetryPolicy");
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

                    if (!json_util::readJson(mrdObj, asyncResp->res,
                                             "@odata.id", mrdUri))

                    {
                        return;
                    }
                    subValue->metricReportDefinitions.emplace_back(mrdUri);
                }
            }
            if ((urlProto == "http") || (urlProto == "https"))
            {
                std::string id =
                    EventServiceManager::getInstance().addSubscription(
                        subValue);
                if (id.empty())
                {
                    messages::internalError(asyncResp->res);
                    return;
                }

                messages::created(asyncResp->res);
                asyncResp->res.addHeader(
                    "Location", "/redfish/v1/EventService/Subscriptions/" + id);
            }
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
                if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
                {
                    return;
                }
                if (param.find("SNMP") != std::string::npos)
                {
                    getSNMPTrapData(asyncResp, param);
                }
                else
                {
                    std::shared_ptr<Subscription> subValue =
                        EventServiceManager::getInstance().getSubscription(
                            param);
                    if (subValue == nullptr)
                    {
                        asyncResp->res.result(
                            boost::beast::http::status::not_found);
                        return;
                    }
                    const std::string& id = param;

                    asyncResp->res.jsonValue = {
                        {"@odata.type",
                         "#EventDestination.v1_7_0.EventDestination"},
                        {"Protocol", "Redfish"}};
                    asyncResp->res.jsonValue["@odata.id"] =
                        "/redfish/v1/EventService/Subscriptions/" + id;
                    asyncResp->res.jsonValue["Id"] = id;
                    asyncResp->res.jsonValue["Name"] =
                        "Event Destination " + id;
                    asyncResp->res.jsonValue["Destination"] =
                        subValue->destinationUrl;
                    asyncResp->res.jsonValue["Context"] = subValue->customText;
                    asyncResp->res.jsonValue["SubscriptionType"] =
                        subValue->subscriptionType;
                    asyncResp->res.jsonValue["HttpHeaders"] =
                        nlohmann::json::array();
                    asyncResp->res.jsonValue["EventFormatType"] =
                        subValue->eventFormatType;
                    asyncResp->res.jsonValue["RegistryPrefixes"] =
                        subValue->registryPrefixes;
                    asyncResp->res.jsonValue["ResourceTypes"] =
                        subValue->resourceTypes;

                    asyncResp->res.jsonValue["MessageIds"] =
                        subValue->registryMsgIds;
                    asyncResp->res.jsonValue["DeliveryRetryPolicy"] =
                        subValue->retryPolicy;

                    std::vector<nlohmann::json> mrdJsonArray;
                    for (const auto& mdrUri : subValue->metricReportDefinitions)
                    {
                        mrdJsonArray.push_back({{"@odata.id", mdrUri}});
                    }
                    asyncResp->res.jsonValue["MetricReportDefinitions"] =
                        mrdJsonArray;
                }
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
                if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
                {
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

                std::optional<std::string> context;
                std::optional<std::string> retryPolicy;
                std::optional<std::vector<nlohmann::json>> headers;

                if (!json_util::readJsonPatch(req, asyncResp->res, "Context",
                                              context, "DeliveryRetryPolicy",
                                              retryPolicy, "HttpHeaders",
                                              headers))
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
                        for (auto& it : headerChunk.items())
                        {
                            const std::string* value =
                                it.value().get_ptr<const std::string*>();
                            if (value == nullptr)
                            {
                                messages::propertyValueFormatError(
                                    asyncResp->res,
                                    it.value().dump(2, ' ', true),
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
                    if (std::find(supportedRetryPolicies.begin(),
                                  supportedRetryPolicies.end(),
                                  *retryPolicy) == supportedRetryPolicies.end())
                    {
                        messages::propertyValueNotInList(asyncResp->res,
                                                         *retryPolicy,
                                                         "DeliveryRetryPolicy");
                        return;
                    }
                    subValue->retryPolicy = *retryPolicy;
                    subValue->updateRetryPolicy();
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
                if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
                {
                    return;
                }
                if (param.find("SNMP") != std::string::npos)
                {
                    deleteSNMPTrap(asyncResp, param);
                }
                else
                {
                    if (!EventServiceManager::getInstance().isSubscriptionExist(
                            param))
                    {
                        asyncResp->res.result(
                            boost::beast::http::status::not_found);
                        return;
                    }
                    EventServiceManager::getInstance().deleteSubscription(
                        param);
                }
            });
}

} // namespace redfish
