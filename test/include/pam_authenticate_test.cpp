// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "bmcweb_config.h"

#include "pam_authenticate.hpp"

#include <security/_pam_types.h>
#include <security/pam_appl.h>

#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace
{
unsigned int& pamStartCalls()
{
    static unsigned int calls = 0;
    return calls;
}
} // namespace

// Fail before touching the host PAM configuration. Disabled password
// authentication must return without even starting a PAM transaction.
extern "C" int pam_start(const char*, const char*, const struct pam_conv*,
                         pam_handle_t**)
{
    ++pamStartCalls();
    return PAM_SYSTEM_ERR;
}

namespace
{
TEST(PasswordAuthentication, Credentials)
{
    pamStartCalls() = 0;
    int result = pamAuthenticateUser("user", "password", std::nullopt);

    EXPECT_EQ(result, BMCWEB_PASSWORD_AUTH ? PAM_SYSTEM_ERR : PAM_AUTH_ERR);
    EXPECT_EQ(pamStartCalls(), BMCWEB_PASSWORD_AUTH ? 1U : 0U);
}

TEST(PasswordAuthentication, CredentialsWithMfa)
{
    pamStartCalls() = 0;
    int result = pamAuthenticateUser("user", "password", "123456");

    EXPECT_EQ(result, BMCWEB_PASSWORD_AUTH ? PAM_SYSTEM_ERR : PAM_AUTH_ERR);
    EXPECT_EQ(pamStartCalls(), BMCWEB_PASSWORD_AUTH ? 1U : 0U);
}

TEST(PasswordAuthentication, OversizedPassword)
{
    pamStartCalls() = 0;
    int result = pamAuthenticateUser("user", std::string(PAM_MAX_MSG_SIZE, 'x'),
                                     std::nullopt);

    EXPECT_EQ(result, BMCWEB_PASSWORD_AUTH ? PAM_CONV_ERR : PAM_AUTH_ERR);
    EXPECT_EQ(pamStartCalls(), 0U);
}
} // namespace
