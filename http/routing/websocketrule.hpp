#pragma once

#include "baserule.hpp"
#include "websocket.hpp"

#include <boost/beast/http/verb.hpp>

#include <memory>
#include <string>
#include <vector>

namespace crow
{
class WebSocketRule : public BaseRule
{
    using self_t = WebSocketRule;

  public:
    explicit WebSocketRule(const std::string& ruleIn) : BaseRule(ruleIn) {}

    void validate() override {}

    void handle(const Request& /*req*/,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::vector<std::string>& /*params*/) override
    {
        asyncResp->res.result(boost::beast::http::status::not_found);
    }

#ifndef BMCWEB_ENABLE_SSL
    void handleUpgrade(const Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& /*asyncResp*/,
                       boost::asio::ip::tcp::socket&& adaptor) override
    {
        BMCWEB_LOG_DEBUG("Websocket handles upgrade");
        std::shared_ptr<
            crow::websocket::ConnectionImpl<boost::asio::ip::tcp::socket>>
            myConnection = std::make_shared<
                crow::websocket::ConnectionImpl<boost::asio::ip::tcp::socket>>(
                req, req.url(), std::move(adaptor), openHandler, messageHandler,
                messageExHandler, closeHandler, errorHandler);
        myConnection->start();
    }
#else
    void handleUpgrade(const Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& /*asyncResp*/,
                       boost::beast::ssl_stream<boost::asio::ip::tcp::socket>&&
                           adaptor) override
    {
        BMCWEB_LOG_DEBUG("Websocket handles upgrade");
        std::shared_ptr<crow::websocket::ConnectionImpl<
            boost::beast::ssl_stream<boost::asio::ip::tcp::socket>>>
            myConnection = std::make_shared<crow::websocket::ConnectionImpl<
                boost::beast::ssl_stream<boost::asio::ip::tcp::socket>>>(
                req, req.url(), std::move(adaptor), openHandler, messageHandler,
                messageExHandler, closeHandler, errorHandler);
        myConnection->start();
    }
#endif

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
    std::function<void(crow::websocket::Connection&)> openHandler;
    std::function<void(crow::websocket::Connection&, std::string_view, bool)>
        messageHandler;
    std::function<void(crow::websocket::Connection&, std::string_view,
                       crow::websocket::MessageType type,
                       bool,
                       std::function<void()>&& whenComplete)>
        messageExHandler;
    std::function<void(crow::websocket::Connection&, const std::string&)>
        closeHandler;
    std::function<void(crow::websocket::Connection&)> errorHandler;
};
} // namespace crow
