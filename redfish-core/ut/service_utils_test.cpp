#include "utils/service_utils.hpp"

#include <gmock/gmock.h>

namespace redfish
{
namespace service_util
{

TEST(ServiceUtilsTest, MatchService)
{
    sdbusplus::message::object_path basePath("/service/base/path");

    EXPECT_TRUE(matchService(basePath / "serviceUnit", "serviceUnit"));
    EXPECT_TRUE(matchService(basePath / "service-unit", "service-unit"));
    EXPECT_FALSE(matchService(basePath / "serviceUnit", "service-unit"));
    EXPECT_TRUE(matchService(basePath / "serviceUnit@aaa", "serviceUnit"));
    EXPECT_TRUE(matchService(basePath / "service-unit@aaa", "service-unit"));
    EXPECT_FALSE(matchService(basePath / "serviceUnitBbb", "serviceUnit"));
    EXPECT_FALSE(matchService(basePath / "service-unit-bbb", "service-unit"));
}

} // namespace service_util
} // namespace redfish
