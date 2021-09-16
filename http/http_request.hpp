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
#include <system_error>

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
    Request(boost::beast::http::request<boost::beast::http::string_body> reqIn,
            std::error_code& ec) :
        req(std::move(reqIn)),
        fields(req.base()), body(req.body())
    {
        if (!setUrlInfo())
        {
            ec = std::make_error_code(std::errc::invalid_argument);
        }
    }

    Request(const Request&) = delete;
    Request& operator=(const Request&) = delete;

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

    bool target(const std::string_view target)
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
        boost::urls::error_code ec;
        urlView = boost::urls::parse_relative_ref(
            boost::urls::string_view(target().data(), target().size()), ec);
        if (ec)
        {
            return false;
        }
        url = std::string_view(urlView.encoded_path().data(),
                               urlView.encoded_path().size());
        urlParams = urlView.query_params();
        return true;
    }
};

} // namespace crow
