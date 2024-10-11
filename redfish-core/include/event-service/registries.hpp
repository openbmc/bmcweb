/*
Copyright (c) 2020 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#pragma once

#include "registries_selector.hpp"
#include "str_utility.hpp"

#include <sys/inotify.h>

#include <boost/asio/io_context.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url_view_base.hpp>
#include <sdbusplus/bus/match.hpp>

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <span>
#include <string>

namespace redfish
{

namespace registries
{

static const Message*
    getMsgFromRegistry(const std::string& messageKey,
                       const std::span<const MessageEntry>& registry)
{
    std::span<const MessageEntry>::iterator messageIt = std::ranges::find_if(
        registry, [&messageKey](const MessageEntry& messageEntry) {
            return messageKey == messageEntry.first;
        });
    if (messageIt != registry.end())
    {
        return &messageIt->second;
    }

    return nullptr;
}
static const Message* formatMessage(std::string_view messageID)
{
    // Redfish MessageIds are in the form
    // RegistryName.MajorVersion.MinorVersion.MessageKey, so parse it to find
    // the right Message
    std::vector<std::string> fields;
    fields.reserve(4);

    bmcweb::split(fields, messageID, '.');
    if (fields.size() != 4)
    {
        return nullptr;
    }
    const std::string& registryName = fields[0];
    const std::string& messageKey = fields[3];

    // Find the right registry and check it for the MessageKey
    return getMsgFromRegistry(messageKey, getRegistryFromPrefix(registryName));
}
} // namespace registries

} // namespace redfish
