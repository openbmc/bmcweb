#include "utils/service_utils.hpp"

#include <gmock/gmock.h>

namespace redfish
{
namespace service_util
{

TEST(ServiceUtilsTest, MatchServiceGood)
{
    using details::matchService;
    sdbusplus::message::object_path basePath("/service/base/path");

    EXPECT_TRUE(matchService(basePath / "serviceUnit", "serviceUnit"));
    EXPECT_TRUE(matchService(basePath / "service-unit", "service-unit"));
    EXPECT_TRUE(matchService(basePath / "serviceUnit@test", "serviceUnit"));
    EXPECT_TRUE(matchService(basePath / "service-unit@test", "service-unit"));
}

TEST(ServiceUtilsTest, MatchServiceBad)
{
    using details::matchService;
    sdbusplus::message::object_path basePath("/service/base/path");

    EXPECT_FALSE(matchService(basePath / "serviceUnit", "service-unit"));
    EXPECT_FALSE(matchService(basePath / "serviceUnitTest", "serviceUnit"));
    EXPECT_FALSE(matchService(basePath / "serviceUnit@test", "service-unit"));
    EXPECT_FALSE(matchService(basePath / "service-unit-test", "service-unit"));
}

} // namespace service_util
} // namespace redfish
