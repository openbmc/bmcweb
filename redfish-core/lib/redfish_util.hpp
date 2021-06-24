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
#ifndef BMCWEB_ENABLE_REDFISH_ONE_CHASSIS

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"

#include <boost/system/error_code.hpp>
#include <sdbusplus/asio/property.hpp>

#include <array>
#include <charconv>
#include <string_view>

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
    constexpr std::array<std::string_view, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Board",
        "xyz.openbmc_project.Inventory.Item.Chassis"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [callback,
         asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << ec;
            return;
        }
        if (subtree.empty())
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
        });
}

template <typename CallbackFunc>
void getPortStatusAndPath(
    std::span<const std::pair<std::string_view, std::string_view>>
        protocolToDBus,
    CallbackFunc&& callback)
{
    crow::connections::systemBus->async_method_call(
        [protocolToDBus, callback{std::forward<CallbackFunc>(callback)}](
            const boost::system::error_code& ec,
            const std::vector<UnitStruct>& r) {
        std::vector<std::tuple<std::string, std::string, bool>> socketData;
        if (ec)
        {
            BMCWEB_LOG_ERROR << ec;
            // return error code
            callback(ec, socketData);
            return;
        }

        // save all service output into vector
        for (const UnitStruct& unit : r)
        {
            // Only traverse through <xyz>.socket units
            const std::string& unitName = std::get<NET_PROTO_UNIT_NAME>(unit);

            // find "." into unitsName
            size_t lastCharPos = unitName.rfind('.');
            if (lastCharPos == std::string::npos)
            {
                continue;
            }

            // is unitsName end with ".socket"
            std::string unitNameEnd = unitName.substr(lastCharPos);
            if (unitNameEnd != ".socket")
            {
                continue;
            }

            // find "@" into unitsName
            if (size_t atCharPos = unitName.rfind('@');
                atCharPos != std::string::npos)
            {
                lastCharPos = atCharPos;
            }

            // unitsName without "@eth(x).socket", only <xyz>
            // unitsName without ".socket", only <xyz>
            std::string unitNameStr = unitName.substr(0, lastCharPos);

            for (const auto& kv : protocolToDBus)
            {
                // We are interested in services, which starts with
                // mapped service name
                if (unitNameStr != kv.second)
                {
                    continue;
                }

                const std::string& socketPath =
                    std::get<NET_PROTO_UNIT_OBJ_PATH>(unit);
                const std::string& unitState =
                    std::get<NET_PROTO_UNIT_SUB_STATE>(unit);

                bool isProtocolEnabled =
                    ((unitState == "running") || (unitState == "listening"));

                socketData.push_back(
                    {socketPath, std::string(kv.first), isProtocolEnabled});
                // We found service, return from inner loop.
                break;
            }
        }

        callback(ec, socketData);
        },
        "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
        "org.freedesktop.systemd1.Manager", "ListUnits");
}

template <typename CallbackFunc>
void getPortNumber(const std::string& socketPath, CallbackFunc&& callback)
{
    sdbusplus::asio::getProperty<
        std::vector<std::tuple<std::string, std::string>>>(
        *crow::connections::systemBus, "org.freedesktop.systemd1", socketPath,
        "org.freedesktop.systemd1.Socket", "Listen",
        [callback{std::forward<CallbackFunc>(callback)}](
            const boost::system::error_code& ec,
            const std::vector<std::tuple<std::string, std::string>>& resp) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << ec;
            callback(ec, 0);
            return;
        }
        if (resp.empty())
        {
            // Network Protocol Listen Response Elements is empty
            boost::system::error_code ec1 =
                boost::system::errc::make_error_code(
                    boost::system::errc::bad_message);
            // return error code
            callback(ec1, 0);
            BMCWEB_LOG_ERROR << ec1;
            return;
        }
        const std::string& listenStream =
            std::get<NET_PROTO_LISTEN_STREAM>(resp[0]);
        const char* pa = &listenStream[listenStream.rfind(':') + 1];
        int port{0};
        if (auto [p, ec2] = std::from_chars(pa, nullptr, port);
            ec2 != std::errc())
        {
            // there is only two possibility invalid_argument and
            // result_out_of_range
            boost::system::error_code ec3 =
                boost::system::errc::make_error_code(
                    boost::system::errc::invalid_argument);
            if (ec2 == std::errc::result_out_of_range)
            {
                ec3 = boost::system::errc::make_error_code(
                    boost::system::errc::result_out_of_range);
            }
            // return error code
            callback(ec3, 0);
            BMCWEB_LOG_ERROR << ec3;
        }
        callback(ec, port);
        });
}

} // namespace redfish
#endif
