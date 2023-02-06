#pragma once

#include "common.hpp"
#include "sessions.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/url/url.hpp>

#include <string>
#include <string_view>
#include <system_error>

namespace crow
{

struct Request
{
    boost::beast::http::request<boost::beast::http::string_body> req;

  private:
    boost::urls::url urlBase{};

  public:
    bool isSecure{false};

    boost::asio::io_context* ioService{};
    boost::asio::ip::address ipAddress{};

    std::shared_ptr<persistent_data::UserSession> session;

    std::string userRole{};
    Request(boost::beast::http::request<boost::beast::http::string_body> reqIn,
            std::error_code& ec) :
        req(std::move(reqIn))
    {
        auto result = boost::urls::parse_relative_ref(target());

        if (!result)
        {
            ec = std::make_error_code(std::errc::invalid_argument);
            return;
        }
        urlBase = *result;
    }

    Request(const Request& other) = default;
    Request(Request&& other) = default;

    Request& operator=(const Request&) = delete;
    Request& operator=(const Request&&) = delete;
    ~Request() = default;

    boost::beast::http::verb method() const
    {
        return req.method();
    }

    std::string_view getHeaderValue(std::string_view key) const
    {
        return req[key];
    }

    std::string_view getHeaderValue(boost::beast::http::field key) const
    {
        return req[key];
    }

    std::string_view methodString() const
    {
        return req.method_string();
    }

    std::string_view target() const
    {
        return req.target();
    }

    boost::urls::url_view url() const
    {
        return {urlBase};
    }

    const boost::beast::http::fields& fields() const
    {
        return req.base();
    }

    const std::string& body() const
    {
        return req.body();
    }

    std::string& body()
    {
        return req.body();
    }

    unsigned version() const
    {
        return req.version();
    }

    bool isUpgrade() const
    {
        return boost::beast::websocket::is_upgrade(req);
    }

    bool keepAlive() const
    {
        return req.keep_alive();
    }
};

} // namespace crow
