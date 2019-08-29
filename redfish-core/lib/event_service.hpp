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

#include <variant>

namespace redfish
{

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
        res.jsonValue = {
            {"@odata.type", "#EventService.v1_3_0.EventService"},
            {"@odata.context",
             "/redfish/v1/$metadata#EventService.EventService"},
            {"Id", "EventService"},
            {"Name", "Event Service"},
            {"ServiceEnabled", true},
            {"DeliveryRetryAttempts", 3},
            {"DeliveryRetryIntervalSeconds", 30},
            {"RegistryPrefixes", {"Base", "OpenBMC"}},
            {"ResourceTypes", {"Task"}},
            {"EventFormatTypes", {"Event", "MetricReport"}},
            {"ServerSentEventUri",
             "/redfish/v1/EventService/Subscriptions/SSE"},
            {"Subscriptions",
             {{"@odata.id", "/redfish/v1/EventService/Subscriptions"}}},
            {"Actions",
             {{"#EventService.SubmitTestEvent",
               {{"target", "/redfish/v1/EventService/Actions/"
                           "EventService.SubmitTestEvent"}}}}},
            {"@odata.id", "/redfish/v1/EventService"}};
        res.end();
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
        res.jsonValue = {
            {"@odata.type",
             "#EventDestinationCollection.v1_0_0.EventDestinationCollection"},
            {"@odata.context",
             "/redfish/v1/"
             "$metadata#EventDestinationCollection.EventDestinationCollection"},
            {"Members@odata.count", 1},
            {"Members",
             {{{"@odata.id", "/redfish/v1/EventService/Subscriptions/1"}},
              {{"@odata.id", "/redfish/v1/EventService/Subscriptions/2"}}}},
            {"@odata.id", "/redfish/v1/EventService/Subscriptions"}};
        res.end();
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
        if (!((id == "1") || (id == "2")))
        {
            res.result(boost::beast::http::status::not_found);
            res.end();
            return;
        }
        res.jsonValue = {
            {"@odata.type", "#EventDestination.v1_5_0.EventDestination"},
            {"@odata.context",
             "/redfish/v1/$metadata#EventDestination.EventDestination"},
            {"Destination", "http://test.domain.com/EventListener"},
            {"SubscriptionType", "RedfishEvent"}, // HTTP Push style
            {"HttpHeaders",
             nlohmann::json::array({{{"X-Auth-Token", "XYZ12345678"}}})},
            {"Context", "CustomText"},
            {"DeliveryRetryPolicy", "TerminateAfterRetries"},
            {"Protocol", "Redfish"}};

        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/EventService/Subscriptions/" + id;
        asyncResp->res.jsonValue["Id"] = id;
        asyncResp->res.jsonValue["Name"] = "Event Subscription sample " + id;

        if (id == "1")
        {
            // Mockup for Event Log Subscription
            asyncResp->res.jsonValue["EventFormatType"] = "Event";

            asyncResp->res.jsonValue["RegistryPrefixes"] =
                nlohmann::json::array({"OpenBMC"});
            asyncResp->res.jsonValue["ResourceTypes"] =
                nlohmann::json::array({"Task"});
            asyncResp->res.jsonValue["MessageIds"] =
                nlohmann::json::array({"DCPowerOff", "InventoryAdded"});
            asyncResp->res.jsonValue["OriginResources"] = nlohmann::json::array(
                {{{"@odata.id", "/redfish/v1/Systems/system/Thermal"}}});
            asyncResp->res.jsonValue["SubordinateResources"] = true;
        }
        else if (id == "2")
        {
            // Mockup for MetricReport subscription
            asyncResp->res.jsonValue["EventFormatType"] = "MetricReport";

            asyncResp->res.jsonValue["MetricReportDefinitions"] =
                nlohmann::json::array(
                    {{{"@odata.id", "/redfish/v1/TelemetryService/"
                                    "MetricReportDefinitions/SampleMetric"}}});
        }
    }
};

} // namespace redfish
