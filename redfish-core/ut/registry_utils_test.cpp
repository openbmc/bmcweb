/*!
 * @file    registry_utils_test.cpp
 * @brief   Source code for registry utility testing.
 */

/* -------------------------------- Includes -------------------------------- */
#include <utils/registry_utils.hpp>

#include <gmock/gmock.h>

namespace redfish
{
namespace registries
{

bool cmpMsg(const Message* msg1, const Message* msg2)
{
    if (strcmp(msg1->description, msg2->description) != 0)
    {
        return false;
    }
    if (strcmp(msg1->message, msg2->message) != 0)
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
    for (size_t i = 0; i < msg1->numberOfArgs; i++)
    {
        if (strcmp(msg1->paramTypes[i], msg2->paramTypes[i]) != 0)
        {
            return false;
        }
    }
    return strcmp(msg1->resolution, msg2->resolution) == 0;
}

bool cmpRegistry(const std::span<const MessageEntry>& registry1,
                 const std::span<const MessageEntry>& registry2)
{
    if (registry1.size() != registry2.size())
    {
        return false;
    }
    for (size_t i = 0; i < registry1.size(); i++)
    {
        const MessageEntry* pRegistry1 = registry1.data();
        const MessageEntry* pRegistry2 = registry2.data();
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
    EXPECT_TRUE(
        cmpRegistry(getRegistryFromPrefix("TaskEvent"),
                    std::span<const MessageEntry>(task_event::registry)));
    EXPECT_TRUE(cmpRegistry(getRegistryFromPrefix("abc"),
                            std::span<const MessageEntry>()));
}

TEST(RegistryUtilsTest, GetMessageFromRegistry)
{
    const std::span<const MessageEntry> resourceRegistry =
        getRegistryFromPrefix("ResourceEvent");

    EXPECT_TRUE(cmpMsg(getMessageFromRegistry("LicenseAdded", resourceRegistry),
                       &resource_event::registry[0].second));
    EXPECT_EQ(getMessageFromRegistry("abc", resourceRegistry), nullptr);
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
    EXPECT_EQ(getMessage("ResourceEvent.0.LicenseAdded"), nullptr);
    EXPECT_EQ(getMessage("a.b.c.d"), nullptr);
}

TEST(RegistryUtilsTest, IsMessageIdValid)
{
    // correct MessageId
    EXPECT_TRUE(isMessageIdValid("ResourceEvent.1.0.LicenseAdded"));

    // invaild MessageId
    EXPECT_FALSE(isMessageIdValid("ResourceEvent.0.LicenseAdded"));
    EXPECT_FALSE(isMessageIdValid("a.b.c.d"));
}

} // namespace registries
} // namespace redfish