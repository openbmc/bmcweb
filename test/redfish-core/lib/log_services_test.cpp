#include "async_resp.hpp"
#include "log_services.hpp"
#include "manager_logservices_journal.hpp"

#include <systemd/sd-id128.h>

#include <cstdint>
#include <format>
#include <memory>
#include <string>

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

TEST(LogServicesPostCodeParse, PostCodeParse)
{
    uint64_t currentValue = 0;
    uint16_t index = 0;
    EXPECT_TRUE(parsePostCode("B1-2", currentValue, index));
    EXPECT_EQ(currentValue, 2);
    EXPECT_EQ(index, 1);
    EXPECT_TRUE(parsePostCode("B200-300", currentValue, index));
    EXPECT_EQ(currentValue, 300);
    EXPECT_EQ(index, 200);

    EXPECT_FALSE(parsePostCode("", currentValue, index));
    EXPECT_FALSE(parsePostCode("B", currentValue, index));
    EXPECT_FALSE(parsePostCode("B1", currentValue, index));
    EXPECT_FALSE(parsePostCode("B1-", currentValue, index));
    EXPECT_FALSE(parsePostCode("B1A-2", currentValue, index));
    EXPECT_FALSE(parsePostCode("B1A-2", currentValue, index));
    EXPECT_FALSE(parsePostCode("B1A-2z", currentValue, index));
    // Uint16_t max + 1
    EXPECT_FALSE(parsePostCode("B65536-1", currentValue, index));

    // Uint64_t max + 1
    EXPECT_FALSE(parsePostCode("B1-18446744073709551616", currentValue, index));

    // Negative numbers
    EXPECT_FALSE(parsePostCode("B-1-2", currentValue, index));
    EXPECT_FALSE(parsePostCode("B-1--2", currentValue, index));
}

} // namespace
} // namespace redfish
