/*
 * Copyright (c) 2021 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "bmcweb_config.h"

#include <app.hpp>
#include <boost/container/flat_map.hpp>
#include <utils/fw_utils.hpp>

#include <variant>

namespace redfish
{

// Only allow one config change at a time
static bool vrConfigChangeInProgress = false;
static constexpr size_t maxVrConfigSize = 1024 * 1024;

static inline void
    fetchConfigLogs(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ecIn,
                    const std::string& vrConfigOutputFile) {
            if (ecIn)
            {
                BMCWEB_LOG_ERROR << "DBUS response error " << ecIn;
                messages::internalError(asyncResp->res);
                return;
            }
            std::ifstream vrResults(vrConfigOutputFile);

            nlohmann::json fileContents =
                nlohmann::json::parse(vrResults, nullptr, false);
            std::filesystem::remove(vrConfigOutputFile);
            if (fileContents.is_discarded())
            {
                messages::internalError(asyncResp->res);
                return;
            }
            asyncResp->res.jsonValue = std::move(fileContents);
            return;
        },
        "xyz.openbmc_project.VR_Manager", "/xyz/openbmc_project/VR_Manager",
        "xyz.openbmc_project.VR_Manager", "ReadConfig");
}

static inline void getStatusBeforeConfigLog(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ecIn,
                    std::variant<std::string>& status) {
            if (ecIn)
            {
                BMCWEB_LOG_ERROR << "DBUS response error " << ecIn;
                messages::internalError(asyncResp->res);
                return;
            }
            if (auto statusp = std::get_if<std::string>(&status);
                statusp && *statusp != "idle")
            {
                messages::serviceTemporarilyUnavailable(asyncResp->res, "30");
                return;
            }
            fetchConfigLogs(asyncResp);
        },
        "xyz.openbmc_project.VR_Manager", "/xyz/openbmc_project/VR_Manager",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.VR_Manager", "status");
}

inline void requestRoutesIntelVoltageRegulators(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Oem/VoltageRegulators/")
        .privileges(redfish::privileges::getVoltageRegulators)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#VoltageRegulators.v1_5_0.VoltageRegulators";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Systems/system/Oem/VoltageRegulators";
                asyncResp->res.jsonValue["Id"] = "VoltageRegulators";
                asyncResp->res.jsonValue["Description"] =
                    "Service for voltage regulator configuration";
                asyncResp->res.jsonValue["Name"] =
                    "Voltage Regulator Config Service";
                asyncResp->res.jsonValue["HttpPushUri"] =
                    "/redfish/v1/Systems/system/Oem/VoltageRegulators";
                // VoltageRegulators cannot be disabled
                asyncResp->res.jsonValue["ServiceEnabled"] = true;
                asyncResp->res.jsonValue["ConfigChangInProgress"] =
                    vrConfigChangeInProgress;
                vrConfigChangeInProgress = false;
                // Get the MaxImageSizeBytes
                asyncResp->res.jsonValue["MaxImageSizeBytes"] = maxVrConfigSize;
                asyncResp->res.jsonValue["ConfigLog"] = {
                    {"@odata.id", "/redfish/v1/Systems/system/Oem/"
                                  "VoltageRegulators/ConfigLog"}};

                crow::connections::systemBus->async_method_call(
                    [asyncResp](
                        const boost::system::error_code ec,
                        const std::map<std::string,
                                       std::variant<int, bool, std::string>>&
                            props) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "DBUS response error " << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        // publish all properties directly
                        for (auto&& [key, value] : props)
                        {
                            std::visit(
                                [&key, &asyncResp](auto&& value) {
                                    using T = std::decay_t<decltype(value)>;
                                    if constexpr (std::is_same_v<T, int> ||
                                                  std::is_same_v<T, bool> ||
                                                  std::is_same_v<T,
                                                                 std::string>)
                                    {
                                        asyncResp->res.jsonValue[key] = value;
                                    }
                                    else
                                    {
                                        // ignore unsupported property types
                                    }
                                },
                                value);
                        }
                    },
                    "xyz.openbmc_project.VR_Manager",
                    "/xyz/openbmc_project/VR_Manager",
                    "org.freedesktop.DBus.Properties", "GetAll",
                    "xyz.openbmc_project.VR_Manager");
            });

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/system/Oem/VoltageRegulators/ConfigLog")
        .privileges(redfish::privileges::getVoltageRegulators)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                getStatusBeforeConfigLog(asyncResp);
            });

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Oem/VoltageRegulators/")
        .privileges(redfish::privileges::putVoltageRegulators)
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                BMCWEB_LOG_ERROR << "doPost...";

                if (vrConfigChangeInProgress)
                {
                    messages::serviceTemporarilyUnavailable(asyncResp->res,
                                                            "30");
                    return;
                }

                if (req.body.size() > maxVrConfigSize)
                {
                    redfish::messages::invalidUpload(
                        asyncResp->res,
                        "/redfish/v1/Systems/system/Oem/VoltageRegulators/",
                        "image too large");
                    return;
                }

                nlohmann::json j =
                    nlohmann::json::parse(req.body, nullptr, false);
                if (j.is_discarded())
                {
                    redfish::messages::invalidUpload(
                        asyncResp->res,
                        "/redfish/v1/Systems/system/Oem/VoltageRegulators/",
                        "Config must be JSON format");
                    return;
                }
                vrConfigChangeInProgress = true;

                std::string filepath("/tmp/vrconfig-tmpXXXXXX");
                int fd = mkstemp(filepath.data());
                if (fd < 0)
                {
                    redfish::messages::internalError(asyncResp->res);
                    return;
                }
                BMCWEB_LOG_ERROR << "Writing file to " << filepath;
                std::ofstream out(filepath, std::ofstream::out |
                                                std::ofstream::binary |
                                                std::ofstream::trunc);
                out << req.body;
                out.flush();
                out.close();
                close(fd);

                // async request to process config
                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code ec) {
                        if (ec)
                        {
                            // FIXME: actually return an error here
                            asyncResp->res.jsonValue["DBusError"] =
                                ec.message();
                            BMCWEB_LOG_ERROR << "DBUS response error " << ec;
                            // messages::internalError(asyncResp->res);
                            // return;
                        }
                        asyncResp->res.jsonValue["Status"] = "Processing";
                    },

                    "xyz.openbmc_project.VR_Manager",
                    "/xyz/openbmc_project/VR_Manager",
                    "xyz.openbmc_project.VR_Manager", "SetConfig", filepath);
                BMCWEB_LOG_ERROR << "file upload complete!!";
            });
}

} // namespace redfish
