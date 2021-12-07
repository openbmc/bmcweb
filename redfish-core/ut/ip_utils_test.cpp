#include "utils/ip_utils.hpp"

#include "gtest/gtest.h"

using redfish::ip_util::toString;
using boost::asio::ip::make_address;

TEST(IpToString, v4mapped)
{
    EXPECT_EQ(toString(make_address("127.0.0.1")), "127.0.0.1");
    EXPECT_EQ(toString(make_address("192.168.1.1")), "192.168.1.1");
    EXPECT_EQ(toString(make_address("::1")), "::1");
    EXPECT_EQ(toString(make_address("fd03:f9ab:25de:89ec::0001")),
              "fd03:f9ab:25de:89ec::1");
    EXPECT_EQ(toString(make_address("fd03:f9ab:25de:89ec::1234:abcd")),
              "fd03:f9ab:25de:89ec::1234:abcd");
    EXPECT_EQ(toString(make_address("fd03:f9ab:25de:89ec:1234:5678:90ab:cdef")),
              "fd03:f9ab:25de:89ec:1234:5678:90ab:cdef");
    EXPECT_EQ(toString(make_address("::ffff:127.0.0.1")), "127.0.0.1");
}
