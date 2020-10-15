#pragma once

#include "common.hpp"
#include "sessions.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/url/url_view.hpp>

#include <string>
#include <string_view>
#include <system_error>

namespace crow
{

struct Request
{
    std::string_view url{};
    boost::urls::url_view urlView{};

    bool isSecure{false};

    boost::asio::io_context* ioService{};
    boost::asio::ip::address ipAddress{};

    std::shared_ptr<persistent_data::UserSession> session;

    std::string userRole{};
    Request(){};

    Request(const Request&) = delete;
    Request(const Request&&) = delete;
    Request& operator=(const Request&) = delete;
    Request& operator=(const Request&&) = delete;
    virtual ~Request() = default;

    virtual std::string& body() = 0;

    virtual const std::string& body() const = 0;

    virtual boost::beast::http::verb method() const = 0;

    virtual std::string_view getHeaderValue(std::string_view key) const = 0;

    virtual std::string_view
        getHeaderValue(boost::beast::http::field key) const = 0;

    virtual std::string_view methodString() const = 0;

    virtual std::string_view target() const = 0;

    virtual unsigned version() const = 0;

    virtual bool isUpgrade() const = 0;

    virtual bool keepAlive() const = 0;
};

} // namespace crow
