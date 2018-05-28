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

#include "../lib/account_service.hpp"
#include "../lib/managers.hpp"
#include "../lib/network_protocol.hpp"
#include "../lib/redfish_sessions.hpp"
#include "../lib/roles.hpp"
#include "../lib/service_root.hpp"
#include "../lib/ethernet.hpp"
#include "../lib/thermal.hpp"
#ifdef OCP_CUSTOM_FLAG // TODO Add OCP custom flag for include target header
#include "../lib/ocp-chassis.hpp"
#include "../lib/power.hpp"
#else
#include "../lib/chassis.hpp"
#endif
#include "../lib/systems.hpp"
#include "../lib/logservices.hpp"
#include "webserver_common.hpp"

namespace redfish {
/*
 * @brief Top level class installing and providing Redfish services
 */
class RedfishService {
 public:
  /*
   * @brief Redfish service constructor
   *
   * Loads Redfish configuration and installs schema resources
   *
   * @param[in] app   Crow app on which Redfish will initialize
   */
  RedfishService(CrowApp& app) {
    nodes.emplace_back(std::make_unique<AccountService>(app));
    nodes.emplace_back(std::make_unique<SessionCollection>(app));
    nodes.emplace_back(std::make_unique<Roles>(app));
    nodes.emplace_back(std::make_unique<RoleCollection>(app));
    nodes.emplace_back(std::make_unique<ServiceRoot>(app));
    nodes.emplace_back(std::make_unique<NetworkProtocol>(app));
    nodes.emplace_back(std::make_unique<SessionService>(app));
    nodes.emplace_back(std::make_unique<EthernetCollection>(app));
    nodes.emplace_back(std::make_unique<EthernetInterface>(app));
    nodes.emplace_back(std::make_unique<Thermal>(app));
    nodes.emplace_back(std::make_unique<ManagerCollection>(app));
    nodes.emplace_back(std::make_unique<ChassisCollection>(app));
#ifdef OCP_CUSTOM_FLAG // Add power node
    nodes.emplace_back(std::make_unique<Power>(app));
#endif //OCP_CUSTOM_FLAG
#ifndef OCP_CUSTOM_FLAG // TODO Remove Chassis node
                        // which unused in case definition of OCP custom flag
    nodes.emplace_back(std::make_unique<Chassis>(app));
#endif
    nodes.emplace_back(std::make_unique<SystemsCollection>(app));
    nodes.emplace_back(std::make_unique<Systems>(app));
    nodes.emplace_back(std::make_unique<LogServiceCollection>(app));
    nodes.emplace_back(std::make_unique<LogService>(app));
    nodes.emplace_back(std::make_unique<LogEntryCollection>(app));
    nodes.emplace_back(std::make_unique<LogEntry>(app));

    for (auto& node : nodes) {
      node->getSubRoutes(nodes);
    }
  }

 private:
  std::vector<std::unique_ptr<Node>> nodes;
};

}  // namespace redfish
