#pragma once

#include "baserule.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "server_sent_event.hpp"

#include <boost/beast/http/verb.hpp>

#include <functional>
#include <memory>
#include <string>

namespace crow
{

class SseSocketRule : public BaseRule
{
    using self_t = SseSocketRule;

  public:
    explicit SseSocketRule(const std::string& ruleIn) : BaseRule(ruleIn) {}

    void validate() override {}

    void handle(const Request& /*req*/,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::vector<std::string>& /*params*/) override
    {
        asyncResp->res.result(boost::beast::http::status::not_found);
    }

    void handleUpgrade(const Request& /*req*/,
                       const std::shared_ptr<bmcweb::AsyncResp>& /*asyncResp*/,
                       boost::asio::ip::tcp::socket&& adaptor) override
    {
        std::shared_ptr<
            crow::sse_socket::ConnectionImpl<boost::asio::ip::tcp::socket>>
            myConnection = std::make_shared<
                crow::sse_socket::ConnectionImpl<boost::asio::ip::tcp::socket>>(
                std::move(adaptor), openHandler, closeHandler);
        myConnection->start();
    }
    void handleUpgrade(const Request& /*req*/,
                       const std::shared_ptr<bmcweb::AsyncResp>& /*asyncResp*/,
                       boost::asio::ssl::stream<boost::asio::ip::tcp::socket>&&
                           adaptor) override
    {
        std::shared_ptr<crow::sse_socket::ConnectionImpl<
            boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>>
            myConnection = std::make_shared<crow::sse_socket::ConnectionImpl<
                boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>>(
                std::move(adaptor), openHandler, closeHandler);
        myConnection->start();
    }

    template <typename Func>
    self_t& onopen(Func f)
    {
        openHandler = f;
        return *this;
    }

    template <typename Func>
    self_t& onclose(Func f)
    {
        closeHandler = f;
        return *this;
    }

  private:
    std::function<void(crow::sse_socket::Connection&)> openHandler;
    std::function<void(crow::sse_socket::Connection&)> closeHandler;
};

} // namespace crow
