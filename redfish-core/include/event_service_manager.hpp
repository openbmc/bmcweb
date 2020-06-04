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

#include <boost/asio/io_context.hpp>
#include <boost/container/flat_map.hpp>
#include <error_messages.hpp>
#include <http_client.hpp>
#include <utils/json_utils.hpp>

#include <cstdlib>
#include <ctime>
#include <fstream>
#include <memory>
#include <variant>

namespace redfish
{

using ReadingsObjType =
    std::vector<std::tuple<std::string, std::string, double, std::string>>;
using EventServiceConfig = std::tuple<bool, uint32_t, uint32_t>;

static constexpr const char* eventFormatType = "Event";
static constexpr const char* metricReportFormatType = "MetricReport";

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
    std::vector<nlohmann::json> metricReportDefinitions;

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
    {}

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

    void filterAndSendReports(const std::string& id,
                              const std::string& readingsTs,
                              const ReadingsObjType& readings)
    {
        std::string metricReportDef =
            "/redfish/v1/TelemetryService/MetricReportDefinitions/" + id;

        // Empty list means no filter. Send everything.
        if (metricReportDefinitions.size())
        {
            if (std::find(metricReportDefinitions.begin(),
                          metricReportDefinitions.end(),
                          metricReportDef) == metricReportDefinitions.end())
            {
                return;
            }
        }

        nlohmann::json metricValuesArray = nlohmann::json::array();
        for (const auto& it : readings)
        {
            metricValuesArray.push_back({});
            nlohmann::json& entry = metricValuesArray.back();

            entry = {{"MetricId", std::get<0>(it)},
                     {"MetricProperty", std::get<1>(it)},
                     {"MetricValue", std::to_string(std::get<2>(it))},
                     {"Timestamp", std::get<3>(it)}};
        }

        nlohmann::json msg = {
            {"@odata.id", "/redfish/v1/TelemetryService/MetricReports/" + id},
            {"@odata.type", "#MetricReport.v1_3_0.MetricReport"},
            {"Id", id},
            {"Name", id},
            {"Timestamp", readingsTs},
            {"MetricReportDefinition", {{"@odata.id", metricReportDef}}},
            {"MetricValues", metricValuesArray}};

        this->sendEvent(msg.dump());
    }

  private:
    uint64_t eventSeqNum;
    std::string host;
    std::string port;
    std::string path;
    std::string uriProto;
    std::shared_ptr<crow::HttpClient> conn;
};

static constexpr const bool defaultEnabledState = true;
static constexpr const uint32_t defaultRetryAttempts = 3;
static constexpr const uint32_t defaultRetryInterval = 30;
static constexpr const char* defaulEventFormatType = "Event";
static constexpr const char* defaulSubscriptionType = "RedfishEvent";
static constexpr const char* defaulRetryPolicy = "TerminateAfterRetries";

class EventServiceManager
{
  private:
    bool serviceEnabled;
    uint32_t retryAttempts;
    uint32_t retryTimeoutInterval;

    EventServiceManager(const EventServiceManager&) = delete;
    EventServiceManager& operator=(const EventServiceManager&) = delete;
    EventServiceManager(EventServiceManager&&) = delete;
    EventServiceManager& operator=(EventServiceManager&&) = delete;

    EventServiceManager() :
        noOfEventLogSubscribers(0), noOfMetricReportSubscribers(0)
    {
        // Load config from persist store.
        initConfig();
    }

    size_t noOfEventLogSubscribers;
    size_t noOfMetricReportSubscribers;
    std::shared_ptr<sdbusplus::bus::match::match> matchTelemetryMonitor;
    boost::container::flat_map<std::string, std::shared_ptr<Subscription>>
        subscriptionsMap;

  public:
    static EventServiceManager& getInstance()
    {
        static EventServiceManager handler;
        return handler;
    }

    void loadDefaultConfig()
    {
        serviceEnabled = defaultEnabledState;
        retryAttempts = defaultRetryAttempts;
        retryTimeoutInterval = defaultRetryInterval;
    }

    void initConfig()
    {
        std::ifstream eventConfigFile(eventServiceFile);
        if (!eventConfigFile.good())
        {
            BMCWEB_LOG_DEBUG << "EventService config not exist";
            loadDefaultConfig();
            return;
        }
        auto jsonData = nlohmann::json::parse(eventConfigFile, nullptr, false);
        if (jsonData.is_discarded())
        {
            BMCWEB_LOG_ERROR << "EventService config parse error.";
            loadDefaultConfig();
            return;
        }

        nlohmann::json jsonConfig;
        if (json_util::getValueFromJsonObject(jsonData, "Configuration",
                                              jsonConfig))
        {
            if (!json_util::getValueFromJsonObject(jsonConfig, "ServiceEnabled",
                                                   serviceEnabled))
            {
                serviceEnabled = defaultEnabledState;
            }
            if (!json_util::getValueFromJsonObject(
                    jsonConfig, "DeliveryRetryAttempts", retryAttempts))
            {
                retryAttempts = defaultRetryAttempts;
            }
            if (!json_util::getValueFromJsonObject(
                    jsonConfig, "DeliveryRetryIntervalSeconds",
                    retryTimeoutInterval))
            {
                retryTimeoutInterval = defaultRetryInterval;
            }
        }
        else
        {
            loadDefaultConfig();
        }

        nlohmann::json subscriptionsList;
        if (!json_util::getValueFromJsonObject(jsonData, "Subscriptions",
                                               subscriptionsList))
        {
            BMCWEB_LOG_DEBUG << "EventService: Subscriptions not exist.";
            return;
        }

        for (nlohmann::json& jsonObj : subscriptionsList)
        {
            std::string protocol;
            if (!json_util::getValueFromJsonObject(jsonObj, "Protocol",
                                                   protocol))
            {
                BMCWEB_LOG_DEBUG << "Invalid subscription Protocol exist.";
                continue;
            }
            std::string destination;
            if (!json_util::getValueFromJsonObject(jsonObj, "Destination",
                                                   destination))
            {
                BMCWEB_LOG_DEBUG << "Invalid subscription destination exist.";
                continue;
            }
            std::string host;
            std::string urlProto;
            std::string port;
            std::string path;
            bool status =
                validateAndSplitUrl(destination, urlProto, host, port, path);

            if (!status)
            {
                BMCWEB_LOG_ERROR
                    << "Failed to validate and split destination url";
                continue;
            }
            std::shared_ptr<Subscription> subValue =
                std::make_shared<Subscription>(host, port, path, urlProto);

            subValue->destinationUrl = destination;
            subValue->protocol = protocol;
            if (!json_util::getValueFromJsonObject(
                    jsonObj, "DeliveryRetryPolicy", subValue->retryPolicy))
            {
                subValue->retryPolicy = defaulRetryPolicy;
            }
            if (!json_util::getValueFromJsonObject(jsonObj, "EventFormatType",
                                                   subValue->eventFormatType))
            {
                subValue->eventFormatType = defaulEventFormatType;
            }
            if (!json_util::getValueFromJsonObject(jsonObj, "SubscriptionType",
                                                   subValue->subscriptionType))
            {
                subValue->subscriptionType = defaulSubscriptionType;
            }

            json_util::getValueFromJsonObject(jsonObj, "Context",
                                              subValue->customText);
            json_util::getValueFromJsonObject(jsonObj, "MessageIds",
                                              subValue->registryMsgIds);
            json_util::getValueFromJsonObject(jsonObj, "RegistryPrefixes",
                                              subValue->registryPrefixes);
            json_util::getValueFromJsonObject(jsonObj, "HttpHeaders",
                                              subValue->httpHeaders);
            json_util::getValueFromJsonObject(
                jsonObj, "MetricReportDefinitions",
                subValue->metricReportDefinitions);

            std::string id = addSubscription(subValue, false);
            if (id.empty())
            {
                BMCWEB_LOG_ERROR << "Failed to add subscription";
            }
        }
        return;
    }

    void updateSubscriptionData()
    {
        // Persist the config and subscription data.
        nlohmann::json jsonData;

        nlohmann::json& configObj = jsonData["Configuration"];
        configObj["ServiceEnabled"] = serviceEnabled;
        configObj["DeliveryRetryAttempts"] = retryAttempts;
        configObj["DeliveryRetryIntervalSeconds"] = retryTimeoutInterval;

        nlohmann::json& subListArray = jsonData["Subscriptions"];
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
            entry["MetricReportDefinitions"] =
                subValue->metricReportDefinitions;

            subListArray.push_back(entry);
        }

        const std::string tmpFile(std::string(eventServiceFile) + "_tmp");
        std::ofstream ofs(tmpFile, std::ios::out);
        const auto& writeData = jsonData.dump();
        ofs << writeData;
        ofs.close();

        BMCWEB_LOG_DEBUG << "EventService config updated to file.";
        if (std::rename(tmpFile.c_str(), eventServiceFile) != 0)
        {
            BMCWEB_LOG_ERROR << "Error in renaming temporary file: "
                             << tmpFile.c_str();
        }
    }

    EventServiceConfig getEventServiceConfig()
    {
        return {serviceEnabled, retryAttempts, retryTimeoutInterval};
    }

    void setEventServiceConfig(const EventServiceConfig& cfg)
    {
        bool updateConfig = false;

        if (serviceEnabled != std::get<0>(cfg))
        {
            serviceEnabled = std::get<0>(cfg);
            if (serviceEnabled && noOfMetricReportSubscribers)
            {
                registerMetricReportSignal();
            }
            else
            {
                unregisterMetricReportSignal();
            }
            updateConfig = true;
        }

        if (retryAttempts != std::get<1>(cfg))
        {
            retryAttempts = std::get<1>(cfg);
            updateConfig = true;
        }

        if (retryTimeoutInterval != std::get<2>(cfg))
        {
            retryTimeoutInterval = std::get<2>(cfg);
            updateConfig = true;
        }

        if (updateConfig)
        {
            updateSubscriptionData();
        }
    }

    void updateNoOfSubscribersCount()
    {
        size_t eventLogSubCount = 0;
        size_t metricReportSubCount = 0;
        for (const auto& it : subscriptionsMap)
        {
            std::shared_ptr<Subscription> entry = it.second;
            if (entry->eventFormatType == eventFormatType)
            {
                eventLogSubCount++;
            }
            else if (entry->eventFormatType == metricReportFormatType)
            {
                metricReportSubCount++;
            }
        }

        noOfEventLogSubscribers = eventLogSubCount;
        if (noOfMetricReportSubscribers != metricReportSubCount)
        {
            noOfMetricReportSubscribers = metricReportSubCount;
            if (noOfMetricReportSubscribers)
            {
                registerMetricReportSignal();
            }
            else
            {
                unregisterMetricReportSignal();
            }
        }
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

    std::string addSubscription(const std::shared_ptr<Subscription> subValue,
                                const bool updateFile = true)
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

        updateNoOfSubscribersCount();

        if (updateFile)
        {
            updateSubscriptionData();
        }
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
            updateNoOfSubscribersCount();
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

    void getMetricReading(const std::string& service,
                          const std::string& objPath, const std::string& intf)
    {
        std::size_t found = objPath.find_last_of("/");
        if (found == std::string::npos)
        {
            BMCWEB_LOG_DEBUG << "Invalid objPath received";
            return;
        }

        std::string idStr = objPath.substr(found + 1);
        if (idStr.empty())
        {
            BMCWEB_LOG_DEBUG << "Invalid ID in objPath";
            return;
        }

        crow::connections::systemBus->async_method_call(
            [idStr{std::move(idStr)}](
                const boost::system::error_code ec,
                boost::container::flat_map<
                    std::string, std::variant<std::string, ReadingsObjType>>&
                    resp) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG
                        << "D-Bus call failed to GetAll metric readings.";
                    return;
                }

                const std::string* timestampPtr =
                    std::get_if<std::string>(&resp["Timestamp"]);
                if (!timestampPtr)
                {
                    BMCWEB_LOG_DEBUG << "Failed to Get timestamp.";
                    return;
                }

                ReadingsObjType* readingsPtr =
                    std::get_if<ReadingsObjType>(&resp["Readings"]);
                if (!readingsPtr)
                {
                    BMCWEB_LOG_DEBUG << "Failed to Get Readings property.";
                    return;
                }

                if (!readingsPtr->size())
                {
                    BMCWEB_LOG_DEBUG << "No metrics report to be transferred";
                    return;
                }

                for (const auto& it :
                     EventServiceManager::getInstance().subscriptionsMap)
                {
                    std::shared_ptr<Subscription> entry = it.second;
                    if (entry->eventFormatType == metricReportFormatType)
                    {
                        entry->filterAndSendReports(idStr, *timestampPtr,
                                                    *readingsPtr);
                    }
                }
            },
            service, objPath, "org.freedesktop.DBus.Properties", "GetAll",
            intf);
    }

    void unregisterMetricReportSignal()
    {
        if (matchTelemetryMonitor)
        {
            BMCWEB_LOG_DEBUG << "Metrics report signal - Unregister";
            matchTelemetryMonitor.reset();
            matchTelemetryMonitor = nullptr;
        }
    }

    void registerMetricReportSignal()
    {
        if (!serviceEnabled || matchTelemetryMonitor)
        {
            BMCWEB_LOG_DEBUG << "Not registering metric report signal.";
            return;
        }

        BMCWEB_LOG_DEBUG << "Metrics report signal - Register";
        std::string matchStr(
            "type='signal',member='ReportUpdate', "
            "interface='xyz.openbmc_project.MonitoringService.Report'");

        matchTelemetryMonitor = std::make_shared<sdbusplus::bus::match::match>(
            *crow::connections::systemBus, matchStr,
            [this](sdbusplus::message::message& msg) {
                if (msg.is_method_error())
                {
                    BMCWEB_LOG_ERROR << "TelemetryMonitor Signal error";
                    return;
                }

                std::string service = msg.get_sender();
                std::string objPath = msg.get_path();
                std::string intf = msg.get_interface();
                getMetricReading(service, objPath, intf);
            });
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
        if (urlProto == "http")
        {
#ifndef BMCWEB_INSECURE_ENABLE_HTTP_PUSH_STYLE_EVENTING
            return false;
#endif
        }

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
