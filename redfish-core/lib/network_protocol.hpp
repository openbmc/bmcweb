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

namespace redfish {

enum NetworkProtocolUnitStructFields {
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

enum NetworkProtocolListenResponseElements {
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

struct ServiceConfiguration {
  std::string serviceName;
  std::string socketPath;
};

class OnDemandNetworkProtocolProvider {
 public:
  template <typename CallbackFunc>
  static void getServices(CallbackFunc&& callback) {
    crow::connections::systemBus->async_method_call(
        [callback{std::move(callback)}](const boost::system::error_code ec,
                                        const std::vector<UnitStruct>& resp) {
          if (ec) {
            callback(false, resp);
          } else {
            callback(true, resp);
          }
        },
        "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
        "org.freedesktop.systemd1.Manager", "ListUnits");
  }

  template <typename CallbackFunc>
  static void getSocketListenPort(const std::string& path,
                                  CallbackFunc&& callback) {
    crow::connections::systemBus->async_method_call(
        [callback{std::move(callback)}](
            const boost::system::error_code ec,
            const sdbusplus::message::variant<
                std::vector<std::tuple<std::string, std::string>>>& resp) {
          if (ec) {
            callback(false, false, 0);
          } else {
            auto responsePtr = mapbox::getPtr<
                const std::vector<std::tuple<std::string, std::string>>>(resp);

            std::string listenStream =
                std::get<NET_PROTO_LISTEN_STREAM>((*responsePtr)[0]);
            auto lastColonPos = listenStream.rfind(":");
            if (lastColonPos != std::string::npos) {
              std::string portStr = listenStream.substr(lastColonPos + 1);
              char* endPtr;
              // Use strtol instead of stroi to avoid exceptions
              long port = std::strtol(portStr.c_str(), &endPtr, 10);

              if (*endPtr != '\0' || portStr.empty()) {
                // Invalid value
                callback(true, false, 0);
              } else {
                // Everything OK
                callback(true, true, port);
              }
            } else {
              // Not a port
              callback(true, false, 0);
            }
          }
        },
        "org.freedesktop.systemd1", path, "org.freedesktop.DBus.Properties",
        "Get", "org.freedesktop.systemd1.Socket", "Listen");
  }
};

class NetworkProtocol : public Node {
 public:
  NetworkProtocol(CrowApp& app)
      : Node(app, "/redfish/v1/Managers/openbmc/NetworkProtocol") {
    Node::json["@odata.type"] =
        "#ManagerNetworkProtocol.v1_1_0.ManagerNetworkProtocol";
    Node::json["@odata.id"] = "/redfish/v1/Managers/openbmc/NetworkProtocol";
    Node::json["@odata.context"] =
        "/redfish/v1/$metadata#ManagerNetworkProtocol.ManagerNetworkProtocol";
    Node::json["Id"] = "NetworkProtocol";
    Node::json["Name"] = "Manager Network Protocol";
    Node::json["Description"] = "Manager Network Service";
    Node::json["Status"]["Health"] = "OK";
    Node::json["Status"]["HealthRollup"] = "OK";
    Node::json["Status"]["State"] = "Enabled";

    for (auto& protocol : protocolToDBus) {
      Node::json[protocol.first]["ProtocolEnabled"] = false;
    }

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
             const std::vector<std::string>& params) override {
    std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

    getData(asyncResp);
  }

  std::string getHostName() const {
    std::string hostName;

    std::array<char, HOST_NAME_MAX> hostNameCStr;
    if (gethostname(hostNameCStr.data(), hostNameCStr.size()) == 0) {
      hostName = hostNameCStr.data();
    }
    return hostName;
  }

  void getData(const std::shared_ptr<AsyncResp>& asyncResp) {
    Node::json["HostName"] = getHostName();
    asyncResp->res.jsonValue = Node::json;

    OnDemandNetworkProtocolProvider::getServices(
        [&, asyncResp](const bool success,
                       const std::vector<UnitStruct>& resp) {
          if (!success) {
            asyncResp->res.jsonValue = nlohmann::json::object();
            messages::addMessageToErrorJson(asyncResp->res.jsonValue,
                                            messages::internalError());
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
          }

          for (auto& unit : resp) {
            for (auto& kv : protocolToDBus) {
              if (kv.second.serviceName ==
                  std::get<NET_PROTO_UNIT_NAME>(unit)) {
                std::string service = kv.first;

                // Process state
                if (std::get<NET_PROTO_UNIT_SUB_STATE>(unit) == "running") {
                  asyncResp->res.jsonValue[service]["ProtocolEnabled"] = true;
                } else {
                  asyncResp->res.jsonValue[service]["ProtocolEnabled"] = false;
                }

                // Process port
                OnDemandNetworkProtocolProvider::getSocketListenPort(
                    kv.second.socketPath,
                    [&, asyncResp, service{std::move(service)} ](
                        const bool fetchSuccess, const bool portAvailable,
                        const unsigned long port) {
                      if (fetchSuccess) {
                        if (portAvailable) {
                          asyncResp->res.jsonValue[service]["Port"] = port;
                        } else {
                          asyncResp->res.jsonValue[service]["Port"] = nullptr;
                        }
                      } else {
                        messages::addMessageToJson(asyncResp->res.jsonValue,
                                                   messages::internalError(),
                                                   "/" + service);
                      }
                    });
                break;
              }
            }
          }
        });
  }

  boost::container::flat_map<std::string, ServiceConfiguration> protocolToDBus{
      {"SSH",
       {"dropbear.service",
        "/org/freedesktop/systemd1/unit/dropbear_2esocket"}},
      {"HTTPS",
       {"phosphor-gevent.service",
        "/org/freedesktop/systemd1/unit/phosphor_2dgevent_2esocket"}},
      {"IPMI",
       {"phosphor-ipmi-net.service",
        "/org/freedesktop/systemd1/unit/phosphor_2dipmi_2dnet_2esocket"}}};
};

}  // namespace redfish
