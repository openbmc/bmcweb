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

namespace redfish {

/**
 * @brief  Class used to store privileges for a given user.
 */
class UserPrivileges {
  // TODO: Temporary stub, implementation will come with next patch-sets
 private:
  uint32_t redfishPrivileges;
  uint32_t oemPrivileges;
};

/**
 * @brief  Class used to store privileges for a given Redfish entity.
 */
class EntityPrivileges {
  // TODO: Temporary stub, implementation will come with next patch-sets
 public:
  bool isMethodAllowed(const crow::HTTPMethod& method,
                       const std::string& username) const {
    return true;
  }
};

/**
 * @brief  Class used to:
 *         -  read the PrivilegeRegistry file,
 *         -  provide EntityPrivileges objects to callers.
 *
 *         To save runtime memory object of this class should
 *         exist only for the time required to install all Nodes.
 */
class PrivilegeProvider {
  // TODO: Temporary stub, implementation will come with next patch-sets
 public:
  PrivilegeProvider() {
    // load privilege_registry.json to memory
  }

  EntityPrivileges getPrivileges(const std::string& entity_url,
                                 const std::string& entity_type) const {
    // return an entity privilege object based on the privilege_registry.json,
    // currently returning default constructed object
    return EntityPrivileges();
  }
};

}  // namespace redfish

