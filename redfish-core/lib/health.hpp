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
#include <dbus_utility.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>

#include <variant>

namespace redfish
{
struct HealthPopulate : std::enable_shared_from_this<HealthPopulate>
{
    HealthPopulate(const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn) :
        asyncResp(asyncRespIn), jsonStatus(asyncResp->res.jsonValue["Status"])
    {}

    HealthPopulate(const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn,
                   nlohmann::json& status) :
        asyncResp(asyncRespIn),
        jsonStatus(status)
    {}

    HealthPopulate(const HealthPopulate&) = delete;
    HealthPopulate(HealthPopulate&&) = delete;
    HealthPopulate& operator=(const HealthPopulate&) = delete;
    HealthPopulate& operator=(const HealthPopulate&&) = delete;

    ~HealthPopulate()
    {
        nlohmann::json& health = jsonStatus["Health"];
        nlohmann::json& rollup = jsonStatus["HealthRollup"];

        health = "OK";
        rollup = "OK";

        for (const std::shared_ptr<HealthPopulate>& healthChild : children)
        {
            healthChild->globalInventoryPath = globalInventoryPath;
            healthChild->statuses = statuses;
        }

        for (const auto& [path, interfaces] : statuses)
        {
            bool isChild = false;
            bool isSelf = false;
            if (selfPath)
            {
                if (boost::equals(path.str, *selfPath) ||
                    boost::starts_with(path.str, *selfPath + "/"))
                {
                    isSelf = true;
                }
            }

            // managers inventory is all the inventory, don't skip any
            if (!isManagersHealth && !isSelf)
            {

                // We only want to look at this association if either the path
                // of this association is an inventory item, or one of the
                // endpoints in this association is a child

                for (const std::string& child : inventory)
                {
                    if (boost::starts_with(path.str, child))
                    {
                        isChild = true;
                        break;
                    }
                }
                if (!isChild)
                {
                    for (const auto& [interface, association] : interfaces)
                    {
                        if (interface != "xyz.openbmc_project.Association")
                        {
                            continue;
                        }
                        for (const auto& [name, value] : association)
                        {
                            if (name != "endpoints")
                            {
                                continue;
                            }

                            const std::vector<std::string>* endpoints =
                                std::get_if<std::vector<std::string>>(&value);
                            if (endpoints == nullptr)
                            {
                                BMCWEB_LOG_ERROR << "Illegal association at "
                                                 << path.str;
                                continue;
                            }
                            bool containsChild = false;
                            for (const std::string& endpoint : *endpoints)
                            {
                                if (std::find(inventory.begin(),
                                              inventory.end(),
                                              endpoint) != inventory.end())
                                {
                                    containsChild = true;
                                    break;
                                }
                            }
                            if (!containsChild)
                            {
                                continue;
                            }
                        }
                    }
                }
            }

            if (boost::starts_with(path.str, globalInventoryPath) &&
                boost::ends_with(path.str, "critical"))
            {
                health = "Critical";
                rollup = "Critical";
                return;
            }
            if (boost::starts_with(path.str, globalInventoryPath) &&
                boost::ends_with(path.str, "warning"))
            {
                health = "Warning";
                if (rollup != "Critical")
                {
                    rollup = "Warning";
                }
            }
            else if (boost::ends_with(path.str, "critical"))
            {
                rollup = "Critical";
                if (isSelf)
                {
                    health = "Critical";
                    return;
                }
            }
            else if (boost::ends_with(path.str, "warning"))
            {
                if (rollup != "Critical")
                {
                    rollup = "Warning";
                }

                if (isSelf)
                {
                    health = "Warning";
                }
            }
        }
    }

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
                   const dbus::utility::MapperGetSubTreePathsResponse& resp) {
                if (ec || resp.size() != 1)
                {
                    // no global item, or too many
                    return;
                }
                self->globalInventoryPath = resp[0];
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
                   const dbus::utility::ManagedObjectType& resp) {
                if (ec)
                {
                    return;
                }
                self->statuses = resp;
                for (auto it = self->statuses.begin();
                     it != self->statuses.end();)
                {
                    if (boost::ends_with(it->first.str, "critical") ||
                        boost::ends_with(it->first.str, "warning"))
                    {
                        it++;
                        continue;
                    }
                    it = self->statuses.erase(it);
                }
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

class HealthRollup
{
    std::string objPath;
    sdbusplus::bus::bus bus;
    uint64_t dbusTimeoutUs;

    static std::string
        translateDbusHealthStateToRedfish(const std::string& state)
    {
        if (state == "xyz.openbmc_project.State.Decorator.Status.HealthType.OK")
        {
            return "OK";
        }
        if (state ==
            "xyz.openbmc_project.State.Decorator.Status.HealthType.Warning")
        {
            return "Warning";
        }
        if (state ==
            "xyz.openbmc_project.State.Decorator.Status.HealthType.Critical")
        {
            return "Critical";
        }
        return "";
    }

    std::string getService(const std::string& objPath)
    {
        try
        {
            sdbusplus::message::message method;
            sdbusplus::message::message reply;
            int err = 0;
            std::map<std::string, std::vector<std::string>> objInfo;
            std::vector<std::string> services;

            method = bus.new_method_call("xyz.openbmc_project.ObjectMapper",
                                         "/xyz/openbmc_project/object_mapper",
                                         "xyz.openbmc_project.ObjectMapper",
                                         "GetObject");
            method.append(objPath,
                          std::vector<std::string>{
                              "xyz.openbmc_project.State.Decorator.Status"});
            reply = bus.call(method, dbusTimeoutUs);
            err = reply.get_errno();
            if (err != 0)
            {
                BMCWEB_LOG_DEBUG << "failed to get object of " << objPath
                                 << ", errno=" << err;
                return "";
            }

            reply.read(objInfo);
            services.reserve(objInfo.size());
            for (auto const& [key, value] : objInfo)
            {
                services.push_back(key);
            }
            if (services.size() != 1)
            {
                BMCWEB_LOG_ERROR << "multiple services refer to the object"
                                 << objPath;
                return "";
            }
            return services[0];
        }
        catch (const sdbusplus::exception::exception& e)
        {
            BMCWEB_LOG_ERROR << e.what();
            return "";
        }
    }

    virtual std::shared_ptr<std::vector<std::string>>
        getChildren(const std::string& objPath)
    {
        std::string healthRollupPath = boost::ends_with(objPath, "/")
                                           ? objPath + "health_rollup"
                                           : objPath + "/health_rollup";
        sdbusplus::message::message method;
        sdbusplus::message::message reply;
        int err = 0;

        try
        {
            std::variant<std::vector<std::string>> endpointsVar;
            std::vector<std::string>* endpoints = nullptr;

            method = bus.new_method_call(
                "xyz.openbmc_project.ObjectMapper", healthRollupPath.c_str(),
                "org.freedesktop.DBus.Properties", "Get");
            method.append("xyz.openbmc_project.Association", "endpoints");
            reply = bus.call(method, dbusTimeoutUs);
            err = reply.get_errno();
            if (err != 0)
            {
                BMCWEB_LOG_DEBUG << "failed to get endpoints of " << objPath
                                 << ", errno=" << err;
                return nullptr;
            }

            reply.read(endpointsVar);
            endpoints = std::get_if<std::vector<std::string>>(&endpointsVar);
            if (endpoints == nullptr)
            {
                BMCWEB_LOG_ERROR << "invalid non-string-array endpoints "
                                    "property of "
                                 << objPath;
                return nullptr;
            }
            return std::make_shared<std::vector<std::string>>(*endpoints);
        }
        catch (const sdbusplus::exception::exception& e)
        {
            // Check if the object exists or not
            try
            {
                method = bus.new_method_call(
                    "xyz.openbmc_project.ObjectMapper",
                    "/xyz/openbmc_project/object_mapper",
                    "xyz.openbmc_project.ObjectMapper", "GetObject");
                method.append(objPath, std::vector<std::string>{});
                reply = bus.call(method, dbusTimeoutUs);
                err = reply.get_errno();
                if (err != 0)
                {
                    BMCWEB_LOG_DEBUG << "failed to get object of " << objPath
                                     << ", errno=" << err;
                    return nullptr;
                }
                return std::make_shared<std::vector<std::string>>();
            }
            catch (const sdbusplus::exception::exception& e)
            {
                BMCWEB_LOG_ERROR << e.what();
                return nullptr;
            }
        }
    }

    virtual std::string getHealth(const std::string& objPath)
    {
        std::string service;

        service = getService(objPath);
        if (service.empty())
        {
            return "";
        }

        try
        {
            sdbusplus::message::message method;
            sdbusplus::message::message reply;
            int err = 0;
            std::variant<std::string> healthStateVar;
            std::string* healthState = nullptr;
            std::string newHealth;

            method =
                bus.new_method_call(service.c_str(), objPath.c_str(),
                                    "org.freedesktop.DBus.Properties", "Get");
            method.append("xyz.openbmc_project.State.Decorator.Status",
                          "Health");
            reply = bus.call(method, dbusTimeoutUs);
            err = reply.get_errno();
            if (err != 0)
            {
                BMCWEB_LOG_ERROR << "failed to get health of " << objPath
                                 << ", errno=" << err;
            }
            else
            {
                reply.read(healthStateVar);
                healthState = std::get_if<std::string>(&healthStateVar);
                if (healthState != nullptr)
                {
                    newHealth = translateDbusHealthStateToRedfish(*healthState);
                    BMCWEB_LOG_DEBUG << "health of " << objPath << ": "
                                     << newHealth;
                }
                else
                {
                    BMCWEB_LOG_ERROR << "invalid non-string health property of "
                                     << objPath;
                }
            }
            return newHealth;
        }
        catch (const sdbusplus::exception::exception& e)
        {
            BMCWEB_LOG_ERROR << e.what();
            return "";
        }
    }

  public:
    std::string health;
    std::string healthRollup;
    bool valid;
    bool isLeaf;

    HealthRollup(const std::string& objPathIn,
                 const uint64_t& dbusTimeoutUsIn = 100000,
                 const bool autoUpdate = true) :
        objPath(objPathIn),
        bus(sdbusplus::bus::new_default()), dbusTimeoutUs(dbusTimeoutUsIn),
        valid(false), isLeaf(true)
    {
        if (autoUpdate)
        {
            update();
        }
    }

    virtual ~HealthRollup() = default;
    HealthRollup(const HealthRollup&) = default;
    HealthRollup(HealthRollup&&) = default;
    HealthRollup& operator=(const HealthRollup& r) = default;
    HealthRollup& operator=(HealthRollup&&) = default;

    void update()
    {
        std::shared_ptr<std::vector<std::string>> endpoints;

        valid = false;

        health = getHealth(objPath);
        if (health.empty())
        {
            return;
        }

        endpoints = getChildren(objPath);
        if (endpoints == nullptr)
        {
            return;
        }
        healthRollup = health;

        if (endpoints->empty())
        {
            isLeaf = true;
        }
        else
        {
            isLeaf = false;
            for (auto const& endpoint : *endpoints)
            {
                std::string childHealth = getHealth(endpoint);
                if (childHealth.empty())
                {
                    return;
                }
                if (childHealth == "Critical" && healthRollup != "Critical")
                {
                    healthRollup = "Critical";
                    valid = true;
                    return;
                }
                if (childHealth == "Warning" && healthRollup == "OK")
                {
                    healthRollup = "Warning";
                }
            }
        }

        valid = true;
    }

    bool setStatus(crow::Response& res)
    {
        if (valid)
        {
            res.jsonValue["Status"]["Health"] = health;
            if (!isLeaf)
            {
                res.jsonValue["Status"]["HealthRollup"] = healthRollup;
            }
            return true;
        }

        messages::internalError(res);
        return false;
    }
};

} // namespace redfish
