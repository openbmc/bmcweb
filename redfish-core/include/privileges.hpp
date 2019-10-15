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

#include <crow/logging.h>

#include <array>
#include <bitset>
#include <boost/beast/http/verb.hpp>
#include <boost/container/flat_map.hpp>
#include <cstdint>
#include <vector>

namespace redfish
{

enum class PrivilegeType
{
    BASE,
    OEM
};

/** @brief A fixed array of compile time privileges  */
constexpr std::array<const char*, 5> basePrivileges{
    "Login", "ConfigureManager", "ConfigureComponents", "ConfigureSelf",
    "ConfigureUsers"};

constexpr const int basePrivilegeCount = basePrivileges.size();

/** @brief Max number of privileges per type  */
constexpr const int maxPrivilegeCount = 32;

/** @brief A vector of all privilege names and their indexes */
static const std::vector<std::string> privilegeNames{basePrivileges.begin(),
                                                     basePrivileges.end()};

/**
 * @brief Redfish privileges
 *
 *        Entity privileges and user privileges are represented by this class.
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
 *        This does not implement any Redfish property overrides,
 *        subordinate overrides, or resource URI overrides.  This does
 *        not implement the limitation of the ConfigureSelf privilege
 *        to operate only on your own account or session.
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
    Privileges(std::initializer_list<const char*> privilegeList)
    {
        for (const char* privilege : privilegeList)
        {
            if (!setSinglePrivilege(privilege))
            {
                BMCWEB_LOG_CRITICAL << "Unable to set privilege " << privilege
                                    << "in constructor";
            }
        }
    }

    /**
     * @brief Resets the given privilege in the bitset
     *
     * @param[in] privilege  Privilege to be reset
     *
     * @return               None
     *
     */
    bool resetSinglePrivilege(const char* privilege)
    {
        for (int searchIndex = 0; searchIndex < privilegeNames.size();
             searchIndex++)
        {
            if (privilege == privilegeNames[searchIndex])
            {
                privilegeBitset.reset(searchIndex);
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
    bool setSinglePrivilege(const char* privilege)
    {
        for (int searchIndex = 0; searchIndex < privilegeNames.size();
             searchIndex++)
        {
            if (privilege == privilegeNames[searchIndex])
            {
                privilegeBitset.set(searchIndex);
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
    bool setSinglePrivilege(const std::string& privilege)
    {
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
    std::vector<const std::string*>
        getActivePrivilegeNames(const PrivilegeType type) const
    {
        std::vector<const std::string*> activePrivileges;

        int searchIndex = 0;
        int endIndex = basePrivilegeCount;
        if (type == PrivilegeType::OEM)
        {
            searchIndex = basePrivilegeCount - 1;
            endIndex = privilegeNames.size();
        }

        for (; searchIndex < endIndex; searchIndex++)
        {
            if (privilegeBitset.test(searchIndex))
            {
                activePrivileges.emplace_back(&privilegeNames[searchIndex]);
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
    bool isSupersetOf(const Privileges& p) const
    {
        return (privilegeBitset & p.privilegeBitset) == p.privilegeBitset;
    }

    std::string to_string() const
    {
        return privilegeBitset.to_string();
    }

  private:
    std::bitset<maxPrivilegeCount> privilegeBitset = 0;
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
    else if (userRole == "priv-operator")
    {
        // Redfish privilege : Operator
        static Privileges op{"Login", "ConfigureSelf", "ConfigureComponents"};
        return op;
    }
    else if (userRole == "special-priv-configure-self")
    {
        // Redfish privilege : N/A - internal within BMCWeb
        static Privileges configSelf{"ConfigureSelf"};
        return configSelf;
    }
    else
    {
        // Redfish privilege : Readonly
        static Privileges readOnly{"Login", "ConfigureSelf"};
        return readOnly;
    }
}

using OperationMap = boost::container::flat_map<boost::beast::http::verb,
                                                std::vector<Privileges>>;

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
    for (auto& requiredPrivileges : operationPrivilegesRequired)
    {
        BMCWEB_LOG_ERROR << "Checking operation privileges...";
        if (userPrivileges.isSupersetOf(requiredPrivileges))
        {
            BMCWEB_LOG_ERROR << "...success";
            return true;
        }
    }
    return false;
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
inline bool isMethodAllowedWithPrivileges(const boost::beast::http::verb method,
                                          const OperationMap& operationMap,
                                          const Privileges& userPrivileges)
{
    const auto& it = operationMap.find(method);
    if (it == operationMap.end())
    {
        return false;
    }

    return isOperationAllowedWithPrivileges(it->second, userPrivileges);
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
inline bool isMethodAllowedForUser(const boost::beast::http::verb method,
                                   const OperationMap& operationMap,
                                   const std::string& user)
{
    // TODO: load user privileges from configuration as soon as its available
    // now we are granting all privileges to everyone.
    Privileges userPrivileges{"Login", "ConfigureManager", "ConfigureSelf",
                              "ConfigureUsers", "ConfigureComponents"};

    return isMethodAllowedWithPrivileges(method, operationMap, userPrivileges);
}

} // namespace redfish
