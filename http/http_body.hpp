// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "duplicatable_file_handle.hpp"
#include "logging.hpp"
#include "multipart_parser.hpp"
#include "utility.hpp"
#include "zstd_compressor.hpp"
#include "zstd_decompressor.hpp"

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
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

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

enum class CompressionType
{
    Raw,
    Gzip,
    Zstd,
};

struct FileBody
{
    std::optional<size_t> fileSize;
    DuplicatableFileHandle fileHandle;
};

struct MultiPartBody
{
    std::vector<FormPart> parts;
};

class HttpBody::value_type
{
    friend HttpBody::reader;
    friend HttpBody::writer;

    std::variant<std::string, FileBody, MultiPartBody> bodyData;

    std::span<const FormPart> getMimeFields() const
    {
        if (const auto* multiPartBody = std::get_if<MultiPartBody>(&bodyData))
        {
            return {multiPartBody->parts};
        }
        return {};
    }

    std::span<FormPart> getMimeFields()
    {
        if (auto* multiPartBody = std::get_if<MultiPartBody>(&bodyData))
        {
            return {multiPartBody->parts};
        }
        return {};
    }

  public:
    value_type() = default;
    explicit value_type(std::string_view s) : bodyData(std::string(s)) {}
    explicit value_type(EncodingType e) : encodingType(e) {}
    EncodingType encodingType = EncodingType::Raw;
    CompressionType compressionType = CompressionType::Raw;
    CompressionType clientCompressionType = CompressionType::Raw;

    ~value_type() = default;

    explicit value_type(EncodingType enc, CompressionType comp) :
        encodingType(enc), compressionType(comp)
    {}

    value_type(const value_type& other) noexcept = default;
    value_type& operator=(const value_type& other) noexcept = default;
    value_type(value_type&& other) noexcept = default;
    value_type& operator=(value_type&& other) noexcept = default;

    const boost::beast::file_posix& file() const
    {
        if (const auto* fileBody = std::get_if<FileBody>(&bodyData))
        {
            return fileBody->fileHandle.fileHandle;
        }
        static boost::beast::file_posix emptyFile;
        return emptyFile;
    }

    std::string& str()
    {
        if (auto* s = std::get_if<std::string>(&bodyData))
        {
            return *s;
        }
        return bodyData.emplace<std::string>();
    }

    const std::string& str() const
    {
        if (const auto* s = std::get_if<std::string>(&bodyData))
        {
            return *s;
        }
        static const std::string emptyString;
        return emptyString;
    }

    std::span<FormPart> multipart()
    {
        if (auto* multiPartBody = std::get_if<MultiPartBody>(&bodyData))
        {
            return {multiPartBody->parts};
        }
        return {};
    }

    std::optional<size_t> payloadSize() const
    {
        if (const auto* s = std::get_if<std::string>(&bodyData))
        {
            return s->size();
        }
        if (const auto* fileBody = std::get_if<FileBody>(&bodyData))
        {
            if (fileBody->fileHandle.fileHandle.is_open() && fileBody->fileSize)
            {
                if (encodingType == EncodingType::Base64)
                {
                    return crow::utility::Base64Encoder::encodedSize(
                        *fileBody->fileSize);
                }
            }
            return fileBody->fileSize;
        }
        return std::nullopt;
    }

    void clear()
    {
        bodyData = std::string{};
        encodingType = EncodingType::Raw;
    }

    void open(const char* path, boost::beast::file_mode mode,
              boost::system::error_code& ec)
    {
        FileBody fileBody;
        fileBody.fileHandle.fileHandle.open(path, mode, ec);
        if (ec)
        {
            return;
        }
        boost::system::error_code ec2;
        uint64_t size = fileBody.fileHandle.fileHandle.size(ec2);
        if (!ec2)
        {
            BMCWEB_LOG_INFO("File size was {} bytes", size);
            fileBody.fileSize = static_cast<size_t>(size);
        }
        else
        {
            BMCWEB_LOG_WARNING("Failed to read file size on {}", path);
        }

        int fadvise =
            posix_fadvise(fileBody.fileHandle.fileHandle.native_handle(), 0, 0,
                          POSIX_FADV_SEQUENTIAL);
        if (fadvise != 0)
        {
            BMCWEB_LOG_WARNING("Fadvise returned {} ignoring", fadvise);
        }
        bodyData = std::move(fileBody);
        ec = {};
    }

    void setFd(int fd, boost::system::error_code& ec)
    {
        FileBody& fileBody = bodyData.emplace<FileBody>();
        fileBody.fileHandle.fileHandle.native_handle(fd);

        boost::system::error_code ec2;
        uint64_t size = fileBody.fileHandle.fileHandle.size(ec2);
        if (!ec2)
        {
            if (size != 0 && size < std::numeric_limits<size_t>::max())
            {
                fileBody.fileSize = static_cast<size_t>(size);
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

    std::optional<ZstdDecompressor> zstdDecompressor;
    std::optional<ZstdCompressor> zstdCompressor;

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
    {
        // If zstd compressed and client doesn't support zstd, need to
        // decompress
        if (body.compressionType == CompressionType::Zstd &&
            body.clientCompressionType != CompressionType::Zstd)
        {
            zstdDecompressor.emplace();
        }
        if (body.compressionType == CompressionType::Raw &&
            body.clientCompressionType == CompressionType::Zstd)
        {
            std::optional<size_t> size = body.payloadSize();
            if (size)
            {
                BMCWEB_LOG_DEBUG(
                    "Body is raw, client supports zstd, and paylod is not streaming.  Compressing.");
                zstdCompressor.emplace();
                if (!zstdCompressor->init(*size))
                {
                    BMCWEB_LOG_ERROR("Failed to initialize Zstd Compressor");
                    zstdCompressor = std::nullopt;
                }
            }
        }
    }

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
        }
        else
        {
            size_t readReq = std::min(fileReadBuf.size(), maxSize);
            BMCWEB_LOG_INFO("Reading {}", readReq);
            boost::system::error_code readEc;
            size_t read = body.file().read(fileReadBuf.data(), readReq, readEc);
            if (readEc)
            {
                if (readEc != boost::system::errc::operation_would_block &&
                    readEc !=
                        boost::system::errc::resource_unavailable_try_again)
                {
                    BMCWEB_LOG_CRITICAL("Failed to read from file {}",
                                        readEc.message());
                    ec = readEc;
                    return boost::none;
                }
            }

            std::string_view chunkView(fileReadBuf.data(), read);
            BMCWEB_LOG_INFO("Read {} bytes from file", read);
            // If the number of bytes read equals the amount requested, we
            // haven't reached EOF yet
            ret.second = read == readReq;
            if (body.encodingType == EncodingType::Base64)
            {
                buf.clear();
                buf.reserve(crow::utility::Base64Encoder::encodedSize(
                    chunkView.size()));
                encoder.encode(chunkView, buf);
                if (!ret.second)
                {
                    encoder.finalize(buf);
                }
                ret.first = const_buffers_type(buf.data(), buf.size());
            }
            else
            {
                ret.first =
                    const_buffers_type(chunkView.data(), chunkView.size());
            }
        }

        if (zstdDecompressor)
        {
            std::optional<const_buffers_type> decompressed =
                zstdDecompressor->decompress(ret.first);
            if (!decompressed)
            {
                return boost::none;
            }
            ret.first = *decompressed;
        }
        if (zstdCompressor)
        {
            BMCWEB_LOG_DEBUG("Compressing body more={}", ret.second);
            std::span<const uint8_t> spanIn(
                static_cast<const uint8_t*>(ret.first.data()),
                ret.first.size());
            std::optional<std::span<const uint8_t>> compressed =
                zstdCompressor->compress(spanIn, ret.second);
            if (!compressed)
            {
                return boost::none;
            }
            ret.first = *compressed;
        }
        BMCWEB_LOG_INFO("Returning {} bytes more={}", ret.first.size(),
                        ret.second);
        return ret;
    }
};

class HttpBody::reader
{
    value_type& value;
    std::optional<MultipartParser> multipartParser;
    const boost::beast::http::fields& hdr;

  public:
    template <bool IsRequest, class Fields>
    reader(boost::beast::http::header<IsRequest, Fields>& headers,
           value_type& body) : value(body), hdr(headers)
    {}

    void init(const boost::optional<std::uint64_t>& contentLength,
              boost::beast::error_code& ec)
    {
        std::string_view contentType =
            hdr[boost::beast::http::field::content_type];

        if (contentType.starts_with("multipart/form-data"))
        {
            BMCWEB_LOG_DEBUG("Processing multipart/form-data");
            MultipartParser& mp = multipartParser.emplace();
            ParserError state = mp.start(contentType);
            if (state != ParserError::PARSER_SUCCESS)
            {
                BMCWEB_LOG_ERROR("Failed to parse content-type: {}",
                                 contentType);
                ec = {boost::system::errc::invalid_argument,
                      boost::system::generic_category()};
                return;
            }

            ec = {};
        }

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
            if (multipartParser)
            {
                std::string_view buf(ptr, b.size());
                ParserError state = multipartParser->parsePart(buf);
                if (state != ParserError::PARSER_SUCCESS)
                {
                    BMCWEB_LOG_ERROR("Failed to parse part: {}",
                                     static_cast<int>(state));
                    ec = {boost::system::errc::invalid_argument,
                          boost::system::generic_category()};
                    return 0;
                }
            }
        }
        ec = {};
        return extra;
    }

    void finish(boost::beast::error_code& ec)
    {
        if (multipartParser)
        {
            ParserError state = multipartParser->finish();
            if (state != ParserError::PARSER_SUCCESS)
            {
                BMCWEB_LOG_ERROR("Failed to finish multipart parser: {}",
                                 static_cast<int>(state));
                ec = {boost::system::errc::invalid_argument,
                      boost::system::generic_category()};
                return;
            }
            value.bodyData =
                MultiPartBody{std::move(multipartParser->mime_fields)};
        }
        ec = {};
    }
};

inline std::uint64_t HttpBody::size(const value_type& body)
{
    std::optional<size_t> payloadSize = body.payloadSize();
    return payloadSize.value_or(0U);
}

} // namespace bmcweb
