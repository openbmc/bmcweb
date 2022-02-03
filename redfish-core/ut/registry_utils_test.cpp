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

/*!
 * @file    registry_utils_test.cpp
 * @brief   Source code for registry utility testing.
 */

/* -------------------------------- Includes -------------------------------- */
#include <utils/registry_utils.hpp>
#include <gmock/gmock.h>

namespace redfish
{
namespace message_registries
{

bool cmpMsg(const Message *msg1, const Message *msg2)
{
    if (strcmp(msg1->description, msg2->description) != 0)
    {
        return false;
    }
    if (strcmp(msg1->message, msg2->message) != 0)
    {
        return false;
    }
    if (strcmp(msg1->severity, msg2->severity) != 0)
    {
        return false;
    }
    if (strcmp(msg1->messageSeverity, msg2->messageSeverity) != 0)
    {
        return false;
    }
    if (msg1->numberOfArgs != msg2->numberOfArgs)
    {
        return false;
    }
    for (size_t i=0; i<msg1->numberOfArgs; i++)
    {
        if (strcmp(msg1->paramTypes[i], msg2->paramTypes[i]) != 0)
        {
            return false;
        }
    }
    if (strcmp(msg1->resolution, msg2->resolution) != 0)
    {
        return false;
    }
    return true;
}

bool cmpRegistry(const boost::beast::span<const MessageEntry>& registry1,
    const boost::beast::span<const MessageEntry>& registry2)
{
    if (registry1.size() != registry2.size())
    {
        return false;
    }
    for (size_t i=0; i<registry1.size(); i++)
    {
        const MessageEntry *pRegistry1 = registry1.data();
        const MessageEntry *pRegistry2 = registry2.data();
        if (strcmp(pRegistry1->first, pRegistry1->first) != 0)
        {
            return false;
        }
        if (!cmpMsg(&pRegistry1->second, &pRegistry2->second))
        {
            return false;
        }
    }
    return true;
}

TEST(RegistryUtilsTest, GetRegistryFromPrefix)
{
    EXPECT_TRUE(cmpRegistry(
        getRegistryFromPrefix(task_event::header.registryPrefix),
        boost::beast::span<const MessageEntry>(task_event::registry)));
    EXPECT_TRUE(cmpRegistry(
        getRegistryFromPrefix("abc"),
        boost::beast::span<const MessageEntry>()));
}

TEST(RegistryUtilsTest, GetMessageFromRegistry)
{
    const boost::beast::span<const MessageEntry> resourceRegistry = 
        getRegistryFromPrefix("ResourceEvent");

    EXPECT_TRUE(
        cmpMsg(getMessageFromRegistry("LicenseAdded", resourceRegistry),
        &resource_event::registry[0].second));
    EXPECT_EQ(getMessageFromRegistry("abc", resourceRegistry), 
        nullptr);
}

TEST(RegistryUtilsTest, GetPrefix)
{
    EXPECT_STREQ(getPrefix("ResourceEvent.1.0.ResourceCreated").c_str(),
        "ResourceEvent");
}

TEST(RegistryUtilsTest, GetMessage)
{
    // correct MessageId
    EXPECT_TRUE(cmpMsg(getMessage("ResourceEvent.1.0.LicenseAdded"),
        &resource_event::registry[0].second));

    // invaild MessageId
    EXPECT_EQ(getMessage("ResourceEvent.0.LicenseAdded"),
        nullptr);
    EXPECT_EQ(getMessage("a.b.c.d"),
        nullptr);
}

TEST(RegistryUtilsTest, IsMessageIdValid)
{
    // correct MessageId
    EXPECT_TRUE(isMessageIdValid("ResourceEvent.1.0.LicenseAdded"));
    
    // invaild MessageId
    EXPECT_FALSE(isMessageIdValid("ResourceEvent.0.LicenseAdded"));
    EXPECT_FALSE(isMessageIdValid("a.b.c.d"));
}

} // namespace message_registries
} // namespace redfish