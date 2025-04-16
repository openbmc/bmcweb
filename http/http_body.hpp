// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "duplicatable_file_handle.hpp"
#include "logging.hpp"
#include "utility.hpp"

#include <fcntl.h>

#include <boost/asio/buffer.hpp>
#include <boost/beast/core/buffer_traits.hpp>
#include <boost/beast/core/buffers_range.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/file_base.hpp>
#include <boost/beast/core/file_posix.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/none.hpp>
#include <boost/optional/optional.hpp>
#include <boost/system/error_code.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace bmcweb
{
struct HttpBody
{
    // Body concept requires specific naming of classes
    // NOLINTBEGIN(readability-identifier-naming)
    class writer;
    class reader;
    class value_type;
    // NOLINTEND(readability-identifier-naming)

    static std::uint64_t size(const value_type& body);
};

enum class EncodingType
{
    Raw,
    Base64,
};

class HttpBody::value_type
{
    DuplicatableFileHandle fileHandle;
    std::optional<size_t> fileSize;
    std::string strBody;

  public:
    value_type() = default;
    explicit value_type(std::string_view s) : strBody(s) {}
    explicit value_type(EncodingType e) : encodingType(e) {}
    EncodingType encodingType = EncodingType::Raw;

    const boost::beast::file_posix& file() const
    {
        return fileHandle.fileHandle;
    }
    boost::beast::file_posix& file()
    {
        return fileHandle.fileHandle;
    }

    std::string& str()
    {
        return strBody;
    }

    const std::string& str() const
    {
        return strBody;
    }

    std::optional<size_t> payloadSize() const
    {
        if (!fileHandle.fileHandle.is_open())
        {
            return strBody.size();
        }
        if (fileSize)
        {
            if (encodingType == EncodingType::Base64)
            {
                return crow::utility::Base64Encoder::encodedSize(*fileSize);
            }
        }
        return fileSize;
    }

    void clear()
    {
        strBody.clear();
        strBody.shrink_to_fit();
        fileHandle.fileHandle = boost::beast::file_posix();
        fileSize = std::nullopt;
        encodingType = EncodingType::Raw;
    }

    void open(const char* path, boost::beast::file_mode mode,
              boost::system::error_code& ec)
    {
        fileHandle.fileHandle.open(path, mode, ec);
        if (ec)
        {
            return;
        }
        boost::system::error_code ec2;
        uint64_t size = fileHandle.fileHandle.size(ec2);
        if (!ec2)
        {
            BMCWEB_LOG_INFO("File size was {} bytes", size);
            fileSize = static_cast<size_t>(size);
        }
        else
        {
            BMCWEB_LOG_WARNING("Failed to read file size on {}", path);
        }

        int fadvise = posix_fadvise(fileHandle.fileHandle.native_handle(), 0, 0,
                                    POSIX_FADV_SEQUENTIAL);
        if (fadvise != 0)
        {
            BMCWEB_LOG_WARNING("Fasvise returned {} ignoring", fadvise);
        }
        ec = {};
    }

    void setFd(int fd, boost::system::error_code& ec)
    {
        fileHandle.fileHandle.native_handle(fd);

        boost::system::error_code ec2;
        uint64_t size = fileHandle.fileHandle.size(ec2);
        if (!ec2)
        {
            if (size != 0 && size < std::numeric_limits<size_t>::max())
            {
                fileSize = static_cast<size_t>(size);
            }
        }
        ec = {};
    }
};

class HttpBody::writer
{
  public:
    using const_buffers_type = boost::asio::const_buffer;

  private:
    std::string buf;
    crow::utility::Base64Encoder encoder;

    value_type& body;
    size_t sent = 0;
    // 64KB This number is arbitrary, and selected to try to optimize for larger
    // files and fewer loops over per-connection reduction in memory usage.
    // Nginx uses 16-32KB here, so we're in the range of what other webservers
    // do.
    constexpr static size_t readBufSize = 1024UL * 64UL;
    std::array<char, readBufSize> fileReadBuf{};

  public:
    template <bool IsRequest, class Fields>
    writer(boost::beast::http::header<IsRequest, Fields>& /*header*/,
           value_type& bodyIn) : body(bodyIn)
    {}

    static void init(boost::beast::error_code& ec)
    {
        ec = {};
    }

    boost::optional<std::pair<const_buffers_type, bool>> get(
        boost::beast::error_code& ec)
    {
        return getWithMaxSize(ec, std::numeric_limits<size_t>::max());
    }

    boost::optional<std::pair<const_buffers_type, bool>> getWithMaxSize(
        boost::beast::error_code& ec, size_t maxSize)
    {
        std::pair<const_buffers_type, bool> ret;
        if (!body.file().is_open())
        {
            size_t remain = body.str().size() - sent;
            size_t toReturn = std::min(maxSize, remain);
            ret.first = const_buffers_type(&body.str()[sent], toReturn);

            sent += toReturn;
            ret.second = sent < body.str().size();
            BMCWEB_LOG_INFO("Returning {} bytes more={}", ret.first.size(),
                            ret.second);
            return ret;
        }
        size_t readReq = std::min(fileReadBuf.size(), maxSize);
        BMCWEB_LOG_INFO("Reading {}", readReq);
        boost::system::error_code readEc;
        size_t read = body.file().read(fileReadBuf.data(), readReq, readEc);
        if (readEc)
        {
            if (readEc != boost::system::errc::operation_would_block &&
                readEc != boost::system::errc::resource_unavailable_try_again)
            {
                BMCWEB_LOG_CRITICAL("Failed to read from file {}",
                                    readEc.message());
                ec = readEc;
                return boost::none;
            }
        }

        std::string_view chunkView(fileReadBuf.data(), read);
        BMCWEB_LOG_INFO("Read {} bytes from file", read);
        // If the number of bytes read equals the amount requested, we haven't
        // reached EOF yet
        ret.second = read == readReq;
        if (body.encodingType == EncodingType::Base64)
        {
            buf.clear();
            buf.reserve(
                crow::utility::Base64Encoder::encodedSize(chunkView.size()));
            encoder.encode(chunkView, buf);
            if (!ret.second)
            {
                encoder.finalize(buf);
            }
            ret.first = const_buffers_type(buf.data(), buf.size());
        }
        else
        {
            ret.first = const_buffers_type(chunkView.data(), chunkView.size());
        }
        return ret;
    }
};

class HttpBody::reader
{
    value_type& value;

  public:
    template <bool IsRequest, class Fields>
    reader(boost::beast::http::header<IsRequest, Fields>& /*headers*/,
           value_type& body) : value(body)
    {}

    void init(const boost::optional<std::uint64_t>& contentLength,
              boost::beast::error_code& ec)
    {
        if (contentLength)
        {
            if (!value.file().is_open())
            {
                value.str().reserve(static_cast<size_t>(*contentLength));
            }
        }
        ec = {};
    }

    template <class ConstBufferSequence>
    std::size_t put(const ConstBufferSequence& buffers,
                    boost::system::error_code& ec)
    {
        size_t extra = boost::beast::buffer_bytes(buffers);
        for (const auto b : boost::beast::buffers_range_ref(buffers))
        {
            const char* ptr = static_cast<const char*>(b.data());
            if (value.file().is_open())
            {
                boost::system::error_code writeEc;
                value.file().write(ptr, b.size(), writeEc);
                if (writeEc)
                {
                    BMCWEB_LOG_ERROR("Failed to write to file {}",
                                     writeEc.message());
                    ec = writeEc;
                    return 0;
                }
            }
            else
            {
                value.str() += std::string_view(ptr, b.size());
            }
        }
        ec = {};
        return extra;
    }

    static void finish(boost::system::error_code& ec)
    {
        ec = {};
    }
};

inline std::uint64_t HttpBody::size(const value_type& body)
{
    std::optional<size_t> payloadSize = body.payloadSize();
    return payloadSize.value_or(0U);
}

} // namespace bmcweb
