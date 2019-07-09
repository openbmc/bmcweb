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

#include <regex>
#include <sdbusplus/message.hpp>

namespace dbus
{

namespace utility
{

using DbusVariantType =
    std::variant<std::vector<std::tuple<std::string, std::string, std::string>>,
                 std::vector<std::string>, std::vector<double>, std::string,
                 int64_t, uint64_t, double, int32_t, uint32_t, int16_t,
                 uint16_t, uint8_t, bool>;

using ManagedObjectType = std::vector<
    std::pair<sdbusplus::message::object_path,
              boost::container::flat_map<
                  std::string,
                  boost::container::flat_map<std::string, DbusVariantType>>>>;

using ManagedItem = std::pair<
    sdbusplus::message::object_path,
    boost::container::flat_map<
        std::string, boost::container::flat_map<std::string, DbusVariantType>>>;

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
    int count = 0;
    std::string::const_iterator first = path.begin();
    std::string::const_iterator last = path.end();
    for (std::string::const_iterator it = path.begin(); it < path.end(); it++)
    {
        // skip first character as it's either a leading slash or the first
        // character in the word
        if (it == path.begin())
        {
            continue;
        }
        if (*it == '/')
        {
            count++;
            if (count == index)
            {
                first = it;
            }
            if (count == index + 1)
            {
                last = it;
                break;
            }
        }
    }
    if (count < index)
    {
        return false;
    }
    if (first != path.begin())
    {
        first++;
    }
    result = path.substr(static_cast<size_t>(first - path.begin()),
                         static_cast<size_t>(last - first));
    return true;
}

template <typename Callback>
inline void checkDbusPathExists(const std::string& path, Callback&& callback)
{
    using GetObjectType =
        std::vector<std::pair<std::string, std::vector<std::string>>>;

    crow::connections::systemBus->async_method_call(
        [callback{std::move(callback)}](const boost::system::error_code ec,
                                        const GetObjectType& object_names) {
            callback(!ec && object_names.size() != 0);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", path,
        std::array<std::string, 0>());
}

} // namespace utility
} // namespace dbus
