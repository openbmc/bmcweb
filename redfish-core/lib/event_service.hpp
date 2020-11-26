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
#include <sdbusplus/unpack_properties.hpp>
#include <utils/dbus_utils.hpp>

#include <charconv>
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

inline void
    doGetSnmpTrapClientdata(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& objectPath)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const dbus::utility::DBusPropertiesMap& propertiesList) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "D-Bus response error on GetSubTree " << ec;
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string* address = nullptr;
        const uint16_t* port = nullptr;

        const bool success = sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), propertiesList, "Address",
            address, "Port", port);

        if (!success)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        if (address != nullptr && port != nullptr)
        {
            std::string destination = "snmp://";
            destination.append(*address);
            destination.append(":");
            destination.append(std::to_string(*port));

            asyncResp->res.jsonValue["Destination"] = std::move(destination);
        }
        },
        "xyz.openbmc_project.Network.SNMP", objectPath,
        "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Network.Client");
}

inline void
    getSnmpTrapClientdata(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& id, const std::string& objectPath)
{
    asyncResp->res.jsonValue["@odata.type"] =
        "#EventDestination.v1_7_0.EventDestination";
    asyncResp->res.jsonValue["Protocol"] = "SNMPv2c";
    asyncResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "EventService", "Subscriptions", id);

    asyncResp->res.jsonValue["Id"] = id;
    asyncResp->res.jsonValue["Name"] = "Event Destination " + id;

    asyncResp->res.jsonValue["SubscriptionType"] = "SNMPTrap";
    asyncResp->res.jsonValue["EventFormatType"] = "Event";

    std::shared_ptr<Subscription> subValue =
        EventServiceManager::getInstance().getSubscription(id);
    if (subValue != nullptr)
    {
        asyncResp->res.jsonValue["Context"] = subValue->customText;
    }
    else
    {
        asyncResp->res.jsonValue["Context"] = "";
    }

    doGetSnmpTrapClientdata(asyncResp, objectPath);
}

inline void
    getSnmpTrapClient(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& id)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, id](const boost::system::error_code ec,
                        dbus::utility::ManagedObjectType& resp) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "D-Bus response error on GetManagedObjects "
                             << ec;
            messages::internalError(asyncResp->res);
            return;
        }

        for (const auto& objpath : resp)
        {
            sdbusplus::message::object_path path(objpath.first);
            const std::string snmpId = path.filename();
            if (snmpId.empty())
            {
                BMCWEB_LOG_ERROR << "The SNMP client ID is wrong";
                messages::internalError(asyncResp->res);
                return;
            }
            const std::string subscriptionId = "snmp" + snmpId;
            if (id != subscriptionId)
            {
                continue;
            }

            getSnmpTrapClientdata(asyncResp, id, objpath.first);
            return;
        }

        messages::resourceNotFound(asyncResp->res, "Subscriptions", id);
        EventServiceManager::getInstance().deleteSubscription(id);
        },
        "xyz.openbmc_project.Network.SNMP",
        "/xyz/openbmc_project/network/snmp/manager",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

inline void
    createSnmpTrapClient(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& host, const uint16_t& snmpTrapPort,
                         const std::shared_ptr<Subscription>& subValue)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, subValue](const boost::system::error_code ec,
                              const std::string& dbusSNMPid) {
        if (ec)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        sdbusplus::message::object_path path(dbusSNMPid);
        const std::string snmpId = path.filename();
        if (snmpId.empty())
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string subscriptionId = "snmp" + snmpId;

        std::string id = EventServiceManager::getInstance().addSubscription(
            subValue, subscriptionId);

        if (id.empty())
        {
            messages::internalError(asyncResp->res);
            return;
        }

        asyncResp->res.addHeader("Location",
                                 "/redfish/v1/EventService/Subscriptions/" +
                                     subscriptionId);
        messages::created(asyncResp->res);
        },
        "xyz.openbmc_project.Network.SNMP",
        "/xyz/openbmc_project/network/snmp/manager",
        "xyz.openbmc_project.Network.Client.Create", "Client", host,
        snmpTrapPort);
}
inline void
    checkSnmpTrapClient(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& host, const uint16_t& snmpTrapPort,
                        const std::string& destUrl,
                        const std::shared_ptr<Subscription>& subValue)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, host, snmpTrapPort, destUrl,
         subValue](const boost::system::error_code ec,
                   dbus::utility::ManagedObjectType& resp) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "D-Bus response error on GetManagedObjects "
                             << ec;
            messages::internalError(asyncResp->res);
            return;
        }

        for (const auto& object : resp)
        {
            for (const auto& interface : object.second)
            {
                if (interface.first == "xyz.openbmc_project.Network.Client")
                {
                    std::string address;
                    uint16_t portNum = 0;
                    for (const auto& property : interface.second)
                    {
                        if (property.first == "Address")
                        {
                            const std::string* value =
                                std::get_if<std::string>(&property.second);
                            if (value == nullptr)
                            {
                                continue;
                            }
                            address = *value;
                        }
                        else if (property.first == "Port")
                        {
                            const uint16_t* value =
                                std::get_if<uint16_t>(&property.second);
                            if (value == nullptr)
                            {
                                continue;
                            }
                            portNum = *value;
                        }
                    }

                    if (address == host && portNum == snmpTrapPort)
                    {
                        messages::resourceAlreadyExists(
                            asyncResp->res,
                            "EventDestination.v1_7_0.EventDestination",
                            "Destination", destUrl);
                        return;
                    }
                }
            }
        }
        // Create the snmp client
        createSnmpTrapClient(asyncResp, host, snmpTrapPort, subValue);
        },
        "xyz.openbmc_project.Network.SNMP",
        "/xyz/openbmc_project/network/snmp/manager",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

inline void
    getSnmpSubscriptionList(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& snmpId,
                            nlohmann::json& memberArray)
{
    const std::string subscriptionId = "snmp" + snmpId;

    nlohmann::json::object_t member;
    asyncResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "EventService", "Subscriptions", subscriptionId);
    memberArray.push_back(std::move(member));

    asyncResp->res.jsonValue["Members@odata.count"] = memberArray.size();
}

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
            // Supported range [30 - 180]
            if ((*retryInterval < 30) || (*retryInterval > 180))
            {
                messages::queryParameterOutOfRange(
                    asyncResp->res, std::to_string(*retryInterval),
                    "DeliveryRetryIntervalSeconds", "[30-180]");
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
    const boost::system::error_code ec,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    dbus::utility::ManagedObjectType& resp)
{
    nlohmann::json& memberArray = asyncResp->res.jsonValue["Members"];
    std::vector<std::string> subscripIds =
        EventServiceManager::getInstance().getAllIDs();
    memberArray = nlohmann::json::array();

    for (const std::string& id : subscripIds)
    {
        if (!boost::starts_with(id, "snmp"))
        {
            nlohmann::json::object_t member;
            member["@odata.id"] =
                "/redfish/v1/EventService/Subscriptions/" + id;
            memberArray.push_back(std::move(member));
        }
    }

    asyncResp->res.jsonValue["Members@odata.count"] = memberArray.size();

    if (ec)
    {
        if (ec.value() == EBADR)
        {
            // this is an optional process so just return if
            // it isn't there
            return;
        }

        BMCWEB_LOG_ERROR << "D-Bus response error on GetManagedObjects " << ec;
        messages::internalError(asyncResp->res);
        return;
    }

    for (const std::pair<sdbusplus::message::object_path,
                         dbus::utility::DBusInteracesMap>& objpath : resp)
    {
        sdbusplus::message::object_path path(objpath.first);
        const std::string snmpId = path.filename();
        if (snmpId.empty())
        {
            BMCWEB_LOG_ERROR << "The SNMP client ID is wrong";
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

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        dbus::utility::ManagedObjectType& resp) {
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

        if (regPrefixes && msgIds)
        {
            if (!regPrefixes->empty() && !msgIds->empty())
            {
                messages::propertyValueConflict(asyncResp->res, "MessageIds",
                                                "RegistryPrefixes");
                return;
            }
        }

        std::string host;
        std::string urlProto;
        uint16_t port = 0;
        std::string path;

        if (!crow::utility::validateAndSplitUrl(destUrl, urlProto, host, port,
                                                path))
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
        std::shared_ptr<Subscription> subValue =
            std::make_shared<Subscription>(host, port, path, urlProto);

        subValue->destinationUrl = destUrl;

        if (subscriptionType)
        {
            if (*subscriptionType != "RedfishEvent" &&
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
            if (protocol == "SNMPv2c")
            {
                subValue->subscriptionType = "SNMPTrap";
            }
            else
            {
                subValue->subscriptionType = "RedfishEvent"; // Default
            }
        }

        if ((protocol != "Redfish") && (protocol != "SNMPv2c"))
        {
            messages::propertyValueNotInList(asyncResp->res, protocol,
                                             "Protocol");
            return;
        }
        subValue->protocol = protocol;

        if (eventFormatType2)
        {
            if (std::find(supportedEvtFormatTypes.begin(),
                          supportedEvtFormatTypes.end(),
                          *eventFormatType2) == supportedEvtFormatTypes.end())
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

        if (protocol == "SNMPv2c")
        {
            // Check whether the client already exists
            checkSnmpTrapClient(asyncResp, host, port, destUrl, subValue);
            return;
        }

        // There is no need to set the default subscriptionId here.
        const std::string subscriptionId;

        std::string id = EventServiceManager::getInstance().addSubscription(
            subValue, subscriptionId);
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

        if (boost::starts_with(param, "snmp"))
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
            "#EventDestination.v1_7_0.EventDestination";
        asyncResp->res.jsonValue["Protocol"] = subValue->protocol;
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/EventService/Subscriptions/" + id;
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
                for (auto& it : headerChunk.items())
                {
                    const std::string* value =
                        it.value().get_ptr<const std::string*>();
                    if (value == nullptr)
                    {
                        messages::propertyValueFormatError(
                            asyncResp->res, it.value().dump(2, ' ', true),
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
                messages::propertyValueNotInList(asyncResp->res, *retryPolicy,
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
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }

        if (boost::starts_with(param, "snmp"))
        {
            std::string_view snmpTrapId = param;

            // Erase "snmp" in the request to find the corresponding
            // dbus snmp client id. For example, the snmpid in the
            // request is "snmp1", which will be "1" after being erased.
            snmpTrapId.remove_prefix(4);

            const std::string snmpPath =
                "/xyz/openbmc_project/network/snmp/manager/" +
                std::string(snmpTrapId);

            crow::connections::systemBus->async_method_call(
                [asyncResp, param](const boost::system::error_code ec) {
                if (ec)
                {
                    // The snmp trap id is incorrect
                    if (ec.value() == EBADR)
                    {
                        messages::resourceNotFound(asyncResp->res,
                                                   "Subscription", param);
                        return;
                    }
                    messages::internalError(asyncResp->res);
                    return;
                }
                messages::success(asyncResp->res);
                },
                "xyz.openbmc_project.Network.SNMP", snmpPath,
                "xyz.openbmc_project.Object.Delete", "Delete");

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
