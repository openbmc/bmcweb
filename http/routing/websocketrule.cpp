// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "websocketrule.hpp"

#include "async_resp.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "websocket_impl.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <memory>
#include <utility>

namespace crow
{
void WebSocketRule::handleUpgrade(
    const Request& req, const std::shared_ptr<bmcweb::AsyncResp>& /*asyncResp*/,
    boost::asio::ip::tcp::socket&& adaptor)
{
    BMCWEB_LOG_DEBUG("Websocket handles upgrade");
    std::shared_ptr<
        crow::websocket::ConnectionImpl<boost::asio::ip::tcp::socket>>
        myConnection = std::make_shared<
            crow::websocket::ConnectionImpl<boost::asio::ip::tcp::socket>>(
            req.url(), req.session, std::move(adaptor), openHandler,
            messageHandler, messageExHandler, closeHandler, errorHandler);
    myConnection->start(req);
}

void WebSocketRule::handleUpgrade(
    const Request& req, const std::shared_ptr<bmcweb::AsyncResp>& /*asyncResp*/,
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket>&& adaptor)
{
    BMCWEB_LOG_DEBUG("Websocket handles upgrade");
    std::shared_ptr<crow::websocket::ConnectionImpl<
        boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>>
        myConnection = std::make_shared<crow::websocket::ConnectionImpl<
            boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>>(
            req.url(), req.session, std::move(adaptor), openHandler,
            messageHandler, messageExHandler, closeHandler, errorHandler);
    myConnection->start(req);
}
} // namespace crow
