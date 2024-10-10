#pragma once

#include "baserule.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "server_sent_event.hpp"

#include <boost/beast/http/verb.hpp>

#include <functional>
#include <memory>
#include <string>

namespace bmcweb
{

class SseSocketRule : public BaseRule
{
    using self_t = SseSocketRule;

  public:
    explicit SseSocketRule(const std::string& ruleIn) : BaseRule(ruleIn)
    {
        isUpgrade = true;
        // Clear GET handler
        methodsBitfield = 0;
    }

    void validate() override {}

    void handle(const Request& /*req*/,
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
                       boost::asio::ip::tcp::socket&& adaptor) override
    {
        std::shared_ptr<
            sse_socket::ConnectionImpl<boost::asio::ip::tcp::socket>>
            myConnection = std::make_shared<
                sse_socket::ConnectionImpl<boost::asio::ip::tcp::socket>>(
                std::move(adaptor), openHandler, closeHandler);
        myConnection->start(req);
    }
    void handleUpgrade(const Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& /*asyncResp*/,
                       boost::asio::ssl::stream<boost::asio::ip::tcp::socket>&&
                           adaptor) override
    {
        std::shared_ptr<sse_socket::ConnectionImpl<
            boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>>
            myConnection = std::make_shared<sse_socket::ConnectionImpl<
                boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>>(
                std::move(adaptor), openHandler, closeHandler);
        myConnection->start(req);
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
    std::function<void(sse_socket::Connection&, const Request&)> openHandler;
    std::function<void(sse_socket::Connection&)> closeHandler;
};

} // namespace bmcweb

namespace crow = bmcweb;
