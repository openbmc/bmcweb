// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
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
        io.run_for(std::chrono::milliseconds(1));
    }
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

TEST(WebSocketConnection, ClosedWhenUserLosesAccess)
{
    boost::asio::io_context io;
    // Beast finds its websocket teardown for its own test stream only.
    boost::beast::test::stream stream(io);
    boost::beast::test::stream out(io);
    stream.connect(out);

    persistent_data::SessionStore& store =
        persistent_data::SessionStore::getInstance();
    std::shared_ptr<persistent_data::UserSession> session =
        store.generateUserSession("user1", boost::asio::ip::address(),
                                  std::nullopt,
                                  persistent_data::SessionType::Session);
    ASSERT_NE(session, nullptr);

    Request req = upgradeRequest(session);

    bool opened = false;
    bool closed = false;
    auto conn = std::make_shared<ConnectionImpl<boost::beast::test::stream>>(
        req.url(), session, std::move(stream),
        [&opened](Connection&) { opened = true; }, nullptr, nullptr,
        [&closed](Connection&, const std::string&) { closed = true; }, nullptr);
    conn->start(req);
    runUntil(io, opened);
    ASSERT_TRUE(opened);

    // Ending the session leaves the connection open (DSP0266 13.3.4.4).
    store.removeSession(session);
    io.run_for(std::chrono::milliseconds(10));
    EXPECT_FALSE(closed);

    store.removeSessionsByUsername("user1");
    runUntil(io, closed);
    EXPECT_TRUE(closed);
}

TEST(WebSocketConnection, NoMessageHandledAfterUserLosesAccess)
{
    boost::asio::io_context io;
    boost::beast::test::stream stream(io);
    boost::beast::test::stream out(io);
    stream.connect(out);

    persistent_data::SessionStore& store =
        persistent_data::SessionStore::getInstance();
    std::shared_ptr<persistent_data::UserSession> session =
        store.generateUserSession("user2", boost::asio::ip::address(),
                                  std::nullopt,
                                  persistent_data::SessionType::Session);
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
    store.removeSessionsByUsername("user2");
    finishMessage();
    runUntil(io, closed);
    EXPECT_TRUE(closed);
    EXPECT_EQ(messages, std::vector<std::string>{"one"});
}

} // namespace
} // namespace websocket
} // namespace crow
