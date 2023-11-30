#include "app.hpp"
#include "async_resp.hpp"
#include "log_services.hpp"

#include <systemd/sd-id128.h>

#include <nlohmann/json.hpp>

#include <format>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(LogServicesBMCJouralTest, LogServicesBMCJouralGetReturnsError)
{
    auto shareAsyncResp = std::make_shared<bmcweb::AsyncResp>();
    sd_id128_t bootIDOut{};
    uint64_t timestampOut = 0;
    uint64_t indexOut = 0;
    uint64_t timestampIn = 1740970301UL;
    std::string badBootIDStr = "78770392794344a29f81507f3ce5e";
    std::string goodBootIDStr = "78770392794344a29f81507f3ce5e78c";
    sd_id128_t goodBootID{};

    // invalid test cases
    EXPECT_FALSE(getTimestampFromID(shareAsyncResp, "", bootIDOut, timestampOut,
                                    indexOut));
    EXPECT_FALSE(getTimestampFromID(shareAsyncResp, badBootIDStr, bootIDOut,
                                    timestampOut, indexOut));
    EXPECT_FALSE(getTimestampFromID(
        shareAsyncResp, std::format("{}_{}", badBootIDStr, timestampIn),
        bootIDOut, timestampOut, indexOut));
    EXPECT_FALSE(getTimestampFromID(
        shareAsyncResp, std::format("{}_{}", badBootIDStr, timestampIn),
        bootIDOut, timestampOut, indexOut));

    // obtain a goodBootID
    EXPECT_GE(sd_id128_from_string(goodBootIDStr.c_str(), &goodBootID), 0);

    EXPECT_FALSE(getTimestampFromID(
        shareAsyncResp, std::format("{}_{}", goodBootIDStr, "InvalidNum"),
        bootIDOut, timestampOut, indexOut));

    // Success cases
    EXPECT_TRUE(getTimestampFromID(
        shareAsyncResp, std::format("{}_{}", goodBootIDStr, timestampIn),
        bootIDOut, timestampOut, indexOut));
    EXPECT_NE(sd_id128_equal(goodBootID, bootIDOut), 0);
    EXPECT_EQ(timestampIn, timestampOut);
    EXPECT_EQ(indexOut, 0);

    uint64_t indexIn = 1;
    EXPECT_TRUE(getTimestampFromID(
        shareAsyncResp,
        std::format("{}_{}_{}", goodBootIDStr, timestampIn, indexIn), bootIDOut,
        timestampOut, indexOut));
    EXPECT_NE(sd_id128_equal(goodBootID, bootIDOut), 0);
    EXPECT_EQ(timestampIn, timestampOut);
    EXPECT_EQ(indexOut, indexIn);
}

} // namespace
} // namespace redfish
