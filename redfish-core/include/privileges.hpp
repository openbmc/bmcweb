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
#include <boost/container/flat_set.hpp>
#include <boost/optional.hpp>

namespace redfish {

class PrivilegeProvider;

enum class PrivilegeType { BASE, OEM };

/** @brief Max number of privileges per type  */
constexpr const size_t MAX_PRIVILEGE_COUNT = 32;

using privilegeBitset = std::bitset<MAX_PRIVILEGE_COUNT>;

/** @brief Number of mappings must be <= MAX_PRIVILEGE_COUNT */
static const boost::container::flat_map<std::string, size_t>
    basePrivNameToIndexMap = {{"Login", 0},
                              {"ConfigureManager", 1},
                              {"ConfigureComponents", 2},
                              {"ConfigureSelf", 3},
                              {"ConfigureUsers", 4}};

/** @brief Number of mappings must be <= MAX_PRIVILEGE_COUNT */
static const boost::container::flat_map<std::string, size_t>
    oemPrivNameToIndexMap = {};

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
 *        A bit is set if the privilege is required (entity domain) or granted
 *        (user domain) and false otherwise.
 *
 */
class Privileges {
 public:
  /**
   * @brief Constructs object without any privileges active
   *
   */
  Privileges() = default;

  /**
   * @brief Constructs object with given privileges active
   *
   * @param[in] privilegeList  List of privileges to be activated
   *
   */
  Privileges(std::initializer_list<std::string> privilegeList) {
    for (const auto& privilege : privilegeList) {
      setSinglePrivilege(privilege);
    }
  }

  /**
   * @brief Retrieves the base privileges bitset
   *
   * @return          Bitset representation of base Redfish privileges
   *
   */
  privilegeBitset getBasePrivilegeBitset() const { return basePrivilegeBitset; }

  /**
   * @brief Retrieves the OEM privileges bitset
   *
   * @return          Bitset representation of OEM Redfish privileges
   *
   */
  privilegeBitset getOEMPrivilegeBitset() const { return oemPrivilegeBitset; }

  /**
   * @brief Sets given privilege in the bitset
   *
   * @param[in] privilege  Privilege to be set
   *
   * @return               None
   *
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
   *
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

  friend class PrivilegeProvider;
};

using OperationMap =
    boost::container::flat_map<crow::HTTPMethod, std::vector<Privileges>>;

/**
 * @brief  Class used to store overrides privileges for Redfish
 *         entities
 *
 */
class EntityPrivilegesOverride {
 protected:
  /**
   * @brief Constructs overrides object for given targets
   *
   * @param[in] operationMap Operation map to be applied for targets
   * @param[in] targets      List of targets whOperation map to be applied for
   * targets
   *
   */
  EntityPrivilegesOverride(OperationMap&& operationMap,
                           std::initializer_list<std::string>&& targets)
      : operationMap(std::move(operationMap)), targets(std::move(targets)) {}

  const OperationMap operationMap;
  const boost::container::flat_set<std::string> targets;
};

class PropertyOverride : public EntityPrivilegesOverride {
 public:
  PropertyOverride(OperationMap&& operationMap,
                   std::initializer_list<std::string>&& targets)
      : EntityPrivilegesOverride(std::move(operationMap), std::move(targets)) {}
};

class SubordinateOverride : public EntityPrivilegesOverride {
 public:
  SubordinateOverride(OperationMap&& operationMap,
                      std::initializer_list<std::string>&& targets)
      : EntityPrivilegesOverride(std::move(operationMap), std::move(targets)) {}
};

class ResourceURIOverride : public EntityPrivilegesOverride {
 public:
  ResourceURIOverride(OperationMap&& operationMap,
                      std::initializer_list<std::string>&& targets)
      : EntityPrivilegesOverride(std::move(operationMap), std::move(targets)) {}
};

/**
 * @brief  Class used to store privileges for Redfish entities
 *
 */
class EntityPrivileges {
 public:
  /**
   * @brief Constructor for default case with no overrides
   *
   * @param[in] operationMap Operation map for the entity
   *
   */
  EntityPrivileges(OperationMap&& operationMap)
      : operationMap(std::move(operationMap)) {}

  /**
   * @brief Constructors for overrides
   *
   * @param[in] operationMap         Default operation map for the entity
   * @param[in] propertyOverrides    Vector of property overrides
   * @param[in] subordinateOverrides Vector of subordinate overrides
   * @param[in] resourceURIOverrides Vector of resource URI overrides
   *
   */
  EntityPrivileges(OperationMap&& operationMap,
                   std::vector<PropertyOverride>&& propertyOverrides,
                   std::vector<SubordinateOverride>&& subordinateOverrides,
                   std::vector<ResourceURIOverride>&& resourceURIOverrides)
      : operationMap(std::move(operationMap)),
        propertyOverrides(std::move(propertyOverrides)),
        subordinateOverrides(std::move(subordinateOverrides)),
        resourceURIOverrides(std::move(resourceURIOverrides)) {}

  /**
   * @brief Checks if a user is allowed to call an HTTP method
   *
   * @param[in] method       HTTP method
   * @param[in] user         Username
   *
   * @return                 True if method allowed, false otherwise
   *
   */
  bool isMethodAllowedForUser(const crow::HTTPMethod method,
                              const std::string& user) const {
    // TODO: load user privileges from configuration as soon as its available
    // now we are granting all privileges to everyone.
    auto userPrivileges =
        Privileges{"Login", "ConfigureManager", "ConfigureSelf",
                   "ConfigureUsers", "ConfigureComponents"};

    return isMethodAllowedWithPrivileges(method, userPrivileges);
  }

  /**
   * @brief Checks if given privileges allow to call an HTTP method
   *
   * @param[in] method       HTTP method
   * @param[in] user         Privileges
   *
   * @return                 True if method allowed, false otherwise
   *
   */
  bool isMethodAllowedWithPrivileges(const crow::HTTPMethod method,
                                     const Privileges& userPrivileges) const {
    if (operationMap.find(method) == operationMap.end()) {
      return false;
    }

    for (auto& requiredPrivileges : operationMap.at(method)) {
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

 private:
  bool verifyPrivileges(const privilegeBitset userPrivilegeBitset,
                        const privilegeBitset requiredPrivilegeBitset) const {
    return (userPrivilegeBitset & requiredPrivilegeBitset) ==
           requiredPrivilegeBitset;
  }

  OperationMap operationMap;

  // Overrides are not implemented at the moment.
  std::vector<PropertyOverride> propertyOverrides;
  std::vector<SubordinateOverride> subordinateOverrides;
  std::vector<ResourceURIOverride> resourceURIOverrides;
};

}  // namespace redfish

