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

#include <bitset>
#include <cstdint>
#include "crow.h"
#include <boost/container/flat_map.hpp>
#include <boost/optional.hpp>

namespace redfish {

class PrivilegeProvider;

enum class PrivilegeType { BASE, OEM };

/** @brief Max number of privileges per type  */
constexpr const size_t MAX_PRIVILEGE_COUNT = 32;
using privilegeBitset = std::bitset<MAX_PRIVILEGE_COUNT>;

/**
 * @brief Redfish privileges
 *
 *        Entity privileges and user privileges are represented by this class.
 *
 *        Each incoming connection requires a comparison between privileges held
 *        by the user issuing a request and the target entity's privileges.
 *
 *        To ensure best runtime performance of this comparison, privileges
 *        are represented as bitsets. Each bit in the bitset corresponds to a
 *        unique privilege name.
 *
 *        Privilege names are read from the privilege_registry.json file and
 *        stored in flat maps.
 *
 *        A bit is set if the privilege is required (entity domain) or granted
 *        (user domain) and false otherwise.
 *
 *        Bitset index to privilege name mapping depends on the order in which
 *        privileges are defined in PrivilegesUsed and OEMPrivilegesUsed arrays
 *        in the privilege_registry.json.
 */
class Privileges {
 public:
  /**
   * @brief Retrieves the base privileges bitset
   *
   * @return          Bitset representation of base Redfish privileges
   */
  privilegeBitset getBasePrivilegeBitset() const { return basePrivilegeBitset; }

  /**
   * @brief Retrieves the OEM privileges bitset
   *
   * @return          Bitset representation of OEM Redfish privileges
   */
  privilegeBitset getOEMPrivilegeBitset() const { return oemPrivilegeBitset; }

  /**
   * @brief Sets given privilege in the bitset
   *
   * @param[in] privilege  Privilege to be set
   *
   * @return               None
   */
  void setSinglePrivilege(const std::string& privilege) {
    auto index = getBitsetIndexForPrivilege(privilege, PrivilegeType::BASE);
    if (index) {
      basePrivilegeBitset.set(*index);
      return;
    }

    index = getBitsetIndexForPrivilege(privilege, PrivilegeType::OEM);
    if (index) {
      oemPrivilegeBitset.set(*index);
    }
  }

  /**
   * @brief Retrieves names of all active privileges for a given type
   *
   * @param[in] type    Base or OEM
   *
   * @return            Vector of active privileges
   */
  std::vector<std::string> getActivePrivilegeNames(
      const PrivilegeType type) const {
    std::vector<std::string> activePrivileges;

    if (type == PrivilegeType::BASE) {
      for (const auto& pair : basePrivNameToIndexMap) {
        if (basePrivilegeBitset.test(pair.second)) {
          activePrivileges.emplace_back(pair.first);
        }
      }
    } else {
      for (const auto& pair : oemPrivNameToIndexMap) {
        if (oemPrivilegeBitset.test(pair.second)) {
          activePrivileges.emplace_back(pair.first);
        }
      }
    }

    return activePrivileges;
  }

 private:
  boost::optional<size_t> getBitsetIndexForPrivilege(
      const std::string& privilege, const PrivilegeType type) const {
    if (type == PrivilegeType::BASE) {
      const auto pair = basePrivNameToIndexMap.find(privilege);
      if (pair != basePrivNameToIndexMap.end()) {
        return pair->second;
      }
    } else {
      const auto pair = oemPrivNameToIndexMap.find(privilege);
      if (pair != oemPrivNameToIndexMap.end()) {
        return pair->second;
      }
    }

    return boost::none;
  }

  privilegeBitset basePrivilegeBitset;
  privilegeBitset oemPrivilegeBitset;

  static boost::container::flat_map<std::string, size_t> basePrivNameToIndexMap;
  static boost::container::flat_map<std::string, size_t> oemPrivNameToIndexMap;

  friend class PrivilegeProvider;
};

/**
 * @brief  Class used to store privileges for Redfish entities
 */
class EntityPrivileges {
 public:
  /**
   * @brief Checks if a user is allowed to call an HTTP method
   *
   * @param[in] method       HTTP method
   * @param[in] user         Username
   *
   * @return                 True if method allowed, false otherwise
   */
  bool isMethodAllowedForUser(const crow::HTTPMethod method,
                              const std::string& user) const;

  /**
   * @brief Checks if given privileges allow to call an HTTP method
   *
   * @param[in] method       HTTP method
   * @param[in] user         Privileges
   *
   * @return                 True if method allowed, false otherwise
   */
  bool isMethodAllowedWithPrivileges(const crow::HTTPMethod method,
                                     const Privileges& userPrivileges) const;

  /**
   * @brief Sets required privileges for a method on a given entity
   *
   * @param[in] method       HTTP method
   * @param[in] privileges   Required privileges
   *
   * @return                 None
   */
  void addPrivilegesRequiredByMethod(const crow::HTTPMethod method,
                                     const Privileges& privileges) {
    methodToPrivilegeMap[method].push_back(privileges);
  }

 private:
  bool verifyPrivileges(const privilegeBitset userPrivilegeBitset,
                        const privilegeBitset requiredPrivilegeBitset) const;

  boost::container::flat_map<crow::HTTPMethod, std::vector<Privileges>>
      methodToPrivilegeMap;
};

/**
 * @brief  Class used to:
 *         -  read the privilege_registry.json file
 *         -  provide EntityPrivileges objects to callers
 *
 *         To save runtime memory, object of this class should
 *         exist only for the time required to install all Nodes
 */
class PrivilegeProvider {
 public:
  PrivilegeProvider(const std::string& privilegeRegistryPath) {
    // TODO: read this path from the configuration once its available
    std::ifstream privilegeRegistryFile{privilegeRegistryPath};

    if (privilegeRegistryFile.is_open()) {
      if (!loadPrivilegesFromFile(privilegeRegistryFile)) {
        privilegeRegistryJson.clear();
        CROW_LOG_ERROR << "Couldn't parse privilege_registry.json";
      }
    } else {
      CROW_LOG_ERROR << "Couldn't open privilege_registry.json";
    }
  }

  /**
   * @brief Gets required privileges for a certain entity type
   *
   * @param[in] entityUrl    Entity url
   * @param[in] entityType   Entity type
   *
   * @return                 EntityPrivilege object
   */
  EntityPrivileges getPrivilegesRequiredByEntity(
      const std::string& entityUrl, const std::string& entityType) const;

 private:
  bool loadPrivilegesFromFile(std::ifstream& privilegeRegistryFile);
  bool privilegeRegistryHasRequiredFields() const;
  bool parseOperationMap(const nlohmann::json& operationMap,
                         EntityPrivileges& entityPrivileges) const;
  bool fillPrivilegeMap(const nlohmann::json& privilegesUsed,
                        boost::container::flat_map<std::string, size_t>&
                            privilegeToIndexMap) const;

  nlohmann::json privilegeRegistryJson;
};

}  // namespace redfish

