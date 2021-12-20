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

#include <app.hpp>
#include <registries/privilege_registry.hpp>

namespace redfish
{
inline void
    getMemoryEnvironmentMetricsData(std::shared_ptr<bmcweb::AsyncResp> aResp,
                                    const std::string& dimmId)
{
    using DimmProperty =
        std::variant<std::string, std::vector<uint32_t>, std::vector<uint16_t>,
                     uint64_t, uint32_t, uint16_t, uint8_t, bool>;

    using DimmProperties =
        boost::container::flat_map<std::string, DimmProperty>;

    BMCWEB_LOG_DEBUG << "Get available system dimm resources.";
    crow::connections::systemBus->async_method_call(
        [dimmId, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const boost::container::flat_map<
                std::string, boost::container::flat_map<
                                 std::string, std::vector<std::string>>>&
                subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);

                return;
            }
            bool found = false;
            for (const auto& [path, object] : subtree)
            {
                if (path.find(dimmId) != std::string::npos)
                {
                    for (const auto& [service, interfaces] : object)
                    {
                        if (!found &&
                            (std::find(
                                 interfaces.begin(), interfaces.end(),
                                 "xyz.openbmc_project.Inventory.Item.Dimm") !=
                             interfaces.end()))
                        {
                            BMCWEB_LOG_DEBUG
                                << "Get available system components.";
                            crow::connections::systemBus->async_method_call(
                                [dimmId,
                                 aResp](const boost::system::error_code ec,
                                        const DimmProperties& properties) {
                                    if (ec)
                                    {
                                        BMCWEB_LOG_DEBUG
                                            << "DBUS response error";
                                        messages::internalError(aResp->res);

                                        return;
                                    }
                                    for (const auto& property : properties)
                                    {
                                        if (property.first ==
                                            "MediaTemperature")
                                        {
                                            auto* value = std::get_if<uint16_t>(
                                                &property.second);
                                            if (value == nullptr)
                                            {
                                                continue;
                                            }
                                            aResp->res
                                                .jsonValue["TemperatureCelsius"]
                                                          ["Reading"] = *value;
                                        }
                                    }
                                },
                                service, path,
                                "org.freedesktop.DBus.Properties", "GetAll",
                                "");

                            found = true;
                        }
                    }
                }
            }
            // Object not found
            if (!found)
            {
                messages::resourceNotFound(aResp->res, "Memory", dimmId);
            }
            return;
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0,
        std::array<const char*, 2>{
            "xyz.openbmc_project.Inventory.Item.Dimm",
            "xyz.openbmc_project.Inventory.Item.PersistentMemory.Partition"});
}

inline void requestRoutesMemoryEnvironmentMetrics(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/system/Memory/<str>/EnvironmentMetrics")
        .privileges({{"Login"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& dimmId) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#EnvironmentMetrics.v1_1_0.EnvironmentMetrics";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Systems/system/Memory/" + dimmId +
                    "/EnvironmentMetrics";
                asyncResp->res.jsonValue["Name"] =
                    "Environment Metrics for Memory";
                asyncResp->res.jsonValue["Description"] =
                    "Environment Metrics for Memory";
                asyncResp->res.jsonValue["Id"] =
                    "Environment Metrics for Memory for " + dimmId;
                asyncResp->res.jsonValue["TemperatureCelsius"]["ReadingType"] =
                    "Temperature";
                asyncResp->res.jsonValue["TemperatureCelsius"]["ReadingUnits"] =
                    "Cel";

                getMemoryEnvironmentMetricsData(asyncResp, dimmId);
            });
}

} // namespace redfish