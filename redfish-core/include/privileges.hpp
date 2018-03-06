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
#include <vector>
#include "crow.h"
#include <boost/container/flat_map.hpp>
#include <boost/optional.hpp>

namespace redfish {

enum class PrivilegeType { BASE, OEM };

/** @brief A fixed array of compile time privileges  */
constexpr std::array<const char*, 5> basePrivileges{
    "Login", "ConfigureManager", "ConfigureComponents", "ConfigureSelf",
    "ConfigureUsers"};

constexpr const int basePrivilegeCount = basePrivileges.size();

/** @brief Max number of privileges per type  */
constexpr const int MAX_PRIVILEGE_COUNT = 32;

/** @brief A vector of all privilege names and their indexes */
static const std::vector<std::string> privilegeNames{basePrivileges.begin(),
                                                     basePrivileges.end()};

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
  Privileges(std::initializer_list<const char*> privilegeList) {
    for (const char* privilege : privilegeList) {
      if (!setSinglePrivilege(privilege)) {
        CROW_LOG_CRITICAL << "Unable to set privilege " << privilege
                          << "in constructor";
      }
    }
  }

  /**
   * @brief Sets given privilege in the bitset
   *
   * @param[in] privilege  Privilege to be set
   *
   * @return               None
   *
   */
  bool setSinglePrivilege(const char* privilege) {
    for (int search_index = 0; search_index < privilegeNames.size();
         search_index++) {
      if (privilege == privilegeNames[search_index]) {
        privilegeBitset.set(search_index);
        return true;
      }
    }

    return false;
  }

  /**
   * @brief Sets given privilege in the bitset
   *
   * @param[in] privilege  Privilege to be set
   *
   * @return               None
   *
   */
  bool setSinglePrivilege(const std::string& privilege) {
    return setSinglePrivilege(privilege.c_str());
  }

  /**
   * @brief Retrieves names of all active privileges for a given type
   *
   * @param[in] type    Base or OEM
   *
   * @return            Vector of active privileges.  Pointers are valid until
   * the setSinglePrivilege is called, or the Privilege structure is destroyed
   *
   */
  std::vector<const std::string*> getActivePrivilegeNames(
      const PrivilegeType type) const {
    std::vector<const std::string*> activePrivileges;

    int search_index = 0;
    int end_index = basePrivilegeCount;
    if (type == PrivilegeType::OEM) {
      search_index = basePrivilegeCount - 1;
      end_index = privilegeNames.size();
    }

    for (; search_index < end_index; search_index++) {
      if (privilegeBitset.test(search_index)) {
        activePrivileges.emplace_back(&privilegeNames[search_index]);
      }
    }

    return activePrivileges;
  }

  /**
   * @brief Determines if this Privilege set is a superset of the given
   * privilege set
   *
   * @param[in] privilege  Privilege to be checked
   *
   * @return               None
   *
   */
  bool isSupersetOf(const Privileges& p) const {
    return (privilegeBitset & p.privilegeBitset) == p.privilegeBitset;
  }

 private:
  std::bitset<MAX_PRIVILEGE_COUNT> privilegeBitset = 0;
};

using OperationMap =
    boost::container::flat_map<crow::HTTPMethod, std::vector<Privileges>>;

/**
 * @brief Checks if given privileges allow to call an HTTP method
 *
 * @param[in] method       HTTP method
 * @param[in] user         Privileges
 *
 * @return                 True if method allowed, false otherwise
 *
 */
inline bool isMethodAllowedWithPrivileges(const crow::HTTPMethod method,
                                          const OperationMap& operationMap,
                                          const Privileges& userPrivileges) {
  const auto& it = operationMap.find(method);
  if (it == operationMap.end()) {
    return false;
  }

  // If there are no privileges assigned, assume no privileges required
  if (it->second.empty()) {
    return true;
  }

  for (auto& requiredPrivileges : it->second) {
    if (userPrivileges.isSupersetOf(requiredPrivileges)) {
      return true;
    }
  }
  return false;
}

/**
 * @brief Checks if a user is allowed to call an HTTP method
 *
 * @param[in] method       HTTP method
 * @param[in] user         Username
 *
 * @return                 True if method allowed, false otherwise
 *
 */
inline bool isMethodAllowedForUser(const crow::HTTPMethod method,
                                   const OperationMap& operationMap,
                                   const std::string& user) {
  // TODO: load user privileges from configuration as soon as its available
  // now we are granting all privileges to everyone.
  Privileges userPrivileges{"Login", "ConfigureManager", "ConfigureSelf",
                            "ConfigureUsers", "ConfigureComponents"};

  return isMethodAllowedWithPrivileges(method, operationMap, userPrivileges);
}

}  // namespace redfish
