// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "baserule.hpp"
#include "http_request.hpp"
#include "server_sent_event.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace crow
{

class SseSocketRule : public BaseRule
{
    using self_t = SseSocketRule;

  public:
    explicit SseSocketRule(const std::string& ruleIn);

    void validate() override;

    void handle(const Request& /*req*/,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::vector<std::string>& /*params*/) override;

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
    self_t& onclose(Func f)
    {
        closeHandler = f;
        return *this;
    }

  private:
    std::function<void(crow::sse_socket::Connection&, const crow::Request&)>
        openHandler;
    std::function<void(crow::sse_socket::Connection&)> closeHandler;
};

} // namespace crow
