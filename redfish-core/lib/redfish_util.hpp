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
void getPortStatusAndPath(const std::string& serviceName,
                          CallbackFunc&& callback)
{
    crow::connections::systemBus->async_method_call(
        [serviceName,
         callback{std::move(callback)}](const boost::system::error_code ec,
                                        const std::vector<UnitStruct>& r) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << ec;
                // return error code
                callback(ec, "", false);
                return;
            }

            for (const UnitStruct& unit : r)
            {
                // Only traverse through <xyz>.socket units
                const std::string& unitName =
                    std::get<NET_PROTO_UNIT_NAME>(unit);

                // find "." into unitsName
                size_t lastCharPos = unitName.rfind('.');
                if (lastCharPos == std::string::npos)
                {
                    continue;
                }

                // is unitsName end with ".socket"
                std::string unitNameEnd = unitName.substr(lastCharPos);
                if (unitNameEnd.compare(".socket") != 0)
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

                // We are interested in services, which starts with
                // mapped service name
                if (unitNameStr != serviceName)
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
                callback(ec, socketPath, isProtocolEnabled);
                return;
            }

            //  no service foudn, throw error
            boost::system::error_code ec1 =
                boost::system::errc::make_error_code(
                    boost::system::errc::no_such_process);
            // return error code
            callback(ec1, "", false);
            BMCWEB_LOG_ERROR << ec1;
        },
        "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
        "org.freedesktop.systemd1.Manager", "ListUnits");
}

template <typename CallbackFunc>
void getPortNumber(const std::string& socketPath, CallbackFunc&& callback)
{
    crow::connections::systemBus->async_method_call(
        [callback{std::move(callback)}](
            const boost::system::error_code ec,
            const std::variant<
                std::vector<std::tuple<std::string, std::string>>>& resp) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << ec;
                callback(ec, 0);
                return;
            }
            const std::vector<
                std::tuple<std::string, std::string>>* responsePtr =
                std::get_if<std::vector<std::tuple<std::string, std::string>>>(
                    &resp);
            if (responsePtr == nullptr || responsePtr->size() < 1)
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
                std::get<NET_PROTO_LISTEN_STREAM>((*responsePtr)[0]);
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
        },
        "org.freedesktop.systemd1", socketPath,
        "org.freedesktop.DBus.Properties", "Get",
        "org.freedesktop.systemd1.Socket", "Listen");
}

#endif

/**
 * @brief Check if the child path is a subpath of the parent path
 *
 * @param[in]  child   D-bus path that is expected to be a subpath of parent
 * @param[in]  parent  D-bus path that is expected to be a superpath of child
 *
 * @return true if child is a subpath of parent, otherwise, return false.
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
void checkValidChassisId(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& chassisId,
                         const std::function<void(bool)>& callback)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, chassisId,
         callback](const boost::system::error_code ec,
                   const std::vector<std::string>& objects) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(asyncResp->res);
                return;
            }

            for (const auto& chassis : objects)
            {
                sdbusplus::message::object_path path(chassis);
                if (chassisId != path.filename())
                {
                    continue;
                }

                callback(true);
                return;
            }

            callback(false);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/inventory", 0,
        std::array<const char*, 2>{
            "xyz.openbmc_project.Inventory.Item.Board",
            "xyz.openbmc_project.Inventory.Item.Chassis"});
}

} // namespace redfish
