#include "bmcweb_config.h"

#include "account_service.hpp"
#include "async_resp.hpp"
#include "http_response.hpp"
#include "sessions.hpp"

#include <boost/beast/http/status.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <memory>
#include <optional>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Eq;
using ::testing::Optional;

namespace redfish
{
namespace
{

using json = nlohmann::json;

TEST(AuthMethodsPatch, RejectTLSDisabledWithoutPasswords)
{
    if constexpr (BMCWEB_PASSWORD_AUTH)
    {
        GTEST_SKIP() << "Requires password-auth=disabled";
    }

    auto& store = persistent_data::SessionStore::getInstance();
    auto original = store.getAuthMethodsConfig();
    bool originalNeedWrite = store.needWrite;
    auto& config = store.getAuthMethodsConfig();
    config.tls = true;
    config.cookie = true;
    config.sessionToken = true;
    config.xtoken = true;
    store.needWrite = false;

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    AuthMethods auth;
    auth.tls = false;
    // A rejected TLS change must not apply other auth fields in the PATCH.
    auth.cookie = false;
    auth.sessionToken = false;
    auth.xToken = false;
    handleAuthMethodsPatch(asyncResp, auth);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::bad_request);
    EXPECT_TRUE(config.tls);
    EXPECT_TRUE(config.cookie);
    EXPECT_TRUE(config.sessionToken);
    EXPECT_TRUE(config.xtoken);
    EXPECT_FALSE(store.needsWrite());

    config = original;
    store.needWrite = originalNeedWrite;
}

TEST(Conversion, PositiveToUint64)
{
    std::optional<uint64_t> passwordExpiration;

    passwordExpiration = passwordExpirationToUint64(nullptr);
    EXPECT_THAT(passwordExpiration, Optional(Eq(0)));

    passwordExpiration = passwordExpirationToUint64("1970-01-01T00:00:00");
    EXPECT_THAT(passwordExpiration, Eq(std::nullopt));

    passwordExpiration = passwordExpirationToUint64("2024-10-17T18:12:04");
    EXPECT_THAT(passwordExpiration, Optional(Eq(1729188724)));

    passwordExpiration = passwordExpirationToUint64("2024-10-17T18:12:04Z");
    EXPECT_THAT(passwordExpiration, Optional(Eq(1729188724)));

    passwordExpiration =
        passwordExpirationToUint64("2024-10-17T18:12:04+03:00");
    EXPECT_THAT(passwordExpiration, Optional(Eq(1729177924)));
}

TEST(Conversion, NegativeToUint64)
{
    EXPECT_EQ(passwordExpirationToUint64("01-02-03"), std::nullopt);

    EXPECT_EQ(passwordExpirationToUint64("1912-02-03"), std::nullopt);

    EXPECT_EQ(passwordExpirationToUint64("2024-10-17T00:00"), std::nullopt);

    EXPECT_EQ(passwordExpirationToUint64("ABC"), std::nullopt);
}

TEST(Conversion, PositiveToJson)
{
    constexpr uint64_t unexpiringPasswordExpiration = 0;

    json value = passwordExpirationToJson(unexpiringPasswordExpiration);
    EXPECT_TRUE(value.is_null());

    value = passwordExpirationToJson(1729188724);
    EXPECT_TRUE(value.is_string());
    EXPECT_EQ(value, R"("2024-10-17T18:12:04+00:00")"_json);
}

} // namespace
} // namespace redfish
