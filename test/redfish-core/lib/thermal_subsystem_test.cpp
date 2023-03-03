#include "async_resp.hpp"
#include "thermal_subsystem.hpp"

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

void assertThermalCollectionGet(crow::Response& res)
{
    nlohmann::json& json = res.jsonValue;
    EXPECT_EQ(json["@odata.type"], "#ThermalSubsystem.v1_0_0.ThermalSubsystem");
    EXPECT_EQ(json["Name"], "Thermal Subsystem");
    EXPECT_EQ(json["Id"], "ThermalSubsystem");
    EXPECT_EQ(json["@odata.id"],
              "/redfish/v1/Chassis/ChassisId/ThermalSubsystem");
    EXPECT_EQ(json["Status"]["State"], "Enabled");
    EXPECT_EQ(json["Status"]["Health"], "OK");
}

TEST(ThermalSubsystemCollectionTest,
     ThermalSubsystemCollectionStaticAttributesAreExpected)
{
    auto shareAsyncResp = std::make_shared<bmcweb::AsyncResp>();
    shareAsyncResp->res.setCompleteRequestHandler(assertThermalCollectionGet);
    doThermalSubsystemCollection(
        shareAsyncResp, chassisId,
        std::make_optional<std::string>(validChassisPath));
}

} // namespace
} // namespace redfish
