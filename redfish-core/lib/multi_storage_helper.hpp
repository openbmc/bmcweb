#pragma once

#include "openbmc_dbus_rest.hpp"

#include <app.hpp>

#include <functional>

/**
 * @brief Retrieves resources over dbus to link to the chassis
 *
 * @param[in] aResp       - Shared pointer for completing asynchronous calls.
 * @param[in] chassisPath  - Chassis dbus path to look for the storage.
 * @param[in] resoruce     - Resource to link to the chassis
 * @param[in] resourceURI  - Resource URI to add the resource
 * @param[in] interfaces   - List of interfaces to constrain the GetSubTree
 * search
 *
 * @return None.
 */
inline void getChassisResources(std::shared_ptr<bmcweb::AsyncResp> aResp,
                                const std::string& chassisPath,
                                const std::string& resource,
                                const std::string& resourceURI,
                                const std::vector<const char*>& interfaces)
{
    crow::connections::systemBus->async_method_call(
        [aResp{std::move(aResp)}, chassisPath, resource,
         resourceURI](const boost::system::error_code ec,
                      const std::vector<std::string>& objects) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                return;
            }

            nlohmann::json& resources = aResp->res.jsonValue["Links"][resource];
            resources = nlohmann::json::array();
            auto& count =
                aResp->res.jsonValue["Links"][resource + "@odata.count"];
            count = 0;

            for (const auto& object : objects)
            {
                sdbusplus::message::object_path path(object);
                std::string leaf = path.filename();
                if (leaf.empty())
                {
                    continue;
                }

                resources.push_back({{"@odata.id", resourceURI + leaf}});
            }
            count = resources.size();
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths", chassisPath, 0,
        interfaces);
}

/**
 * @brief Check if the child path is a subpath of the parent path
 *
 * @param[in]  child   D-bus path that is expected to be a subpath of parent
 * @param[in]  parent  D-bus path that is expected to be a superpath of child
 *
 * @return true if child is a subpath of parent, otherwise, return false.
 */
bool validSubpath(std::string_view child, std::string_view parent)
{
    sdbusplus::message::object_path path(child.data());
    while (!path.str.empty() && !parent.empty() &&
           path.str.size() >= parent.size())
    {
        if (path.str == parent)
        {
            return true;
        }

        path = path.parent_path();
    }
    return false;
}

/**
 * @brief Find the ChassidId in the ObjectMapper Subtree Outputs
 *
 * @param[in]   ec       Error code
 * @param[in]   subtree  Object Mapper subtree
 * @param[in]   path     Object path of the resource
 */
inline std::optional<std::string>
    findChassidId(const boost::system::error_code ec,
                  const crow::openbmc_mapper::GetSubTreeType& subtree,
                  const std::string& path)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR << ec;
        return std::nullopt;
    }
    if (subtree.size() == 0)
    {
        BMCWEB_LOG_DEBUG << "Can't find chassis!";
        return std::nullopt;
    }

    auto chassis = std::find_if(subtree.begin(), subtree.end(),
                                [path](const auto& object) {
                                    return validSubpath(path, object.first);
                                });
    if (chassis == subtree.end())
    {
        BMCWEB_LOG_DEBUG << "Can't find chassis!";
        return std::nullopt;
    }

    std::string chassisId =
        sdbusplus::message::object_path(chassis->first).filename();
    BMCWEB_LOG_DEBUG << "chassisId = " << chassisId;

    return chassisId;
}

/**
 * @brief Get the chassis is related to the resource.
 *
 * The chassis related to the resource is determined by the compare function.
 * Once it find the chassis, it will call the callback function.
 *
 * @param[in,out]   asyncResp    Async HTTP response
 * @param[in]       path         Object path of the current resource
 * @param[in]       callback     Function to call once related chassis is found
 */
void getChassisId(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& path,
    const std::function<void(const std::optional<std::string>&)>& callback)
{
    // Find managed chassis
    crow::connections::systemBus->async_method_call(
        [asyncResp, path,
         callback](const boost::system::error_code ec,
                   const crow::openbmc_mapper::GetSubTreeType& subtree) {
            auto chassidId = findChassidId(ec, subtree, path);
            callback(chassidId);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0,
        std::array<const char*, 2>{
            "xyz.openbmc_project.Inventory.Item.Board",
            "xyz.openbmc_project.Inventory.Item.Chassis"});
}
