// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "authentication.hpp"

#include "sessions.hpp"

#include <gtest/gtest.h>
namespace crow::authentication
{
TEST(Authentication, csrfIsValid)
{
    persistent_data::UserSession session;
    EXPECT_FALSE(csrfIsValid(session, "1234567890abcdefghij"));
    EXPECT_FALSE(csrfIsValid(session, ""));

    session.csrfToken = "1234567890abcdefghij";
    EXPECT_TRUE(csrfIsValid(session, "1234567890abcdefghij"));
    EXPECT_FALSE(csrfIsValid(session, "1234567890abcdefghijk"));
    EXPECT_FALSE(csrfIsValid(session, ""));
    EXPECT_FALSE(csrfIsValid(session, "1234567890abcdefghijk"));
}
} // namespace crow::authentication
