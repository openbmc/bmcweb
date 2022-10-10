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

#include "http_verbs.hpp"
#include "logging.hpp"
#include "registries/privilege_registry.hpp"

#include <boost/beast/http/verb.hpp>
#include <boost/container/vector.hpp>
#include <boost/move/algo/move.hpp>
#include <nlohmann/json.hpp>

#include <array>
#include <bitset>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

// IWYU pragma: no_include <stddef.h>

namespace redfish
{

enum class PrivilegeType
{
    BASE,
    OEM
};

class Privileges;

/**
 * @brief The OperationMap represents the privileges required for a
 * single entity (URI).  It maps from the allowable verbs to the
 * privileges required to use that operation.
 *
 * This represents the Redfish "Privilege AND and OR syntax" as given
 * in the spec and shown in the Privilege Registry.  This does not
 * implement any Redfish property overrides, subordinate overrides, or
 * resource URI overrides.  This does not implement the limitation of
 * the ConfigureSelf privilege to operate only on your own account or
 * session.
 **/
using OperationMap =
    std::array<std::vector<Privileges>, http::allRedfishMethods.size()>;

// Mappings between entities and the relevant privileges that access those
// entities.
// To store the data efficiently, the implementation uses an array.
// E.g., mappings[0] is the privileges required to access the Entity
// |privileges::entities[0]| (which is AccelerationFunction)
using Mappings = std::array<OperationMap, privileges::entities.size()>;

/**
 * @brief Redfish privileges
 *
 *        This implements a set of Redfish privileges.  These directly represent
 *        user privileges and help represent entity privileges.
 *
 *        Each incoming Connection requires a comparison between privileges held
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
class Privileges
{
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
    Privileges(std::initializer_list<const char*> privilegeList);

    explicit Privileges(uint64_t privilegeBitsetIn) :
        privilegeBitset(privilegeBitsetIn)
    {}

    /**
     * @brief Sets given privilege in the bitset
     *
     * @param[in] privilege  Privilege to be set
     *
     * @return               None
     *
     */
    bool setSinglePrivilege(std::string_view privilege);

    /**
     * @brief Resets the given privilege in the bitset
     *
     * @param[in] privilege  Privilege to be reset
     *
     * @return               None
     *
     */
    bool resetSinglePrivilege(std::string_view privilege);

    /**
     * @brief Retrieves names of all active privileges for a given type
     *
     * @param[in] type    Base or OEM
     *
     * @return            Vector of active privileges.  Pointers are valid until
     * the setSinglePrivilege is called, or the Privilege structure is destroyed
     *
     */
    std::vector<std::string> getActivePrivilegeNames(PrivilegeType type) const;

    // Returns active privileges of both types (Base and OEM)
    std::vector<std::string> getAllActivePrivilegeNames() const;

    /**
     * @brief Determines if this Privilege set is a superset of the given
     * privilege set
     *
     * @param[in] privilege  Privilege to be checked
     *
     * @return               None
     *
     */
    bool isSupersetOf(const Privileges& p) const
    {
        return (privilegeBitset & p.privilegeBitset) == p.privilegeBitset;
    }

    /**
     * @brief Returns the intersection of two Privilege sets.
     *
     * @param[in] privilege  Privilege set to intersect with.
     *
     * @return               The new Privilege set.
     *
     */
    Privileges intersection(const Privileges& p) const
    {
        return Privileges{privilegeBitset & p.privilegeBitset};
    }

    static std::span<Privileges>
        getPrivilegesForRedfishRoute(privileges::EntityTag tag,
                                     boost::beast::http::verb method)
    {
        size_t tagIndex = static_cast<size_t>(tag);
        size_t methodIndex = static_cast<size_t>(method);
        return mappings[tagIndex][methodIndex];
    }

    // Returns the OperationMap of the given Entity
    static nlohmann::json getOperationMap(privileges::EntityTag tag);

  private:
    explicit Privileges(const std::bitset<privileges::maxPrivilegeCount>& p) :
        privilegeBitset{p}
    {}

    std::bitset<privileges::maxPrivilegeCount> privilegeBitset = 0;

    /** @brief A vector of all privilege names (Base + OEM)
     *  The index of the vector implies their
     */
    static std::vector<std::string> privilegeNames;

    // Mappings of the current PrivilegeRegistery
    static Mappings mappings;

    const static int basePrivilegeCount;
};

inline const Privileges& getUserPrivileges(const std::string& userRole)
{
    // Redfish privilege : Administrator
    if (userRole == "priv-admin")
    {
        static Privileges admin{"Login", "ConfigureManager", "ConfigureSelf",
                                "ConfigureUsers", "ConfigureComponents"};
        return admin;
    }
    if (userRole == "priv-operator")
    {
        // Redfish privilege : Operator
        static Privileges op{"Login", "ConfigureSelf", "ConfigureComponents"};
        return op;
    }
    if (userRole == "priv-user")
    {
        // Redfish privilege : Readonly
        static Privileges readOnly{"Login", "ConfigureSelf"};
        return readOnly;
    }
    // Redfish privilege : NoAccess
    static Privileges noaccess;
    return noaccess;
}

/* @brief Checks if user is allowed to call an operation
 *
 * @param[in] operationPrivilegesRequired   Privileges required
 * @param[in] userPrivileges                Privileges the user has
 *
 * @return                 True if operation is allowed, false otherwise
 */
inline bool isOperationAllowedWithPrivileges(
    const std::vector<Privileges>& operationPrivilegesRequired,
    const Privileges& userPrivileges)
{
    // If there are no privileges assigned, there are no privileges required
    if (operationPrivilegesRequired.empty())
    {
        return true;
    }
    for (const auto& requiredPrivileges : operationPrivilegesRequired)
    {
        BMCWEB_LOG_DEBUG << "Checking operation privileges...";
        if (userPrivileges.isSupersetOf(requiredPrivileges))
        {
            BMCWEB_LOG_DEBUG << "...success";
            return true;
        }
    }
    return false;
}

} // namespace redfish
