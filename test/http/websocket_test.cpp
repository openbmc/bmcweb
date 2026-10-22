// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "connection_manager.hpp"
#include "http/http_body.hpp"
#include "http/http_request.hpp"
#include "http/websocket.hpp"
#include "http/websocket_impl.hpp"
#include "sessions.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/_experimental/test/stream.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

namespace crow
{
namespace websocket
{
namespace
{

void runUntil(boost::asio::io_context& io, const bool& done)
{
    for (int i = 0; i < 1000 && !done; i++)
    {
        // run_for() leaves the context stopped once it runs out of work, as
        // it does while a connection defers its reads.
        io.restart();
        io.run_for(std::chrono::milliseconds(1));
    }
}

void runFor(boost::asio::io_context& io, std::chrono::milliseconds time)
{
    io.restart();
    io.run_for(time);
}

Request upgradeRequest(
    const std::shared_ptr<persistent_data::UserSession>& session)
{
    using boost::beast::http::field;
    boost::beast::http::request<bmcweb::HttpBody> upgrade(
        boost::beast::http::verb::get, "/websocket", 11);
    upgrade.set(field::host, "localhost");
    upgrade.set(field::connection, "Upgrade");
    upgrade.set(field::upgrade, "websocket");
    upgrade.set(field::sec_websocket_version, "13");
    upgrade.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    std::error_code ec;
    Request req(std::move(upgrade), ec);
    EXPECT_FALSE(ec);
    req.session = session;
    return req;
}

// A short text frame from a client: FIN and the text opcode, the mask bit
// and the length, then a zero mask key, so the payload goes as it is.
std::string clientTextFrame(std::string_view payload)
{
    std::string frame{'\x81', static_cast<char>(0x80U | payload.size())};
    frame.append(4, '\0');
    frame += payload;
    return frame;
}

// A close frame from a client: FIN and the close opcode, the mask bit and no
// payload, then a zero mask key.
std::string clientCloseFrame()
{
    std::string frame{'\x88', '\x80'};
    frame.append(4, '\0');
    return frame;
}

// What the server sends to close with the reason "User lost access": FIN and
// the close opcode, the length, then code 1000 and the reason.
constexpr std::string_view userLostAccessClose =
    "\x88\x12\x03\xe8User lost access";

std::shared_ptr<persistent_data::UserSession> makeSession(
    const std::string& username)
{
    return persistent_data::SessionStore::getInstance().generateUserSession(
        username, boost::asio::ip::address(), std::nullopt,
        persistent_data::SessionType::Session);
}

TEST(WebSocketConnection, ClosedWhenUserLosesAccess)
{
    boost::asio::io_context io;
    // Beast finds its websocket teardown for its own test stream only.
    boost::beast::test::stream stream(io);
    boost::beast::test::stream out(io);
    stream.connect(out);

    std::shared_ptr<persistent_data::UserSession> session =
        makeSession("user1");
    ASSERT_NE(session, nullptr);
    Request req = upgradeRequest(session);

    bool opened = false;
    bool closed = false;
    auto conn = std::make_shared<ConnectionImpl<boost::beast::test::stream>>(
        req.url(), session, std::move(stream),
        [&opened](Connection&) { opened = true; }, nullptr, nullptr,
        [&closed](Connection&, const std::string&) { closed = true; }, nullptr);
    ConnectionManager::getInstance().add(session, conn);
    conn->start(req);
    runUntil(io, opened);
    ASSERT_TRUE(opened);

    persistent_data::SessionStore& store =
        persistent_data::SessionStore::getInstance();
    // Ending the session alone leaves the connection open.
    store.removeSession(session);
    runFor(io, std::chrono::milliseconds(10));
    EXPECT_FALSE(closed);
    EXPECT_FALSE(out.str().ends_with(userLostAccessClose));

    // The user losing access starts a close handshake.
    store.removeSessionsByUsername("user1");
    runFor(io, std::chrono::milliseconds(10));
    EXPECT_TRUE(out.str().ends_with(userLostAccessClose));

    boost::asio::write(out, boost::asio::buffer(clientCloseFrame()));
    runUntil(io, closed);
    EXPECT_TRUE(closed);
}

TEST(WebSocketConnection, NoMessageHandledAfterUserLosesAccess)
{
    boost::asio::io_context io;
    boost::beast::test::stream stream(io);
    boost::beast::test::stream out(io);
    stream.connect(out);

    std::shared_ptr<persistent_data::UserSession> session =
        makeSession("user2");
    ASSERT_NE(session, nullptr);
    Request req = upgradeRequest(session);

    bool opened = false;
    bool handling = false;
    bool closed = false;
    std::vector<std::string> messages;
    std::function<void()> finishMessage;
    auto conn = std::make_shared<ConnectionImpl<boost::beast::test::stream>>(
        req.url(), session, std::move(stream),
        [&opened](Connection&) { opened = true; }, nullptr,
        [&handling, &messages,
         &finishMessage](Connection&, std::string_view msg, MessageType,
                         std::function<void()>&& whenComplete) {
            handling = true;
            messages.emplace_back(msg);
            finishMessage = std::move(whenComplete);
        },
        [&closed](Connection&, const std::string&) { closed = true; }, nullptr);
    ConnectionManager::getInstance().add(session, conn);
    conn->start(req);
    runUntil(io, opened);
    ASSERT_TRUE(opened);

    // Both messages arrive in one read, so the second one is already in the
    // websocket's buffer when the first one is handled.
    std::string frames = clientTextFrame("one") + clientTextFrame("two");
    boost::asio::write(out, boost::asio::buffer(frames));
    runUntil(io, handling);
    ASSERT_TRUE(handling);

    // The user loses access while the first message is being handled.
    persistent_data::SessionStore::getInstance().removeSessionsByUsername(
        "user2");
    finishMessage();
    boost::asio::write(out, boost::asio::buffer(clientCloseFrame()));
    runUntil(io, closed);
    EXPECT_TRUE(closed);
    EXPECT_EQ(messages, std::vector<std::string>{"one"});
}

TEST(WebSocketConnection, ClosedWhenUserLosesAccessWhileReadDeferred)
{
    boost::asio::io_context io;
    boost::beast::test::stream stream(io);
    boost::beast::test::stream out(io);
    stream.connect(out);

    std::shared_ptr<persistent_data::UserSession> session =
        makeSession("user3");
    ASSERT_NE(session, nullptr);
    Request req = upgradeRequest(session);

    bool opened = false;
    bool closed = false;
    auto conn = std::make_shared<ConnectionImpl<boost::beast::test::stream>>(
        req.url(), session, std::move(stream),
        [&opened](Connection& c) {
            // Like the host console and virtual media routes, hold off
            // reading while the route sets up.
            c.deferRead();
            opened = true;
        },
        nullptr, nullptr,
        [&closed](Connection&, const std::string&) { closed = true; }, nullptr);
    std::weak_ptr<Connection> weak = conn;
    ConnectionManager::getInstance().add(session, conn);
    conn->start(req);
    runUntil(io, opened);
    ASSERT_TRUE(opened);

    // The user loses access while the route sets up: the client is sent the
    // close, and the close handler runs once the route's setup is done.
    persistent_data::SessionStore::getInstance().removeSessionsByUsername(
        "user3");
    boost::asio::write(out, boost::asio::buffer(clientCloseFrame()));
    runFor(io, std::chrono::milliseconds(10));
    EXPECT_TRUE(out.str().ends_with(userLostAccessClose));
    EXPECT_FALSE(closed);

    conn->resumeRead();
    conn.reset();
    runUntil(io, closed);
    EXPECT_TRUE(closed);
    runFor(io, std::chrono::milliseconds(10));
    EXPECT_TRUE(weak.expired());
}

TEST(WebSocketConnection, ClosedBeforeAccepted)
{
    boost::asio::io_context io;
    boost::beast::test::stream stream(io);
    boost::beast::test::stream out(io);
    stream.connect(out);

    std::shared_ptr<persistent_data::UserSession> session =
        makeSession("user4");
    ASSERT_NE(session, nullptr);
    Request req = upgradeRequest(session);

    bool opened = false;
    auto conn = std::make_shared<ConnectionImpl<boost::beast::test::stream>>(
        req.url(), session, std::move(stream),
        [&opened](Connection&) { opened = true; }, nullptr, nullptr, nullptr,
        nullptr);
    std::weak_ptr<Connection> weak = conn;
    ConnectionManager::getInstance().add(session, conn);
    conn->start(req);
    conn.reset();

    // The user loses access before the upgrade response is written.
    persistent_data::SessionStore::getInstance().removeSessionsByUsername(
        "user4");
    runFor(io, std::chrono::milliseconds(10));
    EXPECT_FALSE(opened);
    EXPECT_TRUE(out.str().starts_with("HTTP/1.1 101"));
    EXPECT_FALSE(out.str().ends_with(userLostAccessClose));
    EXPECT_TRUE(weak.expired());
}

TEST(WebSocketConnection, CloseTwiceClosesOnce)
{
    boost::asio::io_context io;
    boost::beast::test::stream stream(io);
    boost::beast::test::stream out(io);
    stream.connect(out);
    Request req = upgradeRequest(nullptr);

    bool opened = false;
    int closeCount = 0;
    auto conn = std::make_shared<ConnectionImpl<boost::beast::test::stream>>(
        req.url(), nullptr, std::move(stream),
        [&opened](Connection&) { opened = true; }, nullptr, nullptr,
        [&closeCount](Connection&, const std::string&) { closeCount++; },
        nullptr);
    std::weak_ptr<Connection> weak = conn;
    conn->start(req);
    runUntil(io, opened);
    ASSERT_TRUE(opened);

    // A route and the user losing access can both close a connection.
    conn->close("first");
    conn->close("second");
    conn.reset();
    boost::asio::write(out, boost::asio::buffer(clientCloseFrame()));
    runFor(io, std::chrono::milliseconds(20));
    EXPECT_EQ(closeCount, 1);
    EXPECT_TRUE(weak.expired());
}

} // namespace
} // namespace websocket
} // namespace crow
