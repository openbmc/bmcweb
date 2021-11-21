#pragma once
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/url/url_view.hpp>

#include <string>
#include <string_view>
#include <system_error>

#include "sessions.hpp"

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
            std::error_code& ec);

    boost::beast::http::verb method() const;

    std::string_view getHeaderValue(std::string_view key) const;

    std::string_view getHeaderValue(boost::beast::http::field key) const;

    std::string_view methodString() const;

    std::string_view target() const;

    bool target(const std::string_view target);

    unsigned version() const;

    bool isUpgrade() const;

    bool keepAlive() const;

  private:
    bool setUrlInfo();
};

}