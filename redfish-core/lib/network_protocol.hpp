/*
// Copyright (c) 2018 Intel Corporation
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

#include "error_messages.hpp"
#include "node.hpp"

#include <variant>

namespace redfish
{

enum NetworkProtocolUnitStructFields
{
    NET_PROTO_UNIT_NAME,
    NET_PROTO_UNIT_DESC,
    NET_PROTO_UNIT_LOAD_STATE,
    NET_PROTO_UNIT_ACTIVE_STATE,
    NET_PROTO_UNIT_SUB_STATE,
    NET_PROTO_UNIT_DEVICE,
    NET_PROTO_UNIT_OBJ_PATH,
    NET_PROTO_UNIT_ALWAYS_0,
    NET_PROTO_UNIT_ALWAYS_EMPTY,
    NET_PROTO_UNIT_ALWAYS_ROOT_PATH
};

enum NetworkProtocolListenResponseElements
{
    NET_PROTO_LISTEN_TYPE,
    NET_PROTO_LISTEN_STREAM
};

/**
 * @brief D-Bus Unit structure returned in array from ListUnits Method
 */
using UnitStruct =
    std::tuple<std::string, std::string, std::string, std::string, std::string,
               std::string, sdbusplus::message::object_path, uint32_t,
               std::string, sdbusplus::message::object_path>;

struct ServiceConfiguration
{
    const char* serviceName;
    const char* socketPath;
};

const static boost::container::flat_map<const char*, ServiceConfiguration>
    protocolToDBus{
        {"SSH",
         {"dropbear.service",
          "/org/freedesktop/systemd1/unit/dropbear_2esocket"}},
        {"HTTPS",
         {"phosphor-gevent.service",
          "/org/freedesktop/systemd1/unit/phosphor_2dgevent_2esocket"}},
        {"IPMI",
         {"phosphor-ipmi-net.service",
          "/org/freedesktop/systemd1/unit/phosphor_2dipmi_2dnet_2esocket"}}};

class NetworkProtocol : public Node
{
  public:
    NetworkProtocol(CrowApp& app) :
        Node(app, "/redfish/v1/Managers/bmc/NetworkProtocol")
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
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        getData(asyncResp);
    }

    std::string getHostName() const
    {
        std::string hostName;

        std::array<char, HOST_NAME_MAX> hostNameCStr;
        if (gethostname(hostNameCStr.data(), hostNameCStr.size()) == 0)
        {
            hostName = hostNameCStr.data();
        }
        return hostName;
    }

    void getData(const std::shared_ptr<AsyncResp>& asyncResp)
    {
        asyncResp->res.jsonValue["@odata.type"] =
            "#ManagerNetworkProtocol.v1_1_0.ManagerNetworkProtocol";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/bmc/NetworkProtocol";
        asyncResp->res.jsonValue["@odata.context"] =
            "/redfish/v1/"
            "$metadata#ManagerNetworkProtocol.ManagerNetworkProtocol";
        asyncResp->res.jsonValue["Id"] = "NetworkProtocol";
        asyncResp->res.jsonValue["Name"] = "Manager Network Protocol";
        asyncResp->res.jsonValue["Description"] = "Manager Network Service";
        asyncResp->res.jsonValue["Status"]["Health"] = "OK";
        asyncResp->res.jsonValue["Status"]["HealthRollup"] = "OK";
        asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

        for (auto& protocol : protocolToDBus)
        {
            asyncResp->res.jsonValue[protocol.first]["ProtocolEnabled"] = false;
        }

        asyncResp->res.jsonValue["HostName"] = getHostName();

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const std::vector<UnitStruct>& resp) {
                if (ec)
                {
                    asyncResp->res.jsonValue = nlohmann::json::object();
                    messages::internalError(asyncResp->res);
                    return;
                }

                for (auto& unit : resp)
                {
                    for (auto& kv : protocolToDBus)
                    {
                        if (kv.second.serviceName ==
                            std::get<NET_PROTO_UNIT_NAME>(unit))
                        {
                            continue;
                        }
                        const char* service = kv.first;
                        const char* socketPath = kv.second.socketPath;

                        asyncResp->res.jsonValue[service]["ProtocolEnabled"] =
                            std::get<NET_PROTO_UNIT_SUB_STATE>(unit) ==
                            "running";

                        crow::connections::systemBus->async_method_call(
                            [asyncResp, service{std::string(service)},
                             socketPath](
                                const boost::system::error_code ec,
                                const std::variant<std::vector<std::tuple<
                                    std::string, std::string>>>& resp) {
                                if (ec)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                const std::vector<
                                    std::tuple<std::string, std::string>>*
                                    responsePtr = std::get_if<std::vector<
                                        std::tuple<std::string, std::string>>>(
                                        &resp);
                                if (responsePtr == nullptr ||
                                    responsePtr->size() < 1)
                                {
                                    return;
                                }

                                const std::string& listenStream =
                                    std::get<NET_PROTO_LISTEN_STREAM>(
                                        (*responsePtr)[0]);
                                std::size_t lastColonPos =
                                    listenStream.rfind(":");
                                if (lastColonPos == std::string::npos)
                                {
                                    // Not a port
                                    return;
                                }
                                std::string portStr =
                                    listenStream.substr(lastColonPos + 1);
                                char* endPtr = nullptr;
                                // Use strtol instead of stroi to avoid
                                // exceptions
                                long port =
                                    std::strtol(portStr.c_str(), &endPtr, 10);

                                if (*endPtr != '\0' || portStr.empty())
                                {
                                    // Invalid value
                                    asyncResp->res.jsonValue[service]["Port"] =
                                        nullptr;
                                }
                                else
                                {
                                    // Everything OK
                                    asyncResp->res.jsonValue[service]["Port"] =
                                        port;
                                }
                            },
                            "org.freedesktop.systemd1", socketPath,
                            "org.freedesktop.DBus.Properties", "Get",
                            "org.freedesktop.systemd1.Socket", "Listen");
                    }
                }
            },
            "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
            "org.freedesktop.systemd1.Manager", "ListUnits");
    }
};

} // namespace redfish
