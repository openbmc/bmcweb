#pragma once
#include "http_file_body.hpp"
#include "logging.hpp"
#include "utils/hex_utils.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/beast/http/basic_dynamic_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/message_generator.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/variant2/variant.hpp>
#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <utility>
namespace crow
{

template <typename Adaptor, typename Handler>
class Connection;

namespace http = boost::beast::http;

struct Response
{
    template <typename Adaptor, typename Handler>
    friend class crow::Connection;

    using string_response = http::response<http::string_body>;
    using file_response = http::response<bmcweb::FileBody>;

    // Use boost variant2 because it doesn't have valueless by exception
    boost::variant2::variant<string_response, file_response> response;

    nlohmann::json jsonValue;
    using fields_type = http::header<false, http::fields>;
    fields_type& fields()
    {
        return boost::variant2::visit(
            [](auto&& r) -> fields_type& { return r.base(); }, response);
    }

    const fields_type& fields() const
    {
        return boost::variant2::visit(
            [](auto&& r) -> const fields_type& { return r.base(); }, response);
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

    Response() : response(string_response()) {}
    Response(Response&& res) noexcept :
        response(std::move(res.response)), jsonValue(std::move(res.jsonValue)),
        completed(res.completed)
    {
        // See note in operator= move handler for why this is needed.
        if (!res.completed)
        {
            completeRequestHandler = std::move(res.completeRequestHandler);
            res.completeRequestHandler = nullptr;
        }
        isAliveHelper = res.isAliveHelper;
        res.isAliveHelper = nullptr;
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
        isAliveHelper = std::move(r.isAliveHelper);
        r.isAliveHelper = nullptr;
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
        const string_response* s =
            boost::variant2::get_if<string_response>(&(res.response));
        if (s == nullptr)
        {
            BMCWEB_LOG_ERROR("Unable to copy a file");
            return;
        }
        string_response* myString =
            boost::variant2::get_if<string_response>(&response);
        if (myString == nullptr)
        {
            myString = &response.emplace<string_response>();
        }
        myString->body() = s->body();
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
        string_response* body =
            boost::variant2::get_if<string_response>(&response);
        if (body == nullptr)
        {
            return nullptr;
        }
        return &body->body();
    }

    std::string_view getHeaderValue(std::string_view key) const
    {
        return fields()[key];
    }

    void keepAlive(bool k)
    {
        return boost::variant2::visit([k](auto&& r) { r.keep_alive(k); },
                                      response);
    }

    bool keepAlive() const
    {
        return boost::variant2::visit([](auto&& r) { return r.keep_alive(); },
                                      response);
    }

    uint64_t getContentLength(boost::optional<uint64_t> pSize)
    {
        // This code is a throw-free equivalent to
        // beast::http::message::prepare_payload
        using http::status;
        using http::status_class;
        using http::to_status_class;
        if (!pSize)
        {
            return 0;
        }
        bool is1XXReturn = to_status_class(result()) ==
                           status_class::informational;
        if (*pSize > 0 && (is1XXReturn || result() == status::no_content ||
                           result() == status::not_modified))
        {
            BMCWEB_LOG_CRITICAL("{} Response content provided but code was "
                                "no-content or not_modified, which aren't "
                                "allowed to have a body",
                                logPtr(this));
            return 0;
        }
        return *pSize;
    }

    uint64_t size()
    {
        return boost::variant2::visit(
            [](auto&& res) -> uint64_t { return res.body().size(); }, response);
    }

    void preparePayload()
    {
        boost::variant2::visit(
            [this](auto&& r) {
            r.content_length(getContentLength(r.payload_size()));
        },
            response);
    }

    void clear()
    {
        BMCWEB_LOG_DEBUG("{} Clearing response containers", logPtr(this));
        response.emplace<string_response>();
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
        string_response* str =
            boost::variant2::get_if<string_response>(&response);
        if (str != nullptr)
        {
            str->body() += bodyPart;
            return;
        }
        http::header<false> headTemp = std::move(fields());
        string_response& stringResponse =
            response.emplace<string_response>(std::move(headTemp));
        stringResponse.body() = std::move(bodyPart);
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

    bool isAlive() const
    {
        return isAliveHelper && isAliveHelper();
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

    void setIsAliveHelper(std::function<bool()>&& handler)
    {
        isAliveHelper = std::move(handler);
    }

    std::function<bool()> releaseIsAliveHelper()
    {
        std::function<bool()> ret = std::move(isAliveHelper);
        isAliveHelper = nullptr;
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

    using message_generator = http::message_generator;
    message_generator generator()
    {
        return boost::variant2::visit(
            [](auto& r) -> message_generator { return std::move(r); },
            response);
    }

    bool openFile(const std::filesystem::path& path,
                  bmcweb::EncodingType enc = bmcweb::EncodingType::Raw)
    {
        file_response::body_type::value_type body(enc);
        boost::beast::error_code ec;
        body.open(path.c_str(), boost::beast::file_mode::read, ec);
        if (ec)
        {
            return false;
        }
        updateFileBody(std::move(body));
        return true;
    }

    bool openFd(int fd, bmcweb::EncodingType enc = bmcweb::EncodingType::Raw)
    {
        file_response::body_type::value_type body(enc);
        boost::beast::error_code ec;
        body.setFd(fd, ec);
        if (ec)
        {
            BMCWEB_LOG_ERROR("Failed to set fd");
            return false;
        }
        updateFileBody(std::move(body));
        return true;
    }

  private:
    void updateFileBody(file_response::body_type::value_type file)
    {
        // store the headers on stack temporarily so we can reconstruct the new
        // base with the old headers copied in.
        http::header<false> headTemp = std::move(fields());
        file_response& fileResponse =
            response.emplace<file_response>(std::move(headTemp));
        fileResponse.body() = std::move(file);
    }

    std::optional<std::string> expectedHash;
    bool completed = false;
    std::function<void(Response&)> completeRequestHandler;
    std::function<bool()> isAliveHelper;
};

struct DynamicResponse
{
    using response_type = boost::beast::http::response<
        boost::beast::http::basic_dynamic_body<boost::beast::flat_static_buffer<
            static_cast<std::size_t>(1024 * 1024)>>>;

    std::optional<response_type> bufferResponse;

    void addHeader(const std::string_view key, const std::string_view value)
    {
        bufferResponse->set(key, value);
    }

    void addHeader(boost::beast::http::field key, std::string_view value)
    {
        bufferResponse->set(key, value);
    }

    DynamicResponse() : bufferResponse(response_type{}) {}

    ~DynamicResponse() = default;

    DynamicResponse(const DynamicResponse&) = delete;

    DynamicResponse(DynamicResponse&&) = delete;

    DynamicResponse& operator=(const DynamicResponse& r) = delete;

    DynamicResponse& operator=(DynamicResponse&& r) noexcept
    {
        BMCWEB_LOG_DEBUG("Moving response containers");
        bufferResponse = std::move(r.bufferResponse);
        r.bufferResponse.emplace(response_type{});
        completed = r.completed;
        return *this;
    }

    void result(boost::beast::http::status v)
    {
        bufferResponse->result(v);
    }

    boost::beast::http::status result()
    {
        return bufferResponse->result();
    }

    unsigned resultInt()
    {
        return bufferResponse->result_int();
    }

    std::string_view reason()
    {
        return bufferResponse->reason();
    }

    bool isCompleted() const noexcept
    {
        return completed;
    }

    void keepAlive(bool k)
    {
        bufferResponse->keep_alive(k);
    }

    bool keepAlive()
    {
        return bufferResponse->keep_alive();
    }

    void preparePayload()
    {
        bufferResponse->prepare_payload();
    }

    void clear()
    {
        bufferResponse.emplace(response_type{});
        completed = false;
    }

    void end()
    {
        if (completed)
        {
            BMCWEB_LOG_DEBUG("Dynamic response was ended twice");
            return;
        }
        completed = true;
        BMCWEB_LOG_DEBUG("calling completion handler");
        if (completeRequestHandler)
        {
            BMCWEB_LOG_DEBUG("completion handler was valid");
            completeRequestHandler();
        }
    }

    bool isAlive()
    {
        return isAliveHelper && isAliveHelper();
    }
    std::function<void()> completeRequestHandler;

  private:
    bool completed{};
    std::function<bool()> isAliveHelper;
};

} // namespace crow
