#include "http_request_class_decl.hpp"

namespace crow
{

Request::Request(boost::beast::http::request<boost::beast::http::string_body> reqIn,
        std::error_code& ec) :
    req(std::move(reqIn)),
    fields(req.base()), body(req.body())
{
    if (!setUrlInfo())
    {
        ec = std::make_error_code(std::errc::invalid_argument);
    }
}

boost::beast::http::verb Request::method() const
{
    return req.method();
}

std::string_view Request::getHeaderValue(std::string_view key) const
{
    return req[key];
}

std::string_view Request::getHeaderValue(boost::beast::http::field key) const
{
    return req[key];
}

std::string_view Request::methodString() const
{
    return req.method_string();
}

std::string_view Request::target() const
{
    return req.target();
}

bool Request::target(const std::string_view target)
{
    req.target(target);
    return setUrlInfo();
}

unsigned Request::version() const
{
    return req.version();
}

bool Request::isUpgrade() const
{
    return boost::beast::websocket::is_upgrade(req);
}

bool Request::keepAlive() const
{
    return req.keep_alive();
}

bool Request::setUrlInfo()
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

}