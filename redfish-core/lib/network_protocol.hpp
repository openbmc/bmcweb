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

#include "node.hpp"

namespace redfish {

class NetworkProtocol : public Node {
 public:
  NetworkProtocol(CrowApp& app)
      : Node(app,
             "/redfish/v1/Managers/bmc/NetworkProtocol") {
    Node::json["@odata.type"] =
        "#ManagerNetworkProtocol.v1_1_0.ManagerNetworkProtocol";
    Node::json["@odata.id"] = "/redfish/v1/Managers/bmc/NetworkProtocol";
    Node::json["@odata.context"] =
        "/redfish/v1/$metadata#ManagerNetworkProtocol.ManagerNetworkProtocol";
    Node::json["Id"] = "NetworkProtocol";
    Node::json["Name"] = "Manager Network Protocol";
    Node::json["Description"] = "Manager Network Service";
    Node::json["Status"]["Health"] = "OK";
    Node::json["Status"]["HealthRollup"] = "OK";
    Node::json["Status"]["State"] = "Enabled";

    entityPrivileges = {{crow::HTTPMethod::GET, {{"Login"}}},
                        {crow::HTTPMethod::HEAD, {{"Login"}}},
                        {crow::HTTPMethod::PATCH, {{"ConfigureManager"}}},
                        {crow::HTTPMethod::PUT, {{"ConfigureManager"}}},
                        {crow::HTTPMethod::DELETE, {{"ConfigureManager"}}},
                        {crow::HTTPMethod::POST, {{"ConfigureManager"}}}};
  }

 private:
  void doGet(crow::response& res, const crow::request& req,
             const std::vector<std::string>& params) override {
    refreshProtocolsState();
    Node::json["HostName"] = getHostName();
    res.json_value = Node::json;
    res.end();
  }

  std::string getHostName() const {
    std::string hostName;

    std::array<char, HOST_NAME_MAX> hostNameCStr;
    if (gethostname(hostNameCStr.data(), hostNameCStr.size()) == 0) {
      hostName = hostNameCStr.data();
    }
    return hostName;
  }

  void refreshProtocolsState() {
    refreshListeningPorts();
    for (auto& kv : portToProtocolMap) {
      Node::json[kv.second]["Port"] = kv.first;
      if (listeningPorts.find(kv.first) != listeningPorts.end()) {
        Node::json[kv.second]["ProtocolEnabled"] = true;
      } else {
        Node::json[kv.second]["ProtocolEnabled"] = false;
      }
    }
  }

  void refreshListeningPorts() {
    listeningPorts.clear();
    std::array<char, 128> netstatLine;
    FILE* p = popen("netstat -tuln | awk '{ print $4 }'", "r");
    if (p != nullptr) {
      while (fgets(netstatLine.data(), netstatLine.size(), p) != nullptr) {
        auto s = std::string(netstatLine.data());

        // get port num from strings such as: ".*:.*:.*:port"
        s.erase(0, s.find_last_of(":") + strlen(":"));

        auto port = atoi(s.c_str());
        if (port != 0 &&
            portToProtocolMap.find(port) != portToProtocolMap.end()) {
          listeningPorts.insert(port);
        }
      }
    }
  }

  std::map<int, std::string> portToProtocolMap{
      {22, "SSH"}, {80, "HTTP"}, {443, "HTTPS"}, {623, "IPMI"}, {1900, "SSDP"}};

  std::set<int> listeningPorts;
};

}  // namespace redfish
