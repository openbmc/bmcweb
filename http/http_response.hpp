// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include "http_body.hpp"
#include "logging.hpp"
#include "utils/hex_utils.hpp"

#include <fcntl.h>

#include <boost/beast/core/error.hpp>
#include <boost/beast/core/file_base.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace crow
{

template <typename Adaptor, typename Handler>
class Connection;

namespace http = boost::beast::http;

enum class OpenCode
{
    Success,
    FileDoesNotExist,
    InternalError,
};

struct Response
{
    template <typename Adaptor, typename Handler>
    friend class crow::Connection;

    http::response<bmcweb::HttpBody> response;

    nlohmann::json jsonValue;
    using fields_type = http::header<false, http::fields>;
    fields_type& fields()
    {
        return response.base();
    }

    const fields_type& fields() const
    {
        return response.base();
    }

    void addHeader(std::string_view key, std::string_view value)
    {
        fields().insert(key, value);
    }

    void addHeader(http::field key, std::string_view value)
    {
        fields().insert(key, value);
    }

    void clearHeader(http::field key)
    {
        fields().erase(key);
    }

    Response() = default;
    Response(Response&& res) noexcept :
        response(std::move(res.response)), jsonValue(std::move(res.jsonValue)),
        expectedHash(std::move(res.expectedHash)), completed(res.completed)
    {
        // See note in operator= move handler for why this is needed.
        if (!res.completed)
        {
            completeRequestHandler = std::move(res.completeRequestHandler);
            res.completeRequestHandler = nullptr;
        }
    }

    ~Response() = default;

    Response(const Response&) = delete;
    Response& operator=(const Response& r) = delete;

    Response& operator=(Response&& r) noexcept
    {
        BMCWEB_LOG_DEBUG("Moving response containers; this: {}; other: {}",
                         logPtr(this), logPtr(&r));
        if (this == &r)
        {
            return *this;
        }
        response = std::move(r.response);
        jsonValue = std::move(r.jsonValue);
        expectedHash = std::move(r.expectedHash);

        // Only need to move completion handler if not already completed
        // Note, there are cases where we might move out of a Response object
        // while in a completion handler for that response object.  This check
        // is intended to prevent destructing the functor we are currently
        // executing from in that case.
        if (!r.completed)
        {
            completeRequestHandler = std::move(r.completeRequestHandler);
            r.completeRequestHandler = nullptr;
        }
        else
        {
            completeRequestHandler = nullptr;
        }
        completed = r.completed;
        return *this;
    }

    void result(unsigned v)
    {
        fields().result(v);
    }

    void result(http::status v)
    {
        fields().result(v);
    }

    void copyBody(const Response& res)
    {
        response.body() = res.response.body();
    }

    http::status result() const
    {
        return fields().result();
    }

    unsigned resultInt() const
    {
        return fields().result_int();
    }

    std::string_view reason() const
    {
        return fields().reason();
    }

    bool isCompleted() const noexcept
    {
        return completed;
    }

    const std::string* body()
    {
        return &response.body().str();
    }

    std::string_view getHeaderValue(std::string_view key) const
    {
        return fields()[key];
    }

    std::string_view getHeaderValue(boost::beast::http::field key) const
    {
        return fields()[key];
    }

    void keepAlive(bool k)
    {
        response.keep_alive(k);
    }

    bool keepAlive() const
    {
        return response.keep_alive();
    }

    std::optional<uint64_t> size()
    {
        return response.body().payloadSize();
    }

    void preparePayload()
    {
        // This code is a throw-free equivalent to
        // beast::http::message::prepare_payload
        std::optional<uint64_t> pSize = response.body().payloadSize();

        using http::status;
        using http::status_class;
        using http::to_status_class;
        bool is1XXReturn = to_status_class(result()) ==
                           status_class::informational;
        if (!pSize)
        {
            response.chunked(true);
            return;
        }
        response.content_length(*pSize);

        if (is1XXReturn || result() == status::no_content ||
            result() == status::not_modified)
        {
            BMCWEB_LOG_CRITICAL("{} Response content provided but code was "
                                "no-content or not_modified, which aren't "
                                "allowed to have a body",
                                logPtr(this));
            response.content_length(0);
            return;
        }
    }

    void clear()
    {
        BMCWEB_LOG_DEBUG("{} Clearing response containers", logPtr(this));
        response.clear();
        response.body().clear();

        jsonValue = nullptr;
        completed = false;
        expectedHash = std::nullopt;
    }

    std::string computeEtag() const
    {
        // Only set etag if this request succeeded
        if (result() != http::status::ok)
        {
            return "";
        }
        // and the json response isn't empty
        if (jsonValue.empty())
        {
            return "";
        }
        size_t hashval = std::hash<nlohmann::json>{}(jsonValue);
        return "\"" + intToHexString(hashval, 8) + "\"";
    }

    void write(std::string&& bodyPart)
    {
        response.body().str() = std::move(bodyPart);
    }

    void end()
    {
        if (completed)
        {
            BMCWEB_LOG_ERROR("{} Response was ended twice", logPtr(this));
            return;
        }
        completed = true;
        BMCWEB_LOG_DEBUG("{} calling completion handler", logPtr(this));
        if (completeRequestHandler)
        {
            BMCWEB_LOG_DEBUG("{} completion handler was valid", logPtr(this));
            completeRequestHandler(*this);
        }
    }

    void setCompleteRequestHandler(std::function<void(Response&)>&& handler)
    {
        BMCWEB_LOG_DEBUG("{} setting completion handler", logPtr(this));
        completeRequestHandler = std::move(handler);

        // Now that we have a new completion handler attached, we're no longer
        // complete
        completed = false;
    }

    std::function<void(Response&)> releaseCompleteRequestHandler()
    {
        BMCWEB_LOG_DEBUG("{} releasing completion handler{}", logPtr(this),
                         static_cast<bool>(completeRequestHandler));
        std::function<void(Response&)> ret = completeRequestHandler;
        completeRequestHandler = nullptr;
        completed = true;
        return ret;
    }

    void setHashAndHandleNotModified()
    {
        // Can only hash if we have content that's valid
        if (jsonValue.empty() || result() != http::status::ok)
        {
            return;
        }
        size_t hashval = std::hash<nlohmann::json>{}(jsonValue);
        std::string hexVal = "\"" + intToHexString(hashval, 8) + "\"";
        addHeader(http::field::etag, hexVal);
        if (expectedHash && hexVal == *expectedHash)
        {
            jsonValue = nullptr;
            result(http::status::not_modified);
        }
    }

    void setExpectedHash(std::string_view hash)
    {
        expectedHash = hash;
    }

    OpenCode openFile(const std::filesystem::path& path,
                      bmcweb::EncodingType enc = bmcweb::EncodingType::Raw)
    {
        boost::beast::error_code ec;
        response.body().open(path.c_str(), boost::beast::file_mode::read, ec);
        response.body().encodingType = enc;
        if (ec)
        {
            BMCWEB_LOG_ERROR("Failed to open file {}, ec={}", path.c_str(),
                             ec.value());
            if (ec.value() == boost::system::errc::no_such_file_or_directory)
            {
                return OpenCode::FileDoesNotExist;
            }
            return OpenCode::InternalError;
        }
        return OpenCode::Success;
    }

    bool openFd(int fd, bmcweb::EncodingType enc = bmcweb::EncodingType::Raw)
    {
        boost::beast::error_code ec;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        int retval = fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
        if (retval == -1)
        {
            BMCWEB_LOG_ERROR("Setting O_NONBLOCK failed");
        }
        response.body().encodingType = enc;
        response.body().setFd(fd, ec);
        if (ec)
        {
            BMCWEB_LOG_ERROR("Failed to set fd");
            return false;
        }
        return true;
    }

  private:
    std::optional<std::string> expectedHash;
    bool completed = false;
    std::function<void(Response&)> completeRequestHandler;
};
} // namespace crow
