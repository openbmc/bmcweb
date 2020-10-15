#pragma once

#include "common.hpp"
#include "http/http_request.hpp"
#include "sessions.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/url/url_view.hpp>

namespace crow
{

struct RequestImpl : Request
{
    boost::beast::http::request<boost::beast::http::string_body> req;

    RequestImpl(
        boost::beast::http::request<boost::beast::http::string_body>&& reqIn,
        std::error_code& ec) :
        Request(),
        req(std::move(reqIn))
    {
        if (!setUrlInfo())
        {
            ec = std::make_error_code(std::errc::invalid_argument);
        }
    }

    RequestImpl(
        const boost::beast::http::request<boost::beast::http::string_body>& reqIn,
        std::error_code& ec) :
        req(reqIn)
    {
        if (!setUrlInfo())
        {
            ec = std::make_error_code(std::errc::invalid_argument);
        }
    }

    std::string& body() override
    {
        return req.body();
    }
    
    const std::string& body() const override
    {
        return req.body();
    }

    boost::beast::http::verb method() const override
    {
        return req.method();
    }

    std::string_view getHeaderValue(std::string_view key) const override
    {
        return req[key];
    }

    std::string_view
        getHeaderValue(boost::beast::http::field key) const override
    {
        return req[key];
    }

    std::string_view methodString() const override
    {
        return req.method_string();
    }

    std::string_view target() const override
    {
        return req.target();
    }

    unsigned version() const override
    {
        return req.version();
    }

    bool isUpgrade() const override
    {
        return boost::beast::websocket::is_upgrade(req);
    }

    bool keepAlive() const override
    {
        return req.keep_alive();
    }

  private:
    bool setUrlInfo()
    {
        auto result = boost::urls::parse_relative_ref(
            boost::urls::string_view(target().data(), target().size()));

        if (!result)
        {
            return false;
        }
        urlView = *result;
        url = std::string_view(urlView.encoded_path().data(),
                               urlView.encoded_path().size());
        return true;
    }
};

} // namespace crow
