#pragma once

#include "utility.hpp"

#include <boost/beast/http/file_body.hpp>
namespace bmcweb
{
struct Base64FileBody
{
    /// The type of File this body uses
    using file_type = boost::beast::http::file_body::file_type;

    using reader = boost::beast::http::file_body::reader;

    // Algorithm for retrieving buffers when serializing.
    class writer;

    // The type of the @ref message::body member.
    using value_type = boost::beast::http::file_body::value_type;

    /** Returns the size of the body

        @param body The file body to use
    */
    static std::uint64_t size(const value_type& body)
    {
        return ((boost::beast::http::file_body::size(body) + 2) / 3) * 4;
    }
};

class Base64FileBody::writer
{
  public:
    using Base = boost::beast::http::file_body::writer;
    using const_buffers_type = boost::asio::const_buffer;

  private:
    std::string buf_;
    std::string fromlast;
    Base base;

  public:
    template <bool isRequest, class Fields>
    writer(boost::beast::http::header<isRequest, Fields>& h, value_type& b) :
        base(h, b)
    {}

    void init(boost::beast::error_code& ec)
    {
        base.init(ec);
    }

    boost::optional<std::pair<const_buffers_type, bool>>
        get(boost::beast::error_code& ec)
    {
        boost::optional<std::pair<const_buffers_type, bool>> ret = base.get(ec);
        if (!ret)
        {
            return ret;
        }

        auto chunkView =
            std::string_view(static_cast<const char*>(ret.get().first.data()),
                             ret.get().first.size());
        fromlast.append(chunkView);
        size_t reminder{0};
        if (ret.get().second)
        {
            reminder = fromlast.length() % 3;
        }
        auto view = std::string_view{fromlast.data(),
                                     fromlast.length() - reminder};

        buf_ = std::move(crow::utility::base64encode(view));
        fromlast = fromlast.substr(fromlast.length() - reminder);
        return {
            {const_buffers_type{buf_.data(), buf_.length()}, ret.get().second}};
    }
};
} // namespace bmcweb
