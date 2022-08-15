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
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("128.0.0.0", &bits));
    EXPECT_EQ(bits, 1);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("192.0.0.0", &bits));
    EXPECT_EQ(bits, 2);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("224.0.0.0", &bits));
    EXPECT_EQ(bits, 3);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("240.0.0.0", &bits));
    EXPECT_EQ(bits, 4);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("248.0.0.0", &bits));
    EXPECT_EQ(bits, 5);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("252.0.0.0", &bits));
    EXPECT_EQ(bits, 6);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("254.0.0.0", &bits));
    EXPECT_EQ(bits, 7);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.0.0.0", &bits));
    EXPECT_EQ(bits, 8);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.128.0.0", &bits));
    EXPECT_EQ(bits, 9);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.192.0.0", &bits));
    EXPECT_EQ(bits, 10);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.224.0.0", &bits));
    EXPECT_EQ(bits, 11);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.240.0.0", &bits));
    EXPECT_EQ(bits, 12);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.248.0.0", &bits));
    EXPECT_EQ(bits, 13);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.252.0.0", &bits));
    EXPECT_EQ(bits, 14);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.254.0.0", &bits));
    EXPECT_EQ(bits, 15);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.255.0.0", &bits));
    EXPECT_EQ(bits, 16);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.255.128.0", &bits));
    EXPECT_EQ(bits, 17);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.255.192.0", &bits));
    EXPECT_EQ(bits, 18);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.255.224.0", &bits));
    EXPECT_EQ(bits, 19);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.255.240.0", &bits));
    EXPECT_EQ(bits, 20);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.255.248.0", &bits));
    EXPECT_EQ(bits, 21);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.255.252.0", &bits));
    EXPECT_EQ(bits, 22);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.255.254.0", &bits));
    EXPECT_EQ(bits, 23);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.255.255.0", &bits));
    EXPECT_EQ(bits, 24);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.255.255.128", &bits));
    EXPECT_EQ(bits, 25);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.255.255.192", &bits));
    EXPECT_EQ(bits, 26);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.255.255.224", &bits));
    EXPECT_EQ(bits, 27);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.255.255.240", &bits));
    EXPECT_EQ(bits, 28);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.255.255.248", &bits));
    EXPECT_EQ(bits, 29);
    EXPECT_TRUE(ipv4VerifyIpAndGetBitcount("255.255.255.252", &bits));
    EXPECT_EQ(bits, 30);
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
