#include "registries.hpp"
#include "registries/base_message_registry.hpp"
#include "registries/openbmc_message_registry.hpp"

#include <string.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include "gmock/gmock.h"

TEST(RedfishRegistries, fillMessageArgs)
{
    using redfish::registries::fillMessageArgs;
    std::string toFill("%1");
    fillMessageArgs({{"foo"}}, toFill);
    EXPECT_EQ(toFill, "foo");

    toFill = "";
    fillMessageArgs({}, toFill);
    EXPECT_EQ(toFill, "");

    toFill = "%1, %2";
    fillMessageArgs({{"foo", "bar"}}, toFill);
    EXPECT_EQ(toFill, "foo, bar");
}

namespace
{

// Taken from log_services.hpp
const redfish::registries::Message*
    getMessage(const std::string_view& messageID)
{
    // Redfish MessageIds are in the form
    // RegistryName.MajorVersion.MinorVersion.MessageKey, so parse it to find
    // the right Message
    std::vector<std::string> fields;
    fields.reserve(4);
    boost::split(fields, messageID, boost::is_any_of("."));
    std::string& registryName = fields[0];
    std::string& messageKey = fields[3];

    // Find the right registry and check it for the MessageKey
    if (std::string(redfish::registries::base::header.registryPrefix) ==
        registryName)
    {
        return getMessageFromRegistry(
            messageKey, std::span<const redfish::registries::MessageEntry>(
                            redfish::registries::base::registry));
    }
    if (std::string(redfish::registries::openbmc::header.registryPrefix) ==
        registryName)
    {
        return getMessageFromRegistry(
            messageKey, std::span<const redfish::registries::MessageEntry>(
                            redfish::registries::openbmc::registry));
    }
    return nullptr;
}

} // namespace

// Change 52351; the message shall pass the Validator
TEST(RedfishRegistries, serviceStartedMessage)
{
    ASSERT_EQ(redfish::registries::openbmc::registry.size(), 189);

    // Non-existent
    const redfish::registries::Message* msg = getMessageFromRegistry(
        "Non-Existent", redfish::registries::openbmc::registry);
    ASSERT_EQ(msg, nullptr);

    const redfish::registries::Message* msg1 = getMessageFromRegistry(
        "ServiceStarted", redfish::registries::openbmc::registry);
    ASSERT_EQ(msg1, nullptr);

    const redfish::registries::Message* msg2 =
        getMessage("OpenBMC.1.0.ServiceStarted");
    ASSERT_EQ(msg2, nullptr);
}