/*
// Copyright (c) 2019 Intel Corporation
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

#include "async_resp.hpp"

#include <app.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/container/flat_set.hpp>
#include <dbus_singleton.hpp>

#include <variant>

namespace redfish
{

struct HealthPopulate : std::enable_shared_from_this<HealthPopulate>
{
    HealthPopulate(const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn);

    HealthPopulate(const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn,
                   nlohmann::json& status) :
        asyncResp(asyncRespIn),
        jsonStatus(status)
    {}

    ~HealthPopulate();

    // this should only be called once per url, others should get updated by
    // being added as children to the 'main' health object for the page
    void populate()
    {
        if (populated)
        {
            return;
        }
        populated = true;
        getAllStatusAssociations();
        getGlobalPath();
    }

    void getGlobalPath()
    {
        std::shared_ptr<HealthPopulate> self = shared_from_this();
        crow::connections::systemBus->async_method_call(
            [self](const boost::system::error_code ec,
                   std::vector<std::string>& resp) {
                if (ec || resp.size() != 1)
                {
                    // no global item, or too many
                    return;
                }
                self->globalInventoryPath = std::move(resp[0]);
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths", "/", 0,
            std::array<const char*, 1>{
                "xyz.openbmc_project.Inventory.Item.Global"});
    }

    void getAllStatusAssociations()
    {
        std::shared_ptr<HealthPopulate> self = shared_from_this();
        crow::connections::systemBus->async_method_call(
            [self](const boost::system::error_code ec,
                   dbus::utility::ManagedObjectType& resp) {
                if (ec)
                {
                    return;
                }
                for (auto it = resp.begin(); it != resp.end();)
                {
                    if (boost::ends_with(it->first.str, "critical") ||
                        boost::ends_with(it->first.str, "warning"))
                    {
                        it++;
                        continue;
                    }
                    it = resp.erase(it);
                }
                self->statuses = std::move(resp);
            },
            "xyz.openbmc_project.ObjectMapper", "/",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    }

    std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    nlohmann::json& jsonStatus;

    // we store pointers to other HealthPopulate items so we can update their
    // members and reduce dbus calls. As we hold a shared_ptr to them, they get
    // destroyed last, and they need not call populate()
    std::vector<std::shared_ptr<HealthPopulate>> children;

    // self is used if health is for an individual items status, as this is the
    // 'lowest most' item, the rollup will equal the health
    std::optional<std::string> selfPath;

    std::vector<std::string> inventory;
    bool isManagersHealth = false;
    dbus::utility::ManagedObjectType statuses;
    std::string globalInventoryPath = "-"; // default to illegal dbus path
    bool populated = false;
};
} // namespace redfish
