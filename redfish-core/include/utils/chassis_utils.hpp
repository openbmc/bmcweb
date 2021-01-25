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

#include <node.hpp>

namespace redfish
{

namespace chassis_utils
{

/**
 * @brief Retrieves valid chassis path
 * @param asyncResp   Pointer to object holding response data
 * @param callback  Callback for next step to get valid chassis path
 */
template <typename Callback>
void getValidChassisPath(const std::shared_ptr<AsyncResp>& asyncResp,
                         const std::string& chassisID, Callback&& callback)
{
    BMCWEB_LOG_DEBUG << "checkChassisId enter";
    const std::array<const char*, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Board",
        "xyz.openbmc_project.Inventory.Item.Chassis"};

    auto respHandler =
        [callback{std::move(callback)}, asyncResp,
         chassisID](const boost::system::error_code ec,
                    const std::vector<std::string>& chassisPaths) mutable {
            BMCWEB_LOG_DEBUG << "getValidChassisPath respHandler enter";
            if (ec)
            {
                BMCWEB_LOG_ERROR
                    << "getValidChassisPath respHandler DBUS error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }

            std::optional<std::string> chassisPath;
            std::string chassisName;
            for (const std::string& chassis : chassisPaths)
            {
                sdbusplus::message::object_path path(chassis);
                chassisName = path.filename();
                if (chassisName.empty())
                {
                    BMCWEB_LOG_ERROR << "Failed to find '/' in " << chassis;
                    continue;
                }
                if (chassisName == chassisID)
                {
                    chassisPath = chassis;
                    break;
                }
            }
            callback(chassisPath);
        };

    // Get the Chassis Collection
    crow::connections::systemBus->async_method_call(
        respHandler, "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/inventory", 0, interfaces);
    BMCWEB_LOG_DEBUG << "checkChassisId exit";
}

} // namespace chassis_utils
} // namespace redfish
