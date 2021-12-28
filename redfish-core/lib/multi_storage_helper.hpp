#pragma once

// Will remove this file once it get closer to being merged.

#include "error_messages.hpp"
#include "openbmc_dbus_rest.hpp"

#include <app.hpp>

#include <functional>

/**
 * @brief Retrieves resources over dbus to link to the chassis
 *
 * @param[in] aResp       - Shared pointer for completing asynchronous calls.
 * @param[in] associationPath  - Chassis dbus path to look for the storage.
 * @param[in] resoruce     - Resource to link to the chassis
 * @param[in] resourceURI  - Resource URI to add the resource
 * @param[in] interfaces   - List of interfaces to constrain the GetSubTree
 * search
 *
 * @return None.
 */
inline void getChassisResources(std::shared_ptr<bmcweb::AsyncResp> aResp,
                                const std::string& associationPath,
                                const std::string& resource,
                                const std::string& resourceURI)
{
    sdbusplus::asio::getProperty<std::vector<std::string>>(
        *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper",
        associationPath, "xyz.openbmc_project.Association", "endpoints",
        [aResp, resource,
         resourceURI](const boost::system::error_code ec,
                      const std::vector<std::string>& resourceList) {
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
            for (const std::string& resource : resourceList)
            {
                sdbusplus::message::object_path path(resource);
                std::string leaf = path.filename();
                if (leaf.empty())
                {
                    continue;
                }

                resources.push_back({{"@odata.id", resourceURI + leaf}});
            }
            count = resources.size();
        });
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
void getChassisId(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                  const std::string& path,
                  const std::function<void(const std::string&)>& callback)
{
    sdbusplus::asio::getProperty<std::vector<std::string>>(
        *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper",
        path + "/chassis", "xyz.openbmc_project.Association", "endpoints",
        [asyncResp, path,
         callback](const boost::system::error_code ec,
                   const std::vector<std::string>& chassisList) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << path << " has no Chassis association";
                return;
            }

            if (chassisList.size() != 1)
            {
                BMCWEB_LOG_ERROR
                    << "getChassisId require it to be only one Chassis";
                redfish::messages::internalError(asyncResp->res);
                return;
            }
            const std::string chassisId =
                sdbusplus::message::object_path(chassisList[0]).filename();
            crow::connections::systemBus->async_method_call(
                [asyncResp, chassisId, callback](
                    const boost::system::error_code ec,
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG << "Can't find chassis!";
                        return;
                    }

                    for (const std::pair<std::string,
                                         dbus::utility::MapperServiceMap>&
                             object : subtree)
                    {
                        if (sdbusplus::message::object_path(object.first)
                                .filename() == chassisId)
                        {
                            callback(chassisId);
                            return;
                        }
                    }
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                "/xyz/openbmc_project/inventory", 0,
                std::array<const char*, 2>{
                    "xyz.openbmc_project.Inventory.Item.Board",
                    "xyz.openbmc_project.Inventory.Item.Chassis"});
        });
}
