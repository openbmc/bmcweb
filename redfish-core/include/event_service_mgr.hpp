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
    std::vector<std::string> resourceTypes;
    std::vector<nlohmann::json> httpHeaders; // key-value pair
    std::vector<nlohmann::json> originResources;
    std::vector<nlohmann::json> metricReportDefinitions;

    Subscription(const std::string& _host, const std::string& _port,
                 const std::string& _path, const std::string& _uriProto) :
        host(_host),
        port(_port), path(_path), uriProto(_uriProto)
    {
        conn = std::make_shared<HttpClient>(
            crow::connections::systemBus->get_io_context(), host, port);
    }
    ~Subscription()
    {
    }

    void startConnection()
    {
        conn->openConnection();

        HttpReqHeadersType reqHeaders;
        for (auto& header : httpHeaders)
        {
            for (const auto& item : header.items())
            {
                std::string key = item.key();
                std::string val = item.value();
                reqHeaders.emplace_back(std::pair(key, val));
            }
        }
        conn->setHeaders(reqHeaders);
    }
    void sendEvent(const std::string& msg)
    {
        ConnState state = conn->getState();
        if (state != ConnState::connected)
        {
            startConnection();
        }
        state = conn->getState();
        if (state == ConnState::connected)
        {
            conn->doWrite(path, msg);
        }
        else
        {
            BMCWEB_LOG_DEBUG << "Connection closed";
        }
    }

    void telemetryFormatter(sdbusplus::message::message m)
    {
        std::string intfName;
        std::map<std::string,
                 std::variant<std::vector<std::tuple<std::string, std::string,
                                                     double, std::string>>>>
            properties;
        m.read(intfName, properties);

        const auto propNode = properties.find("Readings");
        if (propNode != properties.end())
        {
            std::string path = m.get_path();
            std::size_t found = path.find_last_of("/");
            std::string idStr = path.substr(found + 1);

            const std::vector<std::tuple<std::string, std::string, double,
                                         std::string>>* propsPtr =
                std::get_if<std::vector<
                    std::tuple<std::string, std::string, double, std::string>>>(
                    &propNode->second);
            nlohmann::json metricValuesArray = nlohmann::json::array();
            if (propsPtr)
            {
                for (auto& it : *propsPtr)
                {
                    metricValuesArray.push_back({});
                    nlohmann::json& entry = metricValuesArray.back();

                    entry = {{"MetricId", std::get<0>(it)},
                             {"MetricProperty", std::get<1>(it)},
                             {"MetricValue", std::to_string(std::get<2>(it))},
                             {"Timestamp", std::get<3>(it)}};
                }
            }
            nlohmann::json msg = {
                {"@odata.id",
                 "/redfish/v1/TelemetryService/MetricReports/" + idStr},
                {"@odata.type", "#MetricReport.v1_0_0.MetricReport"},
                {"Id", idStr},
                {"Name", "Platform " + idStr + " usage metric report"},
                {"ReportSequence", "0"},
                {"MetricReportDefinition",
                 {{"@odata.id", "/redfish/v1/TelemetryService/"
                                "MetricReportDefinitions/" +
                                    idStr}}},
                {"MetricValues", metricValuesArray}};

            sendEvent(msg.dump());
        }
    }

  private:
    std::string host;
    std::string port;
    std::string path;
    std::string uriProto;
    std::shared_ptr<HttpClient> conn;
};

class EventSrvManager
{
  public:
    bool enabled;
    uint32_t retryAttempts;
    uint32_t retryTimeoutInterval;

    EventSrvManager() : telemetryMonitorRunning(false)
    {
        // TODO: Read the persistent data from store and populate
        // Poulating with default.
        enabled = true;
        retryAttempts = 3;
        retryTimeoutInterval = 30; // seconds
    }
    ~EventSrvManager()
    {
    }

    std::shared_ptr<Subscription> getSubscription(const std::string& id)
    {
        auto obj = subscriptionsList.find(id);
        if (obj == subscriptionsList.end())
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
        std::string id = std::to_string(std::rand());
        subscriptionsList.insert(std::pair(id, subValue));

        if (subValue->eventFormatType == "MetricReport")
        {
            metricSubscribers.insert(std::pair(id, subValue));
            if (!metricSubscribers.empty() && !telemetryMonitorRunning)
            {
                startTelemetryMonitor();
            }
        }

        updateSubscriptionData();
        return id;
    }
    bool isSubscriptionExist(const std::string& id)
    {
        auto obj = subscriptionsList.find(id);
        if (obj == subscriptionsList.end())
        {
            return false;
        }
        return true;
    }
    void deleteSubscription(const std::string& id)
    {
        auto obj = subscriptionsList.find(id);
        if (obj != subscriptionsList.end())
        {
            subscriptionsList.erase(obj);
            updateSubscriptionData();
        }

        auto metricObj = metricSubscribers.find(id);
        if (metricObj != metricSubscribers.end())
        {
            metricSubscribers.erase(metricObj);
            if (metricSubscribers.empty() && telemetryMonitorRunning)
            {
                stopTelemetryMonitor();
            }
        }
    }
    std::vector<std::string> getAllIDs()
    {
        std::vector<std::string> idList;
        for (auto& it : subscriptionsList)
        {
            idList.emplace_back(it.first);
        }
        return idList;
    }
    bool isDestinationExist(const std::string& destUrl)
    {
        for (auto& it : subscriptionsList)
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

  private:
    void updateSubscriptionData()
    {
        // Persist the config and subscription data.
        // TODO: subscriptionsList & configData need to be
        // written to Persist store.
        return;
    }

    void stopTelemetryMonitor()
    {
        BMCWEB_LOG_DEBUG << "stopTelemetryMonitor()";
        telemetryMonitorRunning = false;
        matchTelemetryMonitor.reset();
    }
    void startTelemetryMonitor()
    {
        BMCWEB_LOG_DEBUG << "startTelemetryMonitor()";
        telemetryMonitorRunning = true;
        std::string matchStr(
            "type='signal',member='PropertiesChanged', "
            "interface='org.freedesktop.DBus.Properties', "
            "arg0namespace='xyz.openbmc_project.MonitoringService.Report'");

        matchTelemetryMonitor = std::make_shared<sdbusplus::bus::match::match>(
            *crow::connections::systemBus, matchStr,
            [&](sdbusplus::message::message& m) {
                if (m.is_method_error())
                {
                    BMCWEB_LOG_ERROR << "TelemetryMonitor Signal error";
                    return;
                }
                for (auto& it : metricSubscribers)
                {
                    auto subscriber = it.second;
                    subscriber->telemetryFormatter(m);
                }
            });
    }

    std::shared_ptr<sdbusplus::bus::match::match> matchTelemetryMonitor;
    bool telemetryMonitorRunning;
    boost::container::flat_map<std::string, std::shared_ptr<Subscription>>
        subscriptionsList;
    boost::container::flat_map<std::string, std::shared_ptr<Subscription>>
        metricSubscribers;
};

} // namespace redfish
