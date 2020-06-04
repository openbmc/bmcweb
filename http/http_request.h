#pragma once

#include "common.h"
#include "query_string.h"

#include "sessions.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>

#if BOOST_VERSION >= 107000
#include <boost/beast/ssl/ssl_stream.hpp>
#else
#include <boost/beast/experimental/core/ssl_stream.hpp>
#endif

namespace crow
{

struct Request
{
#ifdef BMCWEB_ENABLE_SSL
    using Adaptor = boost::beast::ssl_stream<boost::asio::ip::tcp::socket>;
#else
    using Adaptor = boost::asio::ip::tcp::socket;
#endif

    boost::beast::http::request<boost::beast::http::string_body>& req;
    boost::beast::http::fields& fields;
    std::string_view url{};
    QueryString urlParams{};
    bool isSecure{false};

    const std::string& body;

    void* middlewareContext{};
    boost::asio::io_context* ioService{};

    std::shared_ptr<crow::persistent_data::UserSession> session;

    std::string userRole{};
    std::function<Adaptor&()> socket;
    Request(
        boost::beast::http::request<boost::beast::http::string_body>& reqIn) :
        req(reqIn),
        fields(reqIn.base()), body(reqIn.body())
    {}

    boost::beast::http::verb method() const
    {
        return req.method();
    }

    const std::string_view getHeaderValue(std::string_view key) const
    {
        return req[key];
    }

    const std::string_view getHeaderValue(boost::beast::http::field key) const
    {
        return req[key];
    }

    const std::string_view methodString() const
    {
        return req.method_string();
    }

    const std::string_view target() const
    {
        return req.target();
    }

    unsigned version()
    {
        return req.version();
    }

    bool isUpgrade()
    {
        return boost::beast::websocket::is_upgrade(req);
    }

    bool keepAlive()
    {
        return req.keep_alive();
    }
};

} // namespace crow
