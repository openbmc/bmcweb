#include "health_class_decl.hpp"
#include "../../http/logging.hpp"
#include "../../include/dbus_singleton.hpp"

namespace redfish {

HealthPopulate::HealthPopulate(const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn) :
        asyncResp(asyncRespIn), jsonStatus(asyncResp->res.jsonValue["Status"]) {}

HealthPopulate::HealthPopulate(const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn,
                   nlohmann::json& status) :
        asyncResp(asyncRespIn),
        jsonStatus(status) {}

HealthPopulate::~HealthPopulate()
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
                auto assocIt =
                    interfaces.find("xyz.openbmc_project.Association");
                if (assocIt == interfaces.end())
                {
                    continue;
                }
                auto endpointsIt = assocIt->second.find("endpoints");
                if (endpointsIt == assocIt->second.end())
                {
                    BMCWEB_LOG_ERROR << "Illegal association at "
                                        << path.str;
                    continue;
                }
                const std::vector<std::string>* endpoints =
                    std::get_if<std::vector<std::string>>(
                        &endpointsIt->second);
                if (endpoints == nullptr)
                {
                    BMCWEB_LOG_ERROR << "Illegal association at "
                                        << path.str;
                    continue;
                }
                bool containsChild = false;
                for (const std::string& endpoint : *endpoints)
                {
                    if (std::find(inventory.begin(), inventory.end(),
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

void HealthPopulate::populate()
{
    if (populated)
    {
        return;
    }
    populated = true;
    getAllStatusAssociations();
    getGlobalPath();
}

void HealthPopulate::getGlobalPath()
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

void HealthPopulate::getAllStatusAssociations()
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

}