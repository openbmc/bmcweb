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
#include "privileges.hpp"

namespace redfish {

boost::container::flat_map<std::string, size_t>
    Privileges::basePrivNameToIndexMap;
boost::container::flat_map<std::string, size_t>
    Privileges::oemPrivNameToIndexMap;

bool EntityPrivileges::isMethodAllowedForUser(const crow::HTTPMethod method,
                                              const std::string& user) const {
  // TODO: load user privileges from configuration as soon as its available
  // now we are granting only Login.
  auto userPrivileges = Privileges();
  userPrivileges.setSinglePrivilege("Login");

  return isMethodAllowedWithPrivileges(method, userPrivileges);
}

bool EntityPrivileges::isMethodAllowedWithPrivileges(
    const crow::HTTPMethod method, const Privileges& userPrivileges) const {
  if (methodToPrivilegeMap.find(method) == methodToPrivilegeMap.end()) {
    return false;
  }

  for (auto& requiredPrivileges : methodToPrivilegeMap.at(method)) {
    // Check if user has required base privileges
    if (!verifyPrivileges(userPrivileges.getBasePrivilegeBitset(),
                          requiredPrivileges.getBasePrivilegeBitset())) {
      continue;
    }

    // Check if user has required OEM privileges
    if (!verifyPrivileges(userPrivileges.getOEMPrivilegeBitset(),
                          requiredPrivileges.getOEMPrivilegeBitset())) {
      continue;
    }

    return true;
  }
  return false;
}

bool EntityPrivileges::verifyPrivileges(
    const privilegeBitset userPrivilegeBitset,
    const privilegeBitset requiredPrivilegeBitset) const {
  return (userPrivilegeBitset & requiredPrivilegeBitset) ==
         requiredPrivilegeBitset;
}

EntityPrivileges PrivilegeProvider::getPrivilegesRequiredByEntity(
    const std::string& entityUrl, const std::string& entityType) const {
  if (privilegeRegistryJson.empty()) {
    return EntityPrivileges();
  }

  // type from @odata.type e.g: ServiceRoot from #ServiceRoot.v1_1_1.ServiceRoot
  auto entity = entityType.substr(entityType.find_last_of(".") + strlen("."));

  for (auto mapping : privilegeRegistryJson.at("Mappings")) {
    const auto& entityJson = mapping.find("Entity");
    const auto& operationMapJson = mapping.find("OperationMap");
    const auto& propertyOverridesJson = mapping.find("PropertyOverrides");
    const auto& subordinateOverridesJson = mapping.find("SubordinateOverrides");
    const auto& resourceURIOverridesJson = mapping.find("ResourceURIOverrides");

    if (entityJson == mapping.end() || operationMapJson == mapping.end()) {
      return EntityPrivileges();
    }

    if (entityJson->is_string() && entity == entityJson.value()) {
      auto entityPrivileges = EntityPrivileges();

      if (!parseOperationMap(operationMapJson.value(), entityPrivileges)) {
        return EntityPrivileges();
      }

      if (propertyOverridesJson != mapping.end()) {
        // TODO: implementation comes in next patch-sets
      }
      if (subordinateOverridesJson != mapping.end()) {
        // TODO: implementation comes in next patch-sets
      }
      if (resourceURIOverridesJson != mapping.end()) {
        // TODO: implementation comes in next patch-sets
      }

      return entityPrivileges;
    }
  }
  return EntityPrivileges();
}

bool PrivilegeProvider::parseOperationMap(
    const nlohmann::json& operationMap,
    EntityPrivileges& entityPrivileges) const {
  for (auto it = operationMap.begin(); it != operationMap.end(); ++it) {
    const std::string& method = it.key();
    const nlohmann::json& privilegesForMethod = it.value();

    for (const auto& privilegeOr : privilegesForMethod) {
      const auto& privilegeJson = privilegeOr.find("Privilege");

      if (privilegeJson == privilegeOr.end()) {
        return false;
      }
      auto privileges = Privileges();

      for (auto& privilegeAnd : privilegeJson.value()) {
        if (!privilegeAnd.is_string()) {
          return false;
        }
        privileges.setSinglePrivilege(privilegeAnd);
      }
      entityPrivileges.addPrivilegesRequiredByMethod(operator"" _method(
                                                         method.c_str(),
                                                         method.size()),
                                                     privileges);
    }
  }
  return true;
}

bool PrivilegeProvider::loadPrivilegesFromFile(
    std::ifstream& privilegeRegistryFile) {
  privilegeRegistryJson =
      nlohmann::json::parse(privilegeRegistryFile, nullptr, false);

  if (!privilegeRegistryHasRequiredFields()) {
    return false;
  }

  const nlohmann::json& basePrivilegesUsed =
      privilegeRegistryJson.at("PrivilegesUsed");
  if (basePrivilegesUsed.size() == 0) {
    return false;
  }
  if (!fillPrivilegeMap(basePrivilegesUsed,
                        Privileges::basePrivNameToIndexMap)) {
    return false;
  }

  const nlohmann::json& oemPrivilegesUsed =
      privilegeRegistryJson.at("OEMPrivilegesUsed");
  if (!fillPrivilegeMap(oemPrivilegesUsed, Privileges::oemPrivNameToIndexMap)) {
    return false;
  }

  return true;
}

bool PrivilegeProvider::privilegeRegistryHasRequiredFields() const {
  if (privilegeRegistryJson.is_discarded() ||
      privilegeRegistryJson.find("@Redfish.Copyright") ==
          privilegeRegistryJson.end() ||
      privilegeRegistryJson.find("@odata.type") ==
          privilegeRegistryJson.end() ||
      privilegeRegistryJson.find("Id") == privilegeRegistryJson.end() ||
      privilegeRegistryJson.find("Name") == privilegeRegistryJson.end() ||
      privilegeRegistryJson.find("Mappings") == privilegeRegistryJson.end() ||
      privilegeRegistryJson.find("PrivilegesUsed") ==
          privilegeRegistryJson.end() ||
      privilegeRegistryJson.find("OEMPrivilegesUsed") ==
          privilegeRegistryJson.end()) {
    return false;
  }
  return true;
}

bool PrivilegeProvider::fillPrivilegeMap(
    const nlohmann::json& privilegesUsed,
    boost::container::flat_map<std::string, size_t>& privilegeToIndexMap)
    const {
  privilegeToIndexMap.clear();
  for (auto& privilege : privilegesUsed) {
    if (privilegeToIndexMap.size() < MAX_PRIVILEGE_COUNT) {
      if (!privilege.is_string()) {
        return false;
      }
      privilegeToIndexMap.insert(std::pair<std::string, size_t>(
          privilege.get<std::string>(), privilegeToIndexMap.size()));
    }
  }
  return true;
}

}  // namespace redfish
