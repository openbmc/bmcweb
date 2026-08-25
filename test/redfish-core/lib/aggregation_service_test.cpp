// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "aggregation_service.hpp"
#include "http_response.hpp"

#include <boost/beast/http/status.hpp>
#include <nlohmann/json.hpp>

#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

const nlohmann::json& firstMessage(const crow::Response& res)
{
    return res.jsonValue["error"]["@Message.ExtendedInfo"][0];
}

// A malformed user-id is reported the way account_service.hpp:312 reports one,
// naming the property and quoting the value the client sent.
void expectFormatError(const crow::Response& res, const std::string& fieldName,
                       const std::string& value)
{
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    ASSERT_EQ(res.jsonValue["error"]["@Message.ExtendedInfo"].size(), 1);
    EXPECT_EQ(firstMessage(res)["MessageId"],
              "Base.1.19.PropertyValueFormatError");
    EXPECT_EQ(firstMessage(res)["MessageArgs"],
              nlohmann::json::array({nlohmann::json(value).dump(), fieldName}));
}

// An over-long value is reported by its limit rather than its content, so that
// a rejected Password does not reach the response body.
void expectTooLong(const crow::Response& res, const std::string& fieldName)
{
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    ASSERT_EQ(res.jsonValue["error"]["@Message.ExtendedInfo"].size(), 1);
    EXPECT_EQ(firstMessage(res)["MessageId"], "Base.1.19.StringValueTooLong");
    EXPECT_EQ(firstMessage(res)["MessageArgs"],
              nlohmann::json::array(
                  {fieldName, std::to_string(maxCredentialLength)}));
}

TEST(ValidateUserName, NotProvidedIsAccepted)
{
    crow::Response res;
    EXPECT_TRUE(validateUserName(std::nullopt, res));
    EXPECT_TRUE(res.jsonValue.empty());
}

TEST(ValidateUserName, EmptyIsAccepted)
{
    crow::Response res;
    EXPECT_TRUE(validateUserName(std::optional<std::string>(""), res));
    EXPECT_TRUE(res.jsonValue.empty());
}

TEST(ValidateUserName, ColonIsRejected)
{
    crow::Response res;
    EXPECT_FALSE(validateUserName(std::optional<std::string>("ro:ot"), res));
    expectFormatError(res, "UserName", "ro:ot");
}

TEST(ValidateUserName, MaximumLengthIsAccepted)
{
    crow::Response res;
    EXPECT_TRUE(validateUserName(
        std::optional<std::string>(std::string(maxCredentialLength, 'u')),
        res));
    EXPECT_TRUE(res.jsonValue.empty());
}

TEST(ValidateUserName, OverMaximumLengthIsRejected)
{
    crow::Response res;
    EXPECT_FALSE(validateUserName(
        std::optional<std::string>(std::string(maxCredentialLength + 1, 'u')),
        res));
    expectTooLong(res, "UserName");
}

TEST(ValidateUserName, OverMaximumLengthOmitsTheSubmittedValue)
{
    crow::Response res;
    std::string tooLong(maxCredentialLength + 1, 'u');
    EXPECT_FALSE(validateUserName(std::optional<std::string>(tooLong), res));
    EXPECT_EQ(res.jsonValue.dump().find(tooLong), std::string::npos);
}

TEST(ValidatePassword, NotProvidedIsAccepted)
{
    crow::Response res;
    EXPECT_TRUE(validatePassword(std::nullopt, res));
    EXPECT_TRUE(res.jsonValue.empty());
}

TEST(ValidatePassword, EmptyIsAccepted)
{
    crow::Response res;
    EXPECT_TRUE(validatePassword(std::optional<std::string>(""), res));
    EXPECT_TRUE(res.jsonValue.empty());
}

// The colon is legal in the password because a decoder splits on the first
// one, so everything after it round-trips intact.
TEST(ValidatePassword, ColonIsAccepted)
{
    crow::Response res;
    EXPECT_TRUE(
        validatePassword(std::optional<std::string>("p@ss:word!123"), res));
    EXPECT_TRUE(res.jsonValue.empty());
}

TEST(ValidatePassword, MaximumLengthIsAccepted)
{
    crow::Response res;
    EXPECT_TRUE(validatePassword(
        std::optional<std::string>(std::string(maxCredentialLength, 'p')),
        res));
    EXPECT_TRUE(res.jsonValue.empty());
}

TEST(ValidatePassword, OverMaximumLengthIsRejected)
{
    crow::Response res;
    EXPECT_FALSE(validatePassword(
        std::optional<std::string>(std::string(maxCredentialLength + 1, 'p')),
        res));
    expectTooLong(res, "Password");
}

TEST(ValidatePassword, OverMaximumLengthOmitsTheSubmittedValue)
{
    crow::Response res;
    std::string tooLong(maxCredentialLength + 1, 'p');
    EXPECT_FALSE(validatePassword(std::optional<std::string>(tooLong), res));
    EXPECT_EQ(res.jsonValue.dump().find(tooLong), std::string::npos);
}

} // namespace
} // namespace redfish
