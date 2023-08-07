#pragma once
#include "logging.hpp"
#include "utils/hex_utils.hpp"

#include <boost/beast/http/file_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/message_generator.hpp>
#include <boost/beast/http/string_body.hpp>
#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace crow
{

template <typename Adaptor, typename Handler>
class Connection;

struct Response
{
    template <typename Adaptor, typename Handler>
    friend class crow::Connection;
    using string_response =
        boost::beast::http::response<boost::beast::http::string_body>;
    using file_response =
        boost::beast::http::response<boost::beast::http::file_body>;

    std::variant<string_response, file_response> response;

    nlohmann::json jsonValue;

    boost::beast::http::header<false, boost::beast::http::fields>& fields()
    {
        string_response* str = std::get_if<string_response>(&response);
        if (str != nullptr)
        {
            return str->base();
        }
        file_response* file = std::get_if<file_response>(&response);
        return file->base();
    }

    const boost::beast::http::header<false, boost::beast::http::fields>&
        fields() const
    {
        const string_response* str = std::get_if<string_response>(&response);
        if (str != nullptr)
        {
            return str->base();
        }
        const file_response* file = std::get_if<file_response>(&response);
        return file->base();
    }

    void addHeader(std::string_view key, std::string_view value)
    {
        fields().insert(key, value);
    }

    void addHeader(boost::beast::http::field key, std::string_view value)
    {
        fields().insert(key, value);
    }

    void clearHeader(boost::beast::http::field key)
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
        r.response.emplace<string_response>();

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

    void result(boost::beast::http::status v)
    {
        fields().result(v);
    }

    void copyBody(const Response& res)
    {
        const string_response* s =
            std::get_if<string_response>(&(res.response));
        if (s == nullptr)
        {
            BMCWEB_LOG_ERROR("Unable to copy a file");
            return;
        }
        string_response* myString = std::get_if<string_response>(&response);
        if (myString == nullptr)
        {
            myString = &response.emplace<string_response>();
        }
        myString->body() = s->body();
    }

    boost::beast::http::status result() const
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
        string_response* body = std::get_if<string_response>(&response);
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
        string_response* str = std::get_if<string_response>(&response);
        if (str != nullptr)
        {
            str->keep_alive(k);
        }
        file_response* file = std::get_if<file_response>(&response);
        if (file != nullptr)
        {
            file->keep_alive(k);
        }
    }

    bool keepAlive() const
    {
        const string_response* str = std::get_if<string_response>(&response);
        if (str != nullptr)
        {
            return str->keep_alive();
        }
        const file_response* file = std::get_if<file_response>(&response);
        return file->keep_alive();
    }

    uint64_t getContentLength(boost::optional<uint64_t> pSize)
    {
        // This code is a throw-free equivalent to
        // beast::http::message::prepare_payload
        using boost::beast::http::status;
        using boost::beast::http::status_class;
        using boost::beast::http::to_status_class;
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

    void preparePayload()
    {
        string_response* str = std::get_if<string_response>(&response);
        if (str != nullptr)
        {
            str->content_length(getContentLength(str->payload_size()));
        }
        file_response* file = std::get_if<file_response>(&response);
        if (file != nullptr)
        {
            file->content_length(getContentLength(file->payload_size()));
        }
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
        if (result() != boost::beast::http::status::ok)
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
        string_response* str = std::get_if<string_response>(&response);
        if (str != nullptr)
        {
            str->body() = std::move(bodyPart);
            return;
        }
        response.emplace<string_response>(result(), 11, std::move(bodyPart));
    }

    void end()
    {
        std::string etag = computeEtag();
        if (!etag.empty())
        {
            addHeader(boost::beast::http::field::etag, etag);
        }
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
        if (jsonValue.empty() || result() != boost::beast::http::status::ok)
        {
            return;
        }
        size_t hashval = std::hash<nlohmann::json>{}(jsonValue);
        std::string hexVal = "\"" + intToHexString(hashval, 8) + "\"";
        addHeader(boost::beast::http::field::etag, hexVal);
        if (expectedHash && hexVal == *expectedHash)
        {
            jsonValue = nullptr;
            result(boost::beast::http::status::not_modified);
        }
    }

    void setExpectedHash(std::string_view hash)
    {
        expectedHash = hash;
    }

    boost::beast::http::message_generator generator()
    {
        string_response* str = std::get_if<string_response>(&response);
        if (str != nullptr)
        {
            return boost::beast::http::message_generator(std::move(*str));
        }
        file_response* file = std::get_if<file_response>(&response);
        return boost::beast::http::message_generator(std::move(*file));
    }

    bool openFile(const std::filesystem::path& path)
    {
        boost::beast::http::file_body::value_type file;
        boost::beast::error_code ec;
        file.open(path.c_str(), boost::beast::file_mode::read, ec);
        if (ec)
        {
            return false;
        }
        // store the headers on stack temporarily so we can reconstruct the new
        // base with the old headers copied in.
        boost::beast::http::header<false> headTemp = std::move(fields());
        file_response& fileResponse =
            response.emplace<file_response>(std::move(headTemp));
        fileResponse.body() = std::move(file);
        return true;
    }

  private:
    std::optional<std::string> expectedHash;
    bool completed = false;
    std::function<void(Response&)> completeRequestHandler;
    std::function<bool()> isAliveHelper;
};
} // namespace crow
