#include "bmcweb_config.h"

#include "utils/log_services_utils.hpp"

#include <optional>

#include "gtest/gtest.h"

namespace redfish::location_util
{
namespace
{

using namespace log_services_utils;

TEST(LogServicesUtils, LogServiceParentCollectionToString)
{
    EXPECT_EQ(logServiceParentCollectionToString(
                  LogServiceParentCollection::Managers),
              "Managers");

    EXPECT_EQ(
        logServiceParentCollectionToString(LogServiceParentCollection::Systems),
        "Systems");

    EXPECT_EQ(logServiceParentCollectionToString(
                  static_cast<LogServiceParentCollection>(99)),
              std::nullopt);
}

TEST(LogServicesUtils, GetMemberIdFromParentCollection)
{
    EXPECT_EQ(
        getMemberIdFromParentCollection(LogServiceParentCollection::Managers),
        BMCWEB_REDFISH_MANAGER_URI_NAME);

    EXPECT_EQ(
        getMemberIdFromParentCollection(LogServiceParentCollection::Systems),
        BMCWEB_REDFISH_SYSTEM_URI_NAME);

    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    EXPECT_EQ(getMemberIdFromParentCollection(
                  static_cast<LogServiceParentCollection>(99)),
              std::nullopt);
}

TEST(LogServicesUtils, GetLogEntryDescriptorFromParentCollection)
{
    EXPECT_EQ(getLogEntryDescriptorFromParentCollection(
                  LogServiceParentCollection::Managers),
              "Manager");

    EXPECT_EQ(getLogEntryDescriptorFromParentCollection(
                  LogServiceParentCollection::Systems),
              "System");

    EXPECT_EQ(getLogEntryDescriptorFromParentCollection(
                  static_cast<LogServiceParentCollection>(99)),
              std::nullopt);
}
} // namespace
} // namespace redfish::location_util
