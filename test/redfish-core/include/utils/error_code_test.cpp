// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "bmcweb_config.h"

#include "http_response.hpp"
#include "utils/error_code.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace redfish::error_code
{
namespace
{

TEST(PropogateErrorCode, 500IsWorst)
{
    constexpr std::array<unsigned, 7> codes = {100, 200, 300, 400,
                                               401, 500, 501};
    for (auto code : codes)
    {
        EXPECT_EQ(propogateErrorCode(500, code), 500);
        EXPECT_EQ(propogateErrorCode(code, 500), 500);
    }
}

TEST(PropogateErrorCode, 5xxAreWorseThanOthers)
{
    constexpr std::array<unsigned, 7> codes = {100, 200, 300, 400,
                                               401, 501, 502};
    for (auto code : codes)
    {
        EXPECT_EQ(propogateErrorCode(code, 505), 505);
        EXPECT_EQ(propogateErrorCode(505, code), 505);
    }
    EXPECT_EQ(propogateErrorCode(502, 501), 502);
    EXPECT_EQ(propogateErrorCode(501, 502), 502);
    EXPECT_EQ(propogateErrorCode(503, 502), 503);
}

TEST(PropogateErrorCode, 401IsWorseThanOthers)
{
    constexpr std::array<unsigned, 7> codes = {100, 200, 300, 400, 401};
    for (auto code : codes)
    {
        EXPECT_EQ(propogateErrorCode(code, 401), 401);
        EXPECT_EQ(propogateErrorCode(401, code), 401);
    }
}

TEST(PropogateErrorCode, 4xxIsWorseThanOthers)
{
    constexpr std::array<unsigned, 7> codes = {100, 200, 300, 400, 402};
    for (auto code : codes)
    {
        EXPECT_EQ(propogateErrorCode(code, 405), 405);
        EXPECT_EQ(propogateErrorCode(405, code), 405);
    }
    EXPECT_EQ(propogateErrorCode(400, 402), 402);
    EXPECT_EQ(propogateErrorCode(402, 403), 403);
    EXPECT_EQ(propogateErrorCode(403, 402), 403);
}

TEST(PropogateError, IntermediateNoErrorMessageMakesNoChange)
{
    crow::Response intermediate;
    intermediate.result(boost::beast::http::status::ok);

    crow::Response finalRes;
    finalRes.result(boost::beast::http::status::ok);
    propogateError(finalRes, intermediate);
    EXPECT_EQ(finalRes.result(), boost::beast::http::status::ok);
    EXPECT_EQ(finalRes.jsonValue.find("error"), finalRes.jsonValue.end());
}

TEST(PropogateError, ErrorsArePropergatedWithErrorInRoot)
{
    nlohmann::json root = R"(
{
    "@odata.type": "#Message.v1_1_1.Message",
    "Message": "The request failed due to an internal service error.  The service is still operational.",
    "MessageArgs": [],
    "MessageId": "Base.1.13.0.InternalError",
    "MessageSeverity": "Critical",
    "Resolution": "Resubmit the request.  If the problem persists, consider resetting the service."
}
)"_json;
    crow::Response intermediate;
    intermediate.result(boost::beast::http::status::internal_server_error);
    intermediate.jsonValue = root;

    crow::Response final;
    final.result(boost::beast::http::status::ok);

    propogateError(final, intermediate);

    EXPECT_EQ(final.jsonValue["error"]["code"].get<std::string>(),
              "Base.1.13.0.InternalError");
    EXPECT_EQ(
        final.jsonValue["error"]["message"].get<std::string>(),
        "The request failed due to an internal service error.  The service is still operational.");
    EXPECT_EQ(intermediate.jsonValue, R"({})"_json);
    EXPECT_EQ(final.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(PropogateError, ErrorsArePropergatedWithErrorCode)
{
    crow::Response intermediate;
    intermediate.result(boost::beast::http::status::internal_server_error);

    nlohmann::json error = R"(
{
    "error": {
        "@Message.ExtendedInfo": [],
        "code": "Base.1.13.0.InternalError",
        "message": "The request failed due to an internal service error.  The service is still operational."
    }
}
)"_json;
    nlohmann::json extendedInfo = R"(
{
    "@odata.type": "#Message.v1_1_1.Message",
    "Message": "The request failed due to an internal service error.  The service is still operational.",
    "MessageArgs": [],
    "MessageId": "Base.1.13.0.InternalError",
    "MessageSeverity": "Critical",
    "Resolution": "Resubmit the request.  If the problem persists, consider resetting the service."
}
)"_json;

    for (int i = 0; i < 10; ++i)
    {
        error["error"][messages::messageAnnotation].push_back(extendedInfo);
    }
    intermediate.jsonValue = error;
    crow::Response final;
    final.result(boost::beast::http::status::ok);

    propogateError(final, intermediate);
    EXPECT_EQ(final.jsonValue["error"][messages::messageAnnotation],
              error["error"][messages::messageAnnotation]);
    std::string errorCode = messages::messageVersionPrefix;
    errorCode += "GeneralError";
    std::string errorMessage =
        "A general error has occurred. See Resolution for "
        "information on how to resolve the error.";
    EXPECT_EQ(final.jsonValue["error"]["code"].get<std::string>(), errorCode);
    EXPECT_EQ(final.jsonValue["error"]["message"].get<std::string>(),
              errorMessage);
    EXPECT_EQ(intermediate.jsonValue, R"({})"_json);
    EXPECT_EQ(final.result(),
              boost::beast::http::status::internal_server_error);
}

} // namespace
} // namespace redfish::error_code
