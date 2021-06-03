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

#include <functional>

namespace redfish
{

#ifndef BMCWEB_ENABLE_REDFISH_ONE_CHASSIS
template <typename CallbackFunc>
void getMainChassisId(std::shared_ptr<bmcweb::AsyncResp> asyncResp,
                      CallbackFunc&& callback)
{
    // Find managed chassis
    crow::connections::systemBus->async_method_call(
        [callback,
         asyncResp](const boost::system::error_code ec,
                    const crow::openbmc_mapper::GetSubTreeType& subtree) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << ec;
                return;
            }
            if (subtree.size() == 0)
            {
                BMCWEB_LOG_DEBUG << "Can't find chassis!";
                return;
            }

            std::size_t idPos = subtree[0].first.rfind('/');
            if (idPos == std::string::npos ||
                (idPos + 1) >= subtree[0].first.size())
            {
                messages::internalError(asyncResp->res);
                BMCWEB_LOG_DEBUG << "Can't parse chassis ID!";
                return;
            }
            std::string chassisId = subtree[0].first.substr(idPos + 1);
            BMCWEB_LOG_DEBUG << "chassisId = " << chassisId;
            callback(chassisId, asyncResp);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0,
        std::array<const char*, 2>{
            "xyz.openbmc_project.Inventory.Item.Board",
            "xyz.openbmc_project.Inventory.Item.Chassis"});
}
#endif

/**
 * @brief Get the chassis is related to the resource.
 *
 * The chassis related to the resoruce is determined by the compare function.
 * Once it find the chass, it will call the callback function.
 *
 * @param[in,out]   asyncResp    Async HTTP response.
 * @param[in]       compare      Function to find the related chassis
 * @param[in]       callback     Function to call once related chassis is found
 * @param[in]       cleanup      Function to call if cannot find related chassis
 */
void getChassisId(
    std::shared_ptr<bmcweb::AsyncResp> asyncResp,
    const std::function<bool(const std::string&)> compare,
    const std::function<void(const std::string&,
                             const std::shared_ptr<bmcweb::AsyncResp>&)>
        callback,
    const std::function<void()> cleanup = nullptr)
{
    // Find managed chassis
    crow::connections::systemBus->async_method_call(
        [callback, compare, cleanup,
         asyncResp](const boost::system::error_code ec,
                    const crow::openbmc_mapper::GetSubTreeType& subtree) {
            auto cleanupHelper = [cleanup]() {
                if (!cleanup)
                {
                    return;
                }

                cleanup();
            };

            if (ec)
            {
                BMCWEB_LOG_ERROR << ec;
                cleanupHelper();
                return;
            }
            if (subtree.size() == 0)
            {
                BMCWEB_LOG_DEBUG << "Can't find chassis!";
                cleanupHelper();
                return;
            }

            auto chassis = std::find_if(subtree.begin(), subtree.end(),
                                        [compare](const auto& object) {
                                            return compare(object.first);
                                        });

            if (chassis == subtree.end())
            {
                BMCWEB_LOG_DEBUG << "Can't find chassis!";
                cleanupHelper();
                return;
            }

            std::string chassisId =
                sdbusplus::message::object_path(chassis->first).filename();
            BMCWEB_LOG_DEBUG << "chassisId = " << chassisId;
            callback(chassisId, asyncResp);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0,
        std::array<const char*, 2>{
            "xyz.openbmc_project.Inventory.Item.Board",
            "xyz.openbmc_project.Inventory.Item.Chassis"});
}

/**
 * @brief Check if the child path is a subpath of the parent path
 *
 * @param[in]  child   D-bus path that is expected to be a subpath of parent
 * @param[in]  parent  D-bus path that is expected to be a superpath of child
 *
 * @return true if child is a subpath of parent, othwerwise, return false.
 */
bool validSubpath(const std::string& child, const std::string& parent)
{
    sdbusplus::message::object_path path(child);
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

} // namespace redfish
