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

#include <sdbusplus/message.hpp>

#include <filesystem>
#include <regex>
#include <variant>

namespace dbus
{

namespace utility
{

// clang-format off
using DbusVariantType = std::variant<
    std::vector<std::tuple<std::string, std::string, std::string>>,
    std::vector<std::string>,
    std::vector<double>,
    std::string,
    int64_t,
    uint64_t,
    double,
    int32_t,
    uint32_t,
    int16_t,
    uint16_t,
    uint8_t,
    bool,
    sdbusplus::message::unix_fd,
    std::vector<uint32_t>,
    std::vector<uint16_t>,
    sdbusplus::message::object_path,
    std::tuple<uint64_t, std::vector<std::tuple<std::string, std::string, double, uint64_t>>>,
    std::vector<std::tuple<std::string, std::string>>,
    std::vector<std::tuple<uint32_t, std::vector<uint32_t>>>,
    std::vector<std::tuple<uint32_t, size_t>>,
    std::vector<std::tuple<sdbusplus::message::object_path, std::string,
                           std::string, std::string>>
 >;

// clang-format on
using DBusPropertiesMap = std::vector<std::pair<std::string, DbusVariantType>>;
using DBusInteracesMap = std::vector<std::pair<std::string, DBusPropertiesMap>>;
using ManagedObjectType =
    std::vector<std::pair<sdbusplus::message::object_path, DBusInteracesMap>>;

using ManagedItem = std::pair<
    sdbusplus::message::object_path,
    std::vector<std::pair<
        std::string, std::vector<std::pair<std::string, DbusVariantType>>>>>;

// Map of service name to list of interfaces
using MapperServiceMap =
    std::vector<std::pair<std::string, std::vector<std::string>>>;

// Map of object paths to MapperServiceMaps
using MapperGetSubTreeResponse =
    std::vector<std::pair<std::string, MapperServiceMap>>;

using MapperGetObject =
    std::vector<std::pair<std::string, std::vector<std::string>>>;

using MapperGetAncestorsResponse = std::vector<
    std::pair<std::string,
              std::vector<std::pair<std::string, std::vector<std::string>>>>>;

using MapperGetSubTreePathsResponse = std::vector<std::string>;

inline void escapePathForDbus(std::string& path)
{
    const std::regex reg("[^A-Za-z0-9_/]");
    std::regex_replace(path.begin(), path.begin(), path.end(), reg, "_");
}

// gets the string N strings deep into a path
// i.e.  /0th/1st/2nd/3rd
inline bool getNthStringFromPath(const std::string& path, int index,
                                 std::string& result)
{
    if (index < 0)
    {
        return false;
    }

    std::filesystem::path p1(path);
    int count = -1;
    for (auto const& element : p1)
    {
        if (element.has_filename())
        {
            ++count;
            if (count == index)
            {
                result = element.stem().string();
                break;
            }
        }
    }
    return count >= index;
}

template <typename Callback>
inline void checkDbusPathExists(const std::string& path, Callback&& callback)
{
    crow::connections::systemBus->async_method_call(
        [callback{std::forward<Callback>(callback)}](
            const boost::system::error_code ec,
            const dbus::utility::MapperGetObject& objectNames) {
            callback(!ec && !objectNames.empty());
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", path,
        std::array<std::string, 0>());
}

} // namespace utility
} // namespace dbus
