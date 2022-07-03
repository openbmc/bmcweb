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
} // namespace
} // namespace redfish::ip_util