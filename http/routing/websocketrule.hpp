// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "baserule.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "websocket.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/http/status.hpp>

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace crow
{
class WebSocketRule : public BaseRule
{
    using self_t = WebSocketRule;

  public:
    explicit WebSocketRule(const std::string& ruleIn) : BaseRule(ruleIn)
    {
        isUpgrade = true;
        // Clear GET handler
        methodsBitfield = 0;
    }

    void validate() override {}

    void handle(Request& /*req*/,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::vector<std::string>& /*params*/) override
    {
        BMCWEB_LOG_ERROR(
            "Handle called on websocket rule.  This should never happen");
        asyncResp->res.result(
            boost::beast::http::status::internal_server_error);
    }

    void handleUpgrade(const Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& /*asyncResp*/,
                       boost::asio::ip::tcp::socket&& adaptor) override;

    void handleUpgrade(const Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& /*asyncResp*/,
                       boost::asio::ssl::stream<boost::asio::ip::tcp::socket>&&
                           adaptor) override;

    template <typename Func>
    self_t& onopen(Func f)
    {
        openHandler = f;
        return *this;
    }

    template <typename Func>
    self_t& onmessage(Func f)
    {
        messageHandler = f;
        return *this;
    }

    template <typename Func>
    self_t& onmessageex(Func f)
    {
        messageExHandler = f;
        return *this;
    }

    template <typename Func>
    self_t& onclose(Func f)
    {
        closeHandler = f;
        return *this;
    }

    template <typename Func>
    self_t& onerror(Func f)
    {
        errorHandler = f;
        return *this;
    }

  protected:
    std::function<void(websocket::Connection&)> openHandler;
    std::function<void(websocket::Connection&, const std::string&, bool)>
        messageHandler;
    std::function<void(websocket::Connection&, std::string_view,
                       websocket::MessageType type,
                       std::function<void()>&& whenComplete)>
        messageExHandler;
    std::function<void(websocket::Connection&, const std::string&)>
        closeHandler;
    std::function<void(websocket::Connection&)> errorHandler;
};
} // namespace crow
