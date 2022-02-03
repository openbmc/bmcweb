/*
// Copyright (c) 2021 NVIDIA Corporation
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

#include <cstddef>
#include <string>
#include <cstring>
#include <array>
#include <utility>

#include <registries.hpp>
#include <registries/base_message_registry.hpp>
#include <registries/openbmc_message_registry.hpp>
#include <registries/task_event_message_registry.hpp>
#include <registries/resource_event_message_registry.hpp>

#include <boost/beast/core/span.hpp>
#include <boost/algorithm/string.hpp>

namespace redfish
{
    namespace message_registries
    {
        inline boost::beast::span<const MessageEntry>
            getRegistryFromPrefix(const std::string& registryName)
        {
            if (task_event::header.registryPrefix == registryName)
            {
                return boost::beast::span<const MessageEntry>(
                    task_event::registry);
            }
            if (openbmc::header.registryPrefix == registryName)
            {
                return boost::beast::span<const MessageEntry>(
                    openbmc::registry);
            }
            if (base::header.registryPrefix == registryName)
            {
                return boost::beast::span<const MessageEntry>(base::registry);
            }
            if (resource_event::header.registryPrefix == registryName)
            {
                return boost::beast::span<const MessageEntry>(
                    resource_event::registry);
            }
            return {};
        }

        inline const Message* getMessageFromRegistry
        (
            const std::string& messageKey,
            const boost::beast::span<const MessageEntry> registry
        )
        {
            boost::beast::span<const MessageEntry>::const_iterator messageIt =
                std::find_if(registry.cbegin(), registry.cend(),
                            [&messageKey](const MessageEntry& messageEntry) {
                                return !std::strcmp(messageEntry.first,
                                                    messageKey.c_str());
                            });
            if (messageIt != registry.cend())
            {
                return &messageIt->second;
            }

            return nullptr;
        }

        inline std::string getPrefix(const std::string& messageID)
        {
            size_t pos = messageID.find('.');
            return messageID.substr(0, pos);
        }

        inline const Message* getMessage(const std::string_view& messageID)
        {
            // Redfish MessageIds are in the form
            // RegistryName.MajorVersion.MinorVersion.MessageKey, so parse it to
            // find the right Message
            std::vector<std::string> fields;
            fields.reserve(4);
            boost::split(fields, messageID, boost::is_any_of("."));

            if (fields.size() != 4)
            {
                return nullptr;
            }
            std::string& registryName = fields[0];
            std::string& messageKey = fields[3];

            return getMessageFromRegistry(messageKey,
                getRegistryFromPrefix(registryName));
        }

        inline bool isMessageIdValid(const std::string &messageId)
        {
            const Message* msg = getMessage(messageId);
            (void)msg;
            return msg != nullptr;
        }
    }
}