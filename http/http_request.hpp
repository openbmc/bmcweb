#pragma once

#include "http_body.hpp"
#include "sessions.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/url/url.hpp>

#include <string>
#include <string_view>
#include <system_error>

namespace crow
{

struct Request
{
    using Body = boost::beast::http::request<bmcweb::HttpBody>;
    Body req;

  private:
    boost::urls::url urlBase;

  public:
    boost::asio::io_context* ioService = nullptr;
    boost::asio::ip::address ipAddress;

    std::shared_ptr<persistent_data::UserSession> session;

    std::string userRole;
    Request(Body reqIn, std::error_code& ec) : req(std::move(reqIn))
    {
        if (!setUrlInfo())
        {
            ec = std::make_error_code(std::errc::invalid_argument);
        }
    }

    Request(std::string_view bodyIn, std::error_code& /*ec*/) : req({}, bodyIn)
    {}

    Request() = default;

    Request(const Request& other) = default;
    Request(Request&& other) = default;

    Request& operator=(const Request&) = default;
    Request& operator=(Request&&) = default;
    ~Request() = default;

    void addHeader(std::string_view key, std::string_view value)
    {
        req.insert(key, value);
    }

    void addHeader(boost::beast::http::field key, std::string_view value)
    {
        req.insert(key, value);
    }

    void clear()
    {
        req.clear();
        urlBase.clear();
        ioService = nullptr;
        ipAddress = boost::asio::ip::address();
        session = nullptr;
        userRole = "";
    }

    boost::beast::http::verb method() const
    {
        return req.method();
    }
    void method(boost::beast::http::verb verb)
    {
        req.method(verb);
    }

    std::string_view getHeaderValue(std::string_view key) const
    {
        return req[key];
    }

    std::string_view getHeaderValue(boost::beast::http::field key) const
    {
        return req[key];
    }

    void clearHeader(boost::beast::http::field key)
    {
        req.erase(key);
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
        return req.body().str();
    }

    bool target(std::string_view target)
    {
        req.target(target);
        return setUrlInfo();
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

  private:
    bool setUrlInfo()
    {
        auto result = boost::urls::parse_relative_ref(target());

        if (!result)
        {
            return false;
        }
        urlBase = *result;
        return true;
    }
};

} // namespace crow
