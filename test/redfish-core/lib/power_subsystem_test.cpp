#include "async_resp.hpp"
#include "power_subsystem.hpp"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

constexpr std::string_view chassisId = "ChassisId";
constexpr std::string_view validChassisPath = "ChassisPath";

void assertPowerSubsystemCollectionGet(crow::Response& res)
{
    nlohmann::json& json = res.jsonValue;
    EXPECT_EQ(json["@odata.type"], "#PowerSubsystem.v1_1_0.PowerSubsystem");
    EXPECT_EQ(json["Name"], "Power Subsystem");
    EXPECT_EQ(json["Id"], "PowerSubsystem");
    EXPECT_EQ(json["@odata.id"],
              "/redfish/v1/Chassis/ChassisId/PowerSubsystem");
    EXPECT_EQ(json["Status"]["State"], "Enabled");
    EXPECT_EQ(json["Status"]["Health"], "OK");
}

TEST(PowerSubsystemCollectionTest,
     PowerSubsystemCollectionStaticAttributesAreExpected)
{
    auto shareAsyncResp = std::make_shared<bmcweb::AsyncResp>();
    shareAsyncResp->res.setCompleteRequestHandler(
        assertPowerSubsystemCollectionGet);
    doPowerSubsystemCollection(
        shareAsyncResp, chassisId,
        std::make_optional<std::string>(validChassisPath));
}

} // namespace
} // namespace redfish
