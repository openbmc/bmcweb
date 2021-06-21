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
#ifndef BMCWEB_ENABLE_REDFISH_ONE_CHASSIS
#pragma once

namespace redfish
{

enum NetworkProtocolUnitStructFields
{
    NET_PROTO_UNIT_NAME,
    NET_PROTO_UNIT_DESC,
    NET_PROTO_UNIT_LOAD_STATE,
    NET_PROTO_UNIT_ACTIVE_STATE,
    NET_PROTO_UNIT_SUB_STATE,
    NET_PROTO_UNIT_DEVICE,
    NET_PROTO_UNIT_OBJ_PATH,
    NET_PROTO_UNIT_ALWAYS_0,
    NET_PROTO_UNIT_ALWAYS_EMPTY,
    NET_PROTO_UNIT_ALWAYS_ROOT_PATH
};

enum NetworkProtocolListenResponseElements
{
    NET_PROTO_LISTEN_TYPE,
    NET_PROTO_LISTEN_STREAM
};

/**
 * @brief D-Bus Unit structure returned in array from ListUnits Method
 */
using UnitStruct =
    std::tuple<std::string, std::string, std::string, std::string, std::string,
               std::string, sdbusplus::message::object_path, uint32_t,
               std::string, sdbusplus::message::object_path>;

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

template <typename CallbackFunc>
void getPortStatusAndPath(const char* serviceName, CallbackFunc&& callback)
{
    if (!serviceName)
    {
        return;
    }

    crow::connections::systemBus->async_method_call(
        [serviceName{std::string(serviceName)},
         callback](const boost::system::error_code e,
                   const std::vector<UnitStruct>& r) {
            if (e)
            {
                return;
            }

            for (auto& unit : r)
            {
                // Only traverse through <xyz>.socket units
                const std::string& unitName =
                    std::get<NET_PROTO_UNIT_NAME>(unit);
                if (!boost::ends_with(unitName, ".socket"))
                {
                    continue;
                }

                size_t lastCharPos = unitName.rfind('@');
                if (lastCharPos == std::string::npos)
                {
                    lastCharPos = unitName.rfind('.');
                    if (lastCharPos == std::string::npos)
                    {
                        return;
                    }
                }
                // unitsName without "@eth(x).socket", only <xyz>
                // unitsName without ".socket", only <xyz>
                std::string unitNameStr = unitName.substr(0, lastCharPos);

                // We are interested in services, which starts with
                // mapped service name
                if (!boost::starts_with(unitName, serviceName))
                {
                    continue;
                }

                const std::string& socketPath =
                    std::get<NET_PROTO_UNIT_OBJ_PATH>(unit);
                const std::string& unitState =
                    std::get<NET_PROTO_UNIT_SUB_STATE>(unit);

                bool isProtocolEnabled =
                    ((unitState == "running") || (unitState == "listening"));
                // We found service, return from inner loop.
                callback(socketPath, isProtocolEnabled);
                return;
            }
        },
        "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
        "org.freedesktop.systemd1.Manager", "ListUnits");
}

template <typename CallbackFunc>
void getPortNumber(const std::string& socketPath, CallbackFunc&& callback)
{
    if (socketPath.empty())
    {
        return;
    }
    crow::connections::systemBus->async_method_call(
        [callback](
            const boost::system::error_code ec,
            const std::variant<
                std::vector<std::tuple<std::string, std::string>>>& resp) {
            if (ec)
            {
                return;
            }
            const std::vector<
                std::tuple<std::string, std::string>>* responsePtr =
                std::get_if<std::vector<std::tuple<std::string, std::string>>>(
                    &resp);
            if (responsePtr == nullptr || responsePtr->size() < 1)
            {
                return;
            }
            const std::string& listenStream =
                std::get<NET_PROTO_LISTEN_STREAM>((*responsePtr)[0]);
            const char* pa = &listenStream[(listenStream.rfind(':') + 1)];
            int port;
            if (auto [p, ec] = std::from_chars(pa, nullptr, port);
                ec == std::errc())
            {
                callback(port);
            }
            else
            {
                std::error_code errorCode = std::make_error_code(ec);
                BMCWEB_LOG_ERROR << errorCode.category().name();
            }
        },
        "org.freedesktop.systemd1", socketPath,
        "org.freedesktop.DBus.Properties", "Get",
        "org.freedesktop.systemd1.Socket", "Listen");
}

} // namespace redfish
#endif
