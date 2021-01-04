/*
// Copyright (c) 2018 Intel Corporation
// Copyright (c) 2018 Ampere Computing LLC
/
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

#include "node.hpp"
#include "sensors.hpp"

#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

namespace redfish
{

class Power : public Node
{
  public:
    Power(App& app) :
        Node((app), "/redfish/v1/Chassis/<str>/Power/", std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    static auto handleSetPropertyError(
        const std::shared_ptr<SensorsAsyncResp>& asyncResp)
    {
        return [asyncResp](boost::system::error_code ec) {
            messages::actionNotSupported(asyncResp->res,
                                         std::string("ec: ") + ec.message());
        };
    }

    static auto handleSetPropertySuccess(
        const std::shared_ptr<SensorsAsyncResp>& asyncResp)
    {
        return [asyncResp] {};
    }

    static void patchPowerAllocatedWatts(
        const std::shared_ptr<SensorsAsyncResp>& asyncResp,
        uint16_t powerAllocatedWatts)
    {
        auto handleError = handleSetPropertyError(asyncResp);
        auto handleSuccess = handleSetPropertySuccess(asyncResp);

        sdbusplus::asio::getProperty<uint16_t>(
            *crow::connections::systemBus, "xyz.openbmc_project.NodeManager",
            "/xyz/openbmc_project/NodeManager/Domain/5/Policy/"
            "254",
            "xyz.openbmc_project.NodeManager.PolicyAttributes", "Limit",
            [handleError, handleSuccess,
             powerAllocatedWatts](boost::system::error_code) {
                crow::connections::systemBus->async_method_call(
                    [handleError](boost::system::error_code ec) {
                        if (ec)
                        {
                            handleError(ec);
                        }
                    },
                    "xyz.openbmc_project.NodeManager",
                    "/xyz/openbmc_project/NodeManager/Domain/5",
                    "xyz.openbmc_project.NodeManager.PolicyManager",
                    "CreateWithId", uint8_t{254},
                    std::make_tuple(
                        uint32_t{4065}, powerAllocatedWatts, uint16_t{1},
                        int32_t{1}, int32_t{0}, int32_t{0},
                        std::array<
                            std::map<std::string,
                                     std::variant<std::vector<std::string>,
                                                  std::string>>,
                            0>(),
                        std::array<std::map<std::string, uint32_t>, 0>(),
                        uint8_t{254}, uint16_t{0}, uint8_t{0}));
            },
            [handleError, handleSuccess, powerAllocatedWatts](uint16_t) {
                auto limit = powerAllocatedWatts;
                sdbusplus::asio::setProperty<uint16_t>(
                    *crow::connections::systemBus,
                    "xyz.openbmc_project.NodeManager",
                    "/xyz/openbmc_project/NodeManager/Domain/5/Policy/254",
                    "xyz.openbmc_project.NodeManager.PolicyAttributes", "Limit",
                    std::move(limit), handleError, handleSuccess);
            });
    }

    static void patchOemCray(const std::shared_ptr<SensorsAsyncResp>& asyncResp,
                             nlohmann::json& cray)
    {
        std::optional<uint16_t> powerAllocatedWatts;
        if (!json_util::readJson(cray, asyncResp->res, "PowerAllocatedWatts",
                                 powerAllocatedWatts))
        {
            return;
        }
        if (powerAllocatedWatts)
        {
            patchPowerAllocatedWatts(asyncResp, *powerAllocatedWatts);
        }
    }

    static void patchOem(const std::shared_ptr<SensorsAsyncResp>& asyncResp,
                         nlohmann::json& oem)
    {
        std::optional<nlohmann::json> cray;
        if (!json_util::readJson(oem, asyncResp->res, "Cray", cray))
        {
            return;
        }
        if (cray)
        {
            patchOemCray(asyncResp, *cray);
        }
    }

    void setPowerCapOverride(
        const std::shared_ptr<SensorsAsyncResp>& asyncResp,
        std::vector<nlohmann::json>& powerControlCollections)
    {
        auto getChassisPath =
            [asyncResp, powerControlCollections](
                const std::optional<std::string>& chassisPath) mutable {
                if (!chassisPath)
                {
                    BMCWEB_LOG_ERROR << "Don't find valid chassis path ";
                    messages::resourceNotFound(asyncResp->res, "Chassis",
                                               asyncResp->chassisId);
                    return;
                }

                if (powerControlCollections.size() != 1)
                {
                    BMCWEB_LOG_ERROR
                        << "Don't support multiple hosts at present ";
                    messages::resourceNotFound(asyncResp->res, "Power",
                                               "PowerControl");
                    return;
                }

                auto& item = powerControlCollections[0];

                std::optional<nlohmann::json> powerLimit;
                if (!json_util::readJson(item, asyncResp->res, "PowerLimit",
                                         powerLimit))
                {
                    return;
                }
                if (!powerLimit)
                {
                    return;
                }
                std::optional<nlohmann::json> oem;
                if (!json_util::readJson(*powerLimit, asyncResp->res, "Oem",
                                         oem))
                {
                    return;
                }
                if (oem)
                {
                    patchOem(asyncResp, *oem);
                }
            };
        getValidChassisPath(asyncResp, std::move(getChassisPath));
    }

    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            res.result(boost::beast::http::status::internal_server_error);
            res.end();
            return;
        }
        const std::string& chassisName = params[0];

        res.jsonValue["PowerControl"] = nlohmann::json::array();

        auto sensorAsyncResp = std::make_shared<SensorsAsyncResp>(
            res, chassisName, sensors::dbus::types.at(sensors::node::power),
            sensors::node::power);

        getChassisData(sensorAsyncResp);

        // This callback verifies that the power limit is only provided for
        // the chassis that implements the Chassis inventory item. This
        // prevents things like power supplies providing the chassis power
        // limit
        auto chassisHandler = [sensorAsyncResp](
                                  const boost::system::error_code e,
                                  const std::vector<std::string>&
                                      chassisPaths) {
            if (e)
            {
                BMCWEB_LOG_ERROR
                    << "Power Limit GetSubTreePaths handler Dbus error " << e;
                return;
            }

            bool found = false;
            for (const std::string& chassis : chassisPaths)
            {
                size_t len = std::string::npos;
                size_t lastPos = chassis.rfind('/');
                if (lastPos == std::string::npos)
                {
                    continue;
                }

                if (lastPos == chassis.size() - 1)
                {
                    size_t end = lastPos;
                    lastPos = chassis.rfind('/', lastPos - 1);
                    if (lastPos == std::string::npos)
                    {
                        continue;
                    }

                    len = end - (lastPos + 1);
                }

                std::string interfaceChassisName =
                    chassis.substr(lastPos + 1, len);
                if (!interfaceChassisName.compare(sensorAsyncResp->chassisId))
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                BMCWEB_LOG_DEBUG << "Power Limit not present for "
                                 << sensorAsyncResp->chassisId;
                return;
            }

            nlohmann::json& powerControl =
                sensorAsyncResp->res.jsonValue["PowerControl"];

            if (powerControl.empty())
            {
                powerControl.push_back(
                    {{"@odata.type", "#Power.v1_0_0.PowerControl"},
                     {"@odata.id", "/redfish/v1/Chassis/" +
                                       sensorAsyncResp->chassisId +
                                       "/Power#/PowerControl/0"},
                     {"Name", "Chassis Power Control"},
                     {"MemberId", "0"}});
            }

            auto errorProc = [sensorAsyncResp](boost::system::error_code ec) {
                messages::internalError(sensorAsyncResp->res);
                BMCWEB_LOG_ERROR << "Power Limit GetAll handler: Dbus error "
                                 << ec;
            };

            auto setPowerAllocatedWatts = [sensorAsyncResp](const auto& value) {
                nlohmann::json& json = sensorAsyncResp->res.jsonValue;
                nlohmann::json& powerControl = json["PowerControl"][0];

                powerControl["PowerAllocatedWatts"] = value;
                powerControl["Oem"]["Cray"]["PowerAllocatedWatts"] = value;
            };

            sdbusplus::asio::getAllProperties(
                *crow::connections::systemBus,
                "xyz.openbmc_project.NodeManager",
                "/xyz/openbmc_project/NodeManager/Domain/5/Policy/"
                "254",
                "xyz.openbmc_project.NodeManager.PolicyAttributes",
                [setPowerAllocatedWatts](boost::system::error_code) {
                    setPowerAllocatedWatts(nlohmann::json());
                },
                [setPowerAllocatedWatts](
                    std::vector<std::pair<
                        std::string, std::variant<std::monostate, uint16_t>>>&
                        properties) {
                    try
                    {
                        uint16_t limit = 0u;
                        sdbusplus::unpackProperties(properties, "Limit", limit);

                        setPowerAllocatedWatts(limit);
                    }
                    catch (const sdbusplus::exception::UnpackPropertyError&)
                    {}
                });
        };

        crow::connections::systemBus->async_method_call(
            std::move(chassisHandler), "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
            "/xyz/openbmc_project/inventory", 0,
            std::array<const char*, 2>{
                "xyz.openbmc_project.Inventory.Item.Board",
                "xyz.openbmc_project.Inventory.Item.Chassis"});
    }
    void doPatch(crow::Response& res, const crow::Request& req,
                 const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            messages::internalError(res);
            res.end();
            return;
        }

        const std::string& chassisName = params[0];
        auto asyncResp = std::make_shared<SensorsAsyncResp>(
            res, chassisName, sensors::dbus::types.at(sensors::node::power),
            sensors::node::power);

        std::optional<std::vector<nlohmann::json>> voltageCollections;
        std::optional<std::vector<nlohmann::json>> powerCtlCollections;

        if (!json_util::readJson(req, asyncResp->res, "PowerControl",
                                 powerCtlCollections, "Voltages",
                                 voltageCollections))
        {
            return;
        }

        if (powerCtlCollections)
        {
            setPowerCapOverride(asyncResp, *powerCtlCollections);
        }
        if (voltageCollections)
        {
            std::unordered_map<std::string, std::vector<nlohmann::json>>
                allCollections;
            allCollections.emplace("Voltages", *std::move(voltageCollections));
            checkAndDoSensorsOverride(asyncResp, allCollections);
        }
    }
};

} // namespace redfish
