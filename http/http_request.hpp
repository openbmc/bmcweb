#pragma once

#include "common.hpp"
#include "sessions.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/url/url_view.hpp>

#include <string>
#include <string_view>

namespace crow
{

struct Request
{
    boost::beast::http::request<boost::beast::http::string_body> req;
    boost::beast::http::fields& fields;
    std::string_view url{};
    boost::urls::url_view urlView{};
    boost::urls::query_params_view urlParams{};
    bool isSecure{false};

    const std::string& body;

    boost::asio::io_context* ioService{};
    boost::asio::ip::address ipAddress{};

    std::shared_ptr<persistent_data::UserSession> session;

    std::string userRole{};
    Request(
        boost::beast::http::request<boost::beast::http::string_body> reqIn) :
        req(std::move(reqIn)),
        fields(req.base()), body(req.body())
    {}

    bool read_headers()
    {
        // Note, despite the bmcweb coding policy on use of exceptions
        // for error handling, this one particular use of exceptions is
        // deemed acceptable, as it solved a significant error handling
        // problem that resulted in seg faults, the exact thing that the
        // exceptions rule is trying to avoid. If at some point,
        // boost::urls makes the parser object public (or we port it
        // into bmcweb locally) this will be replaced with
        // parser::parse, which returns a status code

        boost::urls::error_code ec;
        urlView = boost::urls::parse_relative_ref(
            boost::urls::string_view(req.target().data(), req.target().size()),
            ec);
        if (ec)
        {
            return false;
        }
        url = std::string_view(urlView.encoded_path().data(),
                               urlView.encoded_path().size());
        urlParams = urlView.query_params();
        return true;
    }

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
