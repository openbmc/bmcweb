#include "utils/ip_utils.hpp"

#include <gtest/gtest.h> // IWYU pragma: keep

// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"

namespace redfish::ip_util
{
namespace
{

using ::boost::asio::ip::make_address;

TEST(IpToString, ReturnsCorrectIpStringForIpv4Addresses)
{
    EXPECT_EQ(toString(make_address("127.0.0.1")), "127.0.0.1");
    EXPECT_EQ(toString(make_address("192.168.1.1")), "192.168.1.1");
    EXPECT_EQ(toString(make_address("::1")), "::1");
}

TEST(IpToString, ReturnsCorrectIpStringForIpv6Addresses)
{
    EXPECT_EQ(toString(make_address("fd03:f9ab:25de:89ec::0001")),
              "fd03:f9ab:25de:89ec::1");
    EXPECT_EQ(toString(make_address("fd03:f9ab:25de:89ec::1234:abcd")),
              "fd03:f9ab:25de:89ec::1234:abcd");
    EXPECT_EQ(toString(make_address("fd03:f9ab:25de:89ec:1234:5678:90ab:cdef")),
              "fd03:f9ab:25de:89ec:1234:5678:90ab:cdef");
}

TEST(IpToString, ReturnsCorrectIpStringForIpv4MappedIpv6Addresses)
{
    EXPECT_EQ(toString(make_address("::ffff:127.0.0.1")), "127.0.0.1");
}

TEST(ipv4VerifyIpAndGetBitcount, PositiveTests)
{
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("192.168.1.1", nullptr));

    uint8_t bits = 0;
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.255.255.0", &bits));
    EXPECT_EQ(bits, 24);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.255.255.128", &bits));
    EXPECT_EQ(bits, 25);
}

TEST(ipv4VerifyIpAndGetBitcount, NegativeTests)
{
    uint8_t bits = 0;
    // > 256 in any field
    EXPECT_FALSE(ipv4VerifyIpAndGetBitcount("256.0.0.0", &bits));
    EXPECT_FALSE(ipv4VerifyIpAndGetBitcount("1.256.0.0", &bits));
    EXPECT_FALSE(ipv4VerifyIpAndGetBitcount("1.1.256.0", &bits));
    EXPECT_FALSE(ipv4VerifyIpAndGetBitcount("1.1.1.256", &bits));

    // Non contiguous mask
    EXPECT_FALSE(ipv4VerifyIpAndGetBitcount("255.0.255.0", &bits));

    // Too many fields
    EXPECT_FALSE(ipv4VerifyIpAndGetBitcount("1.1.1.1.1", &bits));
    // Not enough fields
    EXPECT_FALSE(ipv4VerifyIpAndGetBitcount("1.1.1", &bits));

    // Empty string
    EXPECT_FALSE(ipv4VerifyIpAndGetBitcount("", &bits));
}
} // namespace
} // namespace redfish::ip_util
