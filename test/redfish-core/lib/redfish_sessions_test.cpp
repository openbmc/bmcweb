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

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

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
    extendedInfo.emplace_back(nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"Message", message},
        {"MessageArgs", std::move(messageArgs)},
        {"MessageId", "Base.1.19.ResourceNotFound"},
        {"MessageSeverity", "Critical"},
        {"Resolution", "Provide a valid resource "
                       "identifier and resubmit the "
                       "request."}});

    return nlohmann::json{{"error",
                           {{"@Message.ExtendedInfo", std::move(extendedInfo)},
                            {"code", "Base.1.19.ResourceNotFound"},
                            {"message", std::move(message)}}}};
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

persistent_data::UserSession makeUserSession(
    std::string uniqueId, std::string username, std::string clientIp,
    std::string userRole, std::optional<std::string> clientId = std::nullopt)
{
    persistent_data::UserSession session;
    session.uniqueId = std::move(uniqueId);
    session.username = std::move(username);
    session.clientIp = std::move(clientIp);
    session.userRole = std::move(userRole);
    session.clientId = std::move(clientId);
    return session;
}

TEST(FillSessionObject, BaseFieldsAreExpected)
{
    persistent_data::UserSession session =
        makeUserSession("session-1", "test-user", "192.0.2.1", "priv-admin");
    crow::Response response;

    fillSessionObject(response, session);

    nlohmann::json& json = response.jsonValue;
    EXPECT_EQ(json.size(), 8);
    EXPECT_EQ(json["Id"], "session-1");
    EXPECT_EQ(json["UserName"], "test-user");
    EXPECT_EQ(json["Roles"].size(), 1);
    EXPECT_EQ(json["Roles"][0], "Administrator");
    EXPECT_EQ(json["@odata.id"],
              "/redfish/v1/SessionService/Sessions/session-1");
    EXPECT_EQ(json["@odata.type"], "#Session.v1_7_0.Session");
    EXPECT_EQ(json["Name"], "User Session");
    EXPECT_EQ(json["Description"], "Manager User Session");
    EXPECT_EQ(json["ClientOriginIPAddress"], "192.0.2.1");
}

TEST(FillSessionObject, ContextFieldIsIncludedWhenPresent)
{
    persistent_data::UserSession session = makeUserSession(
        "session-2", "test-user", "198.51.100.2", "priv-user", "webui-vue");
    crow::Response response;

    fillSessionObject(response, session);

    nlohmann::json& json = response.jsonValue;
    EXPECT_EQ(json.size(), 9);
    EXPECT_EQ(json["Id"], "session-2");
    EXPECT_EQ(json["UserName"], "test-user");
    EXPECT_EQ(json["Roles"].size(), 1);
    EXPECT_EQ(json["Roles"][0], "ReadOnly");
    EXPECT_EQ(json["@odata.id"],
              "/redfish/v1/SessionService/Sessions/session-2");
    EXPECT_EQ(json["@odata.type"], "#Session.v1_7_0.Session");
    EXPECT_EQ(json["Name"], "User Session");
    EXPECT_EQ(json["Description"], "Manager User Session");
    EXPECT_EQ(json["ClientOriginIPAddress"], "198.51.100.2");
    EXPECT_EQ(json["Context"], "webui-vue");
}

TEST(GetSessionCollectionMembers, EmptyCollectionReturnsNoMembers)
{
    clearAllSessions();

    nlohmann::json members = getSessionCollectionMembers();

    EXPECT_EQ(members.size(), 0);
    EXPECT_TRUE(members.empty());
}

TEST(GetSessionCollectionMembers, SessionMembersAreExpected)
{
    clearAllSessions();
    persistent_data::SessionStore& store =
        persistent_data::SessionStore::getInstance();
    const auto firstAddress = boost::asio::ip::make_address("192.0.2.10");
    const auto secondAddress = boost::asio::ip::make_address("192.0.2.11");

    std::shared_ptr<persistent_data::UserSession> firstSession =
        store.generateUserSession("alpha", firstAddress, std::nullopt,
                                  persistent_data::SessionType::Session);
    std::shared_ptr<persistent_data::UserSession> secondSession =
        store.generateUserSession("beta", secondAddress, std::nullopt,
                                  persistent_data::SessionType::Session);

    ASSERT_NE(firstSession, nullptr);
    ASSERT_NE(secondSession, nullptr);

    nlohmann::json members = getSessionCollectionMembers();
    std::vector<std::string> expectedPaths = {
        "/redfish/v1/SessionService/Sessions/" + firstSession->uniqueId,
        "/redfish/v1/SessionService/Sessions/" + secondSession->uniqueId};
    std::ranges::sort(expectedPaths);

    std::vector<std::string> memberPaths;
    memberPaths.reserve(members.size());
    for (const nlohmann::json& member : members)
    {
        EXPECT_EQ(member.size(), 1);
        memberPaths.emplace_back(member["@odata.id"].get<std::string>());
    }
    std::ranges::sort(memberPaths);

    EXPECT_EQ(members.size(), 2);
    EXPECT_EQ(memberPaths.size(), 2);
    EXPECT_EQ(memberPaths[0], expectedPaths[0]);
    EXPECT_EQ(memberPaths[1], expectedPaths[1]);

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
    expectedExtendedInfo.emplace_back(nlohmann::json{
        {"@odata.type", "#Message.v1_1_1.Message"},
        {"Message", message},
        {"MessageArgs", nlohmann::json::array()},
        {"MessageId", "Base.1.19.InsufficientPrivilege"},
        {"MessageSeverity", "Critical"},
        {"Resolution", resolution}});
    nlohmann::json expectedJson = {
        {"error",
         {{"@Message.ExtendedInfo", std::move(expectedExtendedInfo)},
          {"code", "Base.1.19.InsufficientPrivilege"},
          {"message", message}}}};

    EXPECT_EQ(response->res.result(), boost::beast::http::status::forbidden);
    EXPECT_EQ(response->res.jsonValue, expectedJson);
    EXPECT_NE(store.getSessionByUid(targetSession->uniqueId), nullptr);

    clearAllSessions();
}

} // namespace
} // namespace redfish
