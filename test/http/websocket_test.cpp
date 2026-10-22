// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "http/http_body.hpp"
#include "http/http_request.hpp"
#include "http/websocket.hpp"
#include "http/websocket_impl.hpp"
#include "sessions.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/beast/_experimental/test/stream.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <utility>

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
    ASSERT_FALSE(ec);
    req.session = session;

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

} // namespace
} // namespace websocket
} // namespace crow
