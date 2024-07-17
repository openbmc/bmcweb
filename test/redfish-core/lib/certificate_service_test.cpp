#include "certificate_service.hpp"

#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(CertificateServiceTest, isValidEmail)
{
    // Test cases for various email addresses that are valid
    EXPECT_TRUE(isValidEmail("test@example.com"));
    EXPECT_TRUE(isValidEmail("test.something+long.random@example.com"));
    EXPECT_TRUE(isValidEmail("test_1@example.co.uk"));
    EXPECT_TRUE(isValidEmail("test-2@example.org"));
    EXPECT_TRUE(isValidEmail("test.three@sub.example.com"));
    EXPECT_TRUE(isValidEmail("u@d.co"));

    // Test cases for various email addresses that are invalid
    EXPECT_FALSE(isValidEmail("test"));
    EXPECT_FALSE(isValidEmail("@example.com")); // no user name
    EXPECT_FALSE(isValidEmail("test@.com")); // no domain name
    EXPECT_FALSE(isValidEmail("")); // empty string
    EXPECT_FALSE(isValidEmail(" ")); // just a whitespace
    EXPECT_FALSE(isValidEmail("te st@example.com")); // space in local part
    EXPECT_FALSE(isValidEmail("test@ example.com")); // space in domain part
    EXPECT_FALSE(isValidEmail("test@example .com")); // space before TLD
    EXPECT_FALSE(isValidEmail("test@example,com")); // comma instead of period
    EXPECT_FALSE(isValidEmail("test@localhost")); // localhost
    EXPECT_FALSE(isValidEmail("test@example.com#")); // special character after domain name
    EXPECT_FALSE(isValidEmail("test@example..com")); // consecutive dots in domain part
    EXPECT_FALSE(isValidEmail("test@example.com.")); // trailing dot after domain name
}

} // namespace
} // namespace redfish
