#pragma once

#include "utility.hpp"

#include <boost/beast/core/file_posix.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/system/error_code.hpp>

namespace bmcweb
{
struct FileBody
{
    class writer;
    class value_type;

    static std::uint64_t size(const value_type& body);
};

enum class EncodingType
{
    Raw,
    Base64,
};

class FileBody::value_type
{
    boost::beast::file_posix fileHandle;

    std::uint64_t fileSize = 0;

  public:
    EncodingType encodingType = EncodingType::Raw;

    ~value_type() = default;
    value_type() = default;
    explicit value_type(EncodingType enc) : encodingType(enc) {}
    value_type(value_type&& other) = default;
    value_type& operator=(value_type&& other) = default;
    value_type(const value_type& other) = delete;
    value_type& operator=(const value_type& other) = delete;

    boost::beast::file_posix& file()
    {
        return fileHandle;
    }

    std::uint64_t size() const
    {
        return fileSize;
    }

    void open(const char* path, boost::beast::file_mode mode,
              boost::system::error_code& ec)
    {
        fileHandle.open(path, mode, ec);
        fileSize = fileHandle.size(ec);
    }

    void setFd(int fd, boost::system::error_code& ec)
    {
        fileHandle.native_handle(fd);
        fileSize = fileHandle.size(ec);
    }
};

inline std::uint64_t FileBody::size(const value_type& body)
{
    return body.size();
}

class FileBody::writer
{
  public:
    using const_buffers_type = boost::asio::const_buffer;

  private:
    std::string buf;
    crow::utility::Base64Encoder encoder;

    value_type& body;
    std::uint64_t remain;
    constexpr static size_t readBufSize = 4096;
    std::array<char, readBufSize> fileReadBuf{};

  public:
    template <bool IsRequest, class Fields>
    writer(boost::beast::http::header<IsRequest, Fields>& /*header*/,
           value_type& bodyIn) :
        body(bodyIn),
        remain(body.size())
    {}

    static void init(boost::beast::error_code& ec)
    {
        ec = {};
    }

    boost::optional<std::pair<const_buffers_type, bool>>
        get(boost::beast::error_code& ec)
    {
        size_t toRead = fileReadBuf.size();
        if (remain < toRead)
        {
            toRead = static_cast<size_t>(remain);
        }
        size_t read = body.file().read(fileReadBuf.data(), toRead, ec);
        if (read != toRead || ec)
        {
            return boost::none;
        }
        remain -= read;

        std::string_view chunkView(fileReadBuf.data(), read);
        bool more = remain > 0;
        if (body.encodingType == EncodingType::Base64)
        {
            buf.clear();
            buf.reserve(
                crow::utility::Base64Encoder::encodedSize(chunkView.size()));
            encoder.encode(chunkView, buf);
            if (!more)
            {
                encoder.finalize(buf);
            }
            return std::make_pair(const_buffers_type{buf.data(), buf.length()},
                                  more);
        }
        // Else RAW
        return std::make_pair(
            const_buffers_type(chunkView.data(), chunkView.size()), more);
    }
};
} // namespace bmcweb
