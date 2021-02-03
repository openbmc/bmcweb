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

namespace dbus
{

namespace utility
{

using DbusVariantType =
    std::variant<std::vector<std::tuple<std::string, std::string, std::string>>,
                 std::vector<std::string>, std::vector<double>, std::string,
                 int64_t, uint64_t, double, int32_t, uint32_t, int16_t,
                 uint16_t, uint8_t, bool>;

using DBusPropertiesMap =
    boost::container::flat_map<std::string, DbusVariantType>;
using DBusInteracesMap =
    boost::container::flat_map<std::string, DBusPropertiesMap>;
using ManagedObjectType =
    std::vector<std::pair<sdbusplus::message::object_path, DBusInteracesMap>>;

using ManagedItem = std::pair<
    sdbusplus::message::object_path,
    boost::container::flat_map<
        std::string, boost::container::flat_map<std::string, DbusVariantType>>>;

inline void escapePathForDbus(std::string& path)
{
    for (char& c : path)
    {
        if (!isalnum(c) && c != '_' && c != '/')
        {
            c = '_';
        }
    }
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
    if (count < index)
    {
        return false;
    }

    return true;
}

template <typename Callback>
inline void checkDbusPathExists(const std::string& path, Callback&& callback)
{
    using GetObjectType =
        std::vector<std::pair<std::string, std::vector<std::string>>>;

    crow::connections::systemBus->async_method_call(
        [callback{std::move(callback)}](const boost::system::error_code ec,
                                        const GetObjectType& objectNames) {
            callback(!ec && objectNames.size() != 0);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", path,
        std::array<std::string, 0>());
}

inline bool isHostnameValid(const std::string& hostName)
{
    if (hostName.empty() || hostName.length() > 64)
    {
        return false;
    }

    std::string::const_iterator c = hostName.begin();
    if (!isalnum(*c))
    {
        return false;
    }
    for (; c != hostName.end(); c++)
    {
        if (!isalnum(*c) && *c != '-')
        {
            return false;
        }
        c++;
    }
    // Can't start or end with a dash.
    if (*c == '-')
    {
        return false;
    }

    return true;
}

} // namespace utility
} // namespace dbus
