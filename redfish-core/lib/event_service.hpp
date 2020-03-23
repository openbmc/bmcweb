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

#include <boost/container/flat_map.hpp>
#include <variant>

namespace redfish
{

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
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);

        asyncResp->res.jsonValue["@odata.type"] =
            "#EventDestination.v1_6_0.EventDestination";
        asyncResp->res.jsonValue["Name"] = "EventSubscription";
        asyncResp->res.jsonValue["SubscriptionType"] = "SNMPTrap";
        asyncResp->res.jsonValue["DeliveryRetryPolicy"] =
            "TerminateAfterRetries";
        asyncResp->res.jsonValue["SNMP"] = {"TrapCommunity", ""};
        asyncResp->res.jsonValue["Status"] = {"State", "Enable"};
        asyncResp->res.jsonValue["EventTypes"] = {"Alert"};
        asyncResp->res.jsonValue["Context"] = "WebUser3";
        asyncResp->res.jsonValue["Id"] = params[0];
        asyncResp->res.jsonValue["Actions"]["#EventDestination."
                                            "ResumeSubscription"] = {
            {"target", "/redfish/v1/EventService/Subscriptions/" + params[0] +
                           "/Actions/EventDestination.ResumeSubscription"}};
        asyncResp->res.jsonValue["OriginResources"] = {
            {"@odata.id",
             "/redfish/v1/EventService/Subscriptions/" + params[0]}};
        asyncResp->res.jsonValue["Protocol"] = "SNMPv2c";
        asyncResp->res.jsonValue["@odata.id"] = {
            "/redfish/v1/EventService/Subscriptions/" + params[0]};

        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const std::variant<std::string>& address) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                const std::string* s = std::get_if<std::string>(&address);
                asyncResp->res.jsonValue["Address"] = *s;
            },
            "xyz.openbmc_project.Network.SNMP",
            "/xyz/openbmc_project/network/snmp/manager/" + params[0],
            "org.freedesktop.DBus.Properties", "Get",
            "xyz.openbmc_project.Network.Client", "Address");

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const std::variant<uint16_t>& port) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }

                const uint16_t* p = std::get_if<uint16_t>(&port);
                asyncResp->res.jsonValue["Port"] = *p;
            },
            "xyz.openbmc_project.Network.SNMP",
            "/xyz/openbmc_project/network/snmp/manager/" + params[0],
            "org.freedesktop.DBus.Properties", "Get",
            "xyz.openbmc_project.Network.Client", "Port");
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

        std::optional<std::string> address;
        std::optional<uint16_t> port;
        if (!json_util::readJson(req, res, "Address", address, "Port", port))
        {
            return;
        }

        if (address)
        {
            crow::connections::systemBus->async_method_call(
                [asyncResp](const boost::system::error_code ec) {
                    if (ec)
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    messages::success(asyncResp->res);
                },
                "xyz.openbmc_project.Network.SNMP",
                "/xyz/openbmc_project/network/snmp/manager/" + params[0],
                "org.freedesktop.DBus.Properties", "Set",
                "xyz.openbmc_project.Network.Client", "Address",
                std::variant<std::string>(*address));
        }

        if (port)
        {
            crow::connections::systemBus->async_method_call(
                [asyncResp](const boost::system::error_code ec) {
                    if (ec)
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    messages::success(asyncResp->res);
                },
                "xyz.openbmc_project.Network.SNMP",
                "/xyz/openbmc_project/network/snmp/manager/" + params[0],
                "org.freedesktop.DBus.Properties", "Set",
                "xyz.openbmc_project.Network.Client", "Port",
                std::variant<uint16_t>(*port));
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

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                messages::success(asyncResp->res);
            },
            "xyz.openbmc_project.Network.SNMP",
            "/xyz/openbmc_project/network/snmp/manager/" + params[0],
            "xyz.openbmc_project.Object.Delete", "Delete");
    }
};

/**
 * EventDestinationCollection derived class for delivering eventService
 * Collection Schema
 */
class EventDestinationCollection : public Node
{
  public:
    EventDestinationCollection(CrowApp& app) :
        Node(app, "/redfish/v1/EventService/Subscriptions")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureUsers"}}},
            {boost::beast::http::verb::put, {{"ConfigureUsers"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureUsers"}}},
            {boost::beast::http::verb::post, {{"ConfigureUsers"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/EventService";
        asyncResp->res.jsonValue["@odata.type"] =
            "#EventDestinationCollection.EventDestinaDestinationCollection.";
        asyncResp->res.jsonValue["Name"] = "Event Destination Collection";

        crow::connections::systemBus->async_method_call(
            [asyncResp](
                const boost::system::error_code ec,
                const std::vector<std::pair<
                    std::string, std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>>&
                    objectPaths) {
                asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
                nlohmann::json& members = asyncResp->res.jsonValue["Members"];
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
                asyncResp->res.jsonValue["Members@odata.count"] = 0;

                for (auto& obj : objectPaths)
                {
                    // if can't parse snmp manager id then return
                    std::size_t idPos;
                    if ((idPos = obj.first.rfind("/")) == std::string::npos)
                    {
                        messages::internalError(asyncResp->res);
                        BMCWEB_LOG_DEBUG << "Can't parse snmp manager ID!!";
                        return;
                    }
                    std::string snmpId = obj.first.substr(idPos + 1);
                    members.push_back(
                        {{"@odata.id",
                          "/redfish/v1/EventService/Subscriptions/" + snmpId}});
                }
                asyncResp->res.jsonValue["Members@odata.count"] =
                    members.size();
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
            "/xyz/openbmc_project/network/snmp/manager", 0,
            std::array<const char*, 1>{"xyz.openbmc_project.Network.Client"});
    }

    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);

        std::string address;
        uint16_t port;
        if (!json_util::readJson(req, res, "Address", address, "Port", port))
        {
            return;
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp, address, port](const boost::system::error_code ec) {
                if (ec)
                {
                    // TODO Handle for specific error code
                    BMCWEB_LOG_ERROR << "doClearLog resp_handler got error "
                                     << ec;
                    asyncResp->res.result(
                        boost::beast::http::status::internal_server_error);
                    return;
                }
            },
            "xyz.openbmc_project.Network.SNMP",
            "/xyz/openbmc_project/network/snmp/manager",
            "xyz.openbmc_project.Network.Client.Create", "Client", address,
            port);
    }
};

/**
 * EventService derived class for delivering Event Service Schema.
 */
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
        res.jsonValue["@odata.type"] = "#EventService.v1_4_0.EventService";
        res.jsonValue["@odata.id"] = "/redfish/v1/EventService/";
        res.jsonValue["Id"] = "EventService";
        res.jsonValue["ServiceEnabled"] = true;
        res.jsonValue["DeliveryRetryAttempts"] = 3;
        res.jsonValue["EventTypesForSubscription"] = "Alert";
        res.jsonValue["Name"] = "Session Service";
        res.jsonValue["Description"] = "Session Service";
        res.jsonValue["Actions"]["#ComputerSystem.Reset"] = {
            {"target",
             "/redfish/v1/EventService/Actions/EventService.SubmitTestEvent"}};
        res.jsonValue["Subscriptions"] =
            "/redfish/v1/EventService/Subscriptions";

        res.end();
    }
};

} // namespace redfish