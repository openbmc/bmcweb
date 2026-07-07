// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "app.hpp"
#include "async_resp.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "redfish_sessions.hpp"
#include "sessions.hpp"

#include <boost/asio/ip/address.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

nlohmann::json getSessionNotFoundError(std::string_view sessionId)
{
    std::string message = "The requested resource of type Session named '" +
                          std::string(sessionId) + "' was not found.";
    nlohmann::json messageArgs = nlohmann::json::array();
    messageArgs.emplace_back("Session");
    messageArgs.emplace_back(sessionId);
    nlohmann::json extendedInfo = nlohmann::json::array();
    nlohmann::json::object_t messageEntry;
    messageEntry["@odata.type"] = "#Message.v1_1_1.Message";
    messageEntry["Message"] = message;
    messageEntry["MessageArgs"] = std::move(messageArgs);
    messageEntry["MessageId"] = "Base.1.19.ResourceNotFound";
    messageEntry["MessageSeverity"] = "Critical";
    messageEntry["Resolution"] =
        "Provide a valid resource "
        "identifier and resubmit the "
        "request.";
    extendedInfo.emplace_back(std::move(messageEntry));

    nlohmann::json::object_t error;
    error["@Message.ExtendedInfo"] = std::move(extendedInfo);
    error["code"] = "Base.1.19.ResourceNotFound";
    error["message"] = std::move(message);

    nlohmann::json::object_t ret;
    ret["error"] = std::move(error);
    return ret;
}

void clearAllSessions()
{
    persistent_data::SessionStore& store =
        persistent_data::SessionStore::getInstance();
    for (const std::shared_ptr<persistent_data::UserSession>& session :
         store.getSessions())
    {
        store.removeSession(session);
    }
}

TEST(FillSessionObject, BaseFieldsAreExpected)
{
    clearAllSessions();
    persistent_data::SessionStore& store =
        persistent_data::SessionStore::getInstance();
    const auto clientAddress = boost::asio::ip::make_address("192.0.2.1");
    std::shared_ptr<persistent_data::UserSession> session =
        store.generateUserSession("test-user", clientAddress, std::nullopt,
                                  persistent_data::SessionType::Session);
    crow::Response response;

    ASSERT_NE(session, nullptr);
    session->userRole = "priv-admin";

    fillSessionObject(response, *session);

    nlohmann::json& json = response.jsonValue;
    EXPECT_EQ(json.size(), 8);
    EXPECT_EQ(json["Id"], session->uniqueId);
    EXPECT_EQ(json["UserName"], "test-user");
    EXPECT_EQ(json["Roles"].size(), 1);
    EXPECT_EQ(json["Roles"][0], "Administrator");
    EXPECT_EQ(json["@odata.id"],
              "/redfish/v1/SessionService/Sessions/" + session->uniqueId);
    EXPECT_EQ(json["@odata.type"], "#Session.v1_7_0.Session");
    EXPECT_EQ(json["Name"], "User Session");
    EXPECT_EQ(json["Description"], "Manager User Session");
    EXPECT_EQ(json["ClientOriginIPAddress"], "192.0.2.1");

    clearAllSessions();
}

TEST(FillSessionObject, ContextFieldIsIncludedWhenPresent)
{
    clearAllSessions();
    persistent_data::SessionStore& store =
        persistent_data::SessionStore::getInstance();
    const auto clientAddress = boost::asio::ip::make_address("198.51.100.2");
    std::shared_ptr<persistent_data::UserSession> session =
        store.generateUserSession("test-user", clientAddress, "webui-vue",
                                  persistent_data::SessionType::Session);
    crow::Response response;

    ASSERT_NE(session, nullptr);
    session->userRole = "priv-user";

    fillSessionObject(response, *session);

    nlohmann::json& json = response.jsonValue;
    EXPECT_EQ(json.size(), 9);
    EXPECT_EQ(json["Id"], session->uniqueId);
    EXPECT_EQ(json["UserName"], "test-user");
    EXPECT_EQ(json["Roles"].size(), 1);
    EXPECT_EQ(json["Roles"][0], "ReadOnly");
    EXPECT_EQ(json["@odata.id"],
              "/redfish/v1/SessionService/Sessions/" + session->uniqueId);
    EXPECT_EQ(json["@odata.type"], "#Session.v1_7_0.Session");
    EXPECT_EQ(json["Name"], "User Session");
    EXPECT_EQ(json["Description"], "Manager User Session");
    EXPECT_EQ(json["ClientOriginIPAddress"], "198.51.100.2");
    EXPECT_EQ(json["Context"], "webui-vue");

    clearAllSessions();
}

TEST(HandleSessionGet, MissingSessionReturnsNotFound)
{
    clearAllSessions();

    auto response = std::make_shared<bmcweb::AsyncResp>();
    std::error_code errorCode;
    crow::Request request{{boost::beast::http::verb::get,
                           "/redfish/v1/SessionService/Sessions/missing", 11},
                          errorCode};
    crow::App app;

    ASSERT_FALSE(errorCode);

    handleSessionGet(app, request, response, "missing");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(
        response->res.getHeaderValue(boost::beast::http::field::link),
        "</redfish/v1/JsonSchemas/Session/Session.json>; rel=describedby");
    EXPECT_EQ(response->res.jsonValue, getSessionNotFoundError("missing"));
}

TEST(HandleSessionDelete, MissingSessionReturnsNotFound)
{
    clearAllSessions();

    auto response = std::make_shared<bmcweb::AsyncResp>();
    std::error_code errorCode;
    crow::Request request{{boost::beast::http::verb::delete_,
                           "/redfish/v1/SessionService/Sessions/missing", 11},
                          errorCode};
    crow::App app;

    ASSERT_FALSE(errorCode);

    handleSessionDelete(app, request, response, "missing");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(response->res.jsonValue, getSessionNotFoundError("missing"));
}

TEST(HandleSessionDelete, CrossUserDeleteWithoutConfigureUsersIsForbidden)
{
    clearAllSessions();
    persistent_data::SessionStore& store =
        persistent_data::SessionStore::getInstance();
    const auto requesterAddress = boost::asio::ip::make_address("203.0.113.21");
    const auto targetAddress = boost::asio::ip::make_address("203.0.113.22");

    std::shared_ptr<persistent_data::UserSession> requesterSession =
        store.generateUserSession("requester", requesterAddress, std::nullopt,
                                  persistent_data::SessionType::Session);
    std::shared_ptr<persistent_data::UserSession> targetSession =
        store.generateUserSession("target", targetAddress, std::nullopt,
                                  persistent_data::SessionType::Session);

    ASSERT_NE(requesterSession, nullptr);
    ASSERT_NE(targetSession, nullptr);
    requesterSession->userRole = "priv-user";

    auto response = std::make_shared<bmcweb::AsyncResp>();
    std::string targetUrl =
        "/redfish/v1/SessionService/Sessions/" + targetSession->uniqueId;
    std::error_code errorCode;
    crow::Request request{{boost::beast::http::verb::delete_, targetUrl, 11},
                          errorCode};
    request.session = requesterSession;
    crow::App app;

    ASSERT_FALSE(errorCode);

    handleSessionDelete(app, request, response, targetSession->uniqueId);

    constexpr std::string_view message =
        "There are insufficient privileges for the account or credentials "
        "associated with the current session to perform the requested "
        "operation.";
    constexpr std::string_view resolution =
        "Either abandon the operation or change the associated access rights "
        "and resubmit the request if the operation failed.";
    nlohmann::json expectedExtendedInfo = nlohmann::json::array();
    nlohmann::json::object_t expectedMessage;
    expectedMessage["@odata.type"] = "#Message.v1_1_1.Message";
    expectedMessage["Message"] = message;
    expectedMessage["MessageArgs"] = nlohmann::json::array();
    expectedMessage["MessageId"] = "Base.1.19.InsufficientPrivilege";
    expectedMessage["MessageSeverity"] = "Critical";
    expectedMessage["Resolution"] = resolution;
    expectedExtendedInfo.emplace_back(std::move(expectedMessage));
    nlohmann::json::object_t expectedError;
    expectedError["@Message.ExtendedInfo"] = std::move(expectedExtendedInfo);
    expectedError["code"] = "Base.1.19.InsufficientPrivilege";
    expectedError["message"] = message;
    nlohmann::json::object_t expectedJson;
    expectedJson["error"] = std::move(expectedError);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::forbidden);
    EXPECT_EQ(response->res.jsonValue, expectedJson);
    EXPECT_NE(store.getSessionByUid(targetSession->uniqueId), nullptr);

    clearAllSessions();
}

} // namespace
} // namespace redfish
