#pragma once

#include <boost/beast/core/detail/base64.hpp>
#include <boost/beast/http/file_body.hpp>

namespace crow
{
namespace http = boost::beast::http;
namespace beast = boost::beast;
namespace net = boost::asio;
struct Base64FileBody
{
    /// The type of File this body uses
    using file_type = http::file_body::file_type;

    using reader = http::file_body::reader;

    // Algorithm for retrieving buffers when serializing.
    class writer;

    // The type of the @ref message::body member.
    using value_type = http::file_body::value_type;

    /** Returns the size of the body

        @param body The file body to use
    */
    static std::uint64_t size(const value_type& body)
    {
        return ((http::file_body::size(body) + 2) / 3) * 4;
    }
};

class Base64FileBody::writer : public http::file_body::writer
{
    char buf_[BOOST_BEAST_FILE_BUFFER_SIZE * 2]{0}; // Small buffer for reading

  public:
    using Base = http::file_body::writer;
    using const_buffers_type = net::const_buffer;

    template <bool isRequest, class Fields>
    writer(http::header<isRequest, Fields>& h, value_type& b) : Base(h, b)
    {}

    void init(beast::error_code& ec)
    {
        Base::init(ec);
    }

    boost::optional<std::pair<const_buffers_type, bool>>
        get(beast::error_code& ec)
    {
        auto ret = Base::get(ec);
        if (!ret)
        {
            return ret;
        }
        auto enclength = beast::detail::base64::encode(
            buf_, ret.get().first.data(), ret.get().first.size());
        return {{const_buffers_type{buf_, enclength}, ret.get().second}};
    }
};
} // namespace crow
