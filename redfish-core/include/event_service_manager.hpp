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
using ReadingsObjType =
    std::vector<std::tuple<std::string, std::string, double, std::string>>;

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
        host(inHost),
        port(inPort), path(inPath), uriProto(inUriProto)
    {
        conn = std::make_shared<crow::HttpClient>(
            crow::connections::systemBus->get_io_context(), host, port, path);

        reportSeqNum = 1;
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

    void filterAndSendReports(const std::string& id,
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
            {"Name", "Platform " + id + " usage metric report"},
            {"ReportSequence", std::to_string(reportSeqNum)},
            {"MetricReportDefinition", {{"@odata.id", metricReportDef}}},
            {"MetricValues", metricValuesArray}};

        this->sendEvent(msg.dump());
        this->reportSeqNum++;
    }

  private:
    std::string host;
    std::string port;
    std::string path;
    std::string uriProto;
    std::shared_ptr<crow::HttpClient> conn;
    uint64_t reportSeqNum;
};

class EventServiceManager
{
  private:
    EventServiceManager(const EventServiceManager&) = delete;
    EventServiceManager& operator=(const EventServiceManager&) = delete;
    EventServiceManager(EventServiceManager&&) = delete;
    EventServiceManager& operator=(EventServiceManager&&) = delete;

    EventServiceManager() : noOfMetricReportSubscribers(0)
    {
        // TODO: Read the persistent data from store and populate.
        // Populating with default.
        enabled = true;
        retryAttempts = 3;
        retryTimeoutInterval = 30; // seconds
    }

    int noOfMetricReportSubscribers;
    bool telemetryMonitorRunning;
    std::shared_ptr<sdbusplus::bus::match::match> matchTelemetryMonitor;
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
            BMCWEB_LOG_ERROR << "Failed to generate random number";
            return std::string("");
        }

        if (subValue->eventFormatType == "MetricReport")
        {
            // If it is first entry,  Register Metrics report signal.
            if (++noOfMetricReportSubscribers == 1)
            {
                registerMetricReportSignal();
            }
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
            std::shared_ptr<Subscription> entry = obj->second;
            if (entry->eventFormatType == "MetricReport")
            {
                if (--noOfMetricReportSubscribers == 0)
                {
                    unregisterMetricReportSignal();
                }
            }

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
                const std::variant<ReadingsObjType>& resp) {
                const ReadingsObjType* readingsPtr =
                    std::get_if<ReadingsObjType>(&resp);
                if (!readingsPtr)
                {
                    BMCWEB_LOG_DEBUG << "Failed to get metric readings";
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
                    if (entry->eventFormatType == "MetricReport")
                    {
                        entry->filterAndSendReports(idStr, *readingsPtr);
                    }
                }
            },
            service, objPath, "org.freedesktop.DBus.Properties", "Get", intf,
            "Readings");
    }

    void unregisterMetricReportSignal()
    {
        BMCWEB_LOG_DEBUG << "Metrics report signal - Unregister";
        matchTelemetryMonitor.reset();
        matchTelemetryMonitor = nullptr;
    }

    void registerMetricReportSignal()
    {
        if (matchTelemetryMonitor)
        {
            BMCWEB_LOG_DEBUG << "Metrics report signal - Already registered.";
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
};

} // namespace redfish
