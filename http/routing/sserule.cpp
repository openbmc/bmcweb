// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "sserule.hpp"

#include "async_resp.hpp"
#include "baserule.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "server_sent_event_impl.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/http/status.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace crow
{

SseSocketRule::SseSocketRule(const std::string& ruleIn) : BaseRule(ruleIn)
{
    isUpgrade = true;
    // Clear GET handler
    methodsBitfield = 0;
}

void SseSocketRule::validate() {}

void SseSocketRule::handle(const Request& /*req*/,
                           const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::vector<std::string>& /*params*/)
{
    BMCWEB_LOG_ERROR(
        "Handle called on websocket rule.  This should never happen");
    asyncResp->res.result(boost::beast::http::status::internal_server_error);
}

void SseSocketRule::handleUpgrade(
    const Request& req, const std::shared_ptr<bmcweb::AsyncResp>& /*asyncResp*/,
    boost::asio::ip::tcp::socket&& adaptor)
{
    std::shared_ptr<
        crow::sse_socket::ConnectionImpl<boost::asio::ip::tcp::socket>>
        myConnection = std::make_shared<
            crow::sse_socket::ConnectionImpl<boost::asio::ip::tcp::socket>>(
            std::move(adaptor), openHandler, closeHandler);
    myConnection->start(req);
}
void SseSocketRule::handleUpgrade(
    const Request& req, const std::shared_ptr<bmcweb::AsyncResp>& /*asyncResp*/,
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket>&& adaptor)
{
    std::shared_ptr<crow::sse_socket::ConnectionImpl<
        boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>>
        myConnection = std::make_shared<crow::sse_socket::ConnectionImpl<
            boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>>(
            std::move(adaptor), openHandler, closeHandler);
    myConnection->start(req);
}

} // namespace crow
