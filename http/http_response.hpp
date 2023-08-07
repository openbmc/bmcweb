#pragma once
#include "logging.hpp"
#include "utils/hex_utils.hpp"

#include <boost/beast/http/file_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/variant2/variant.hpp>
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

    // Use boost variant2 so that we don't have to worry about throwing
    // valueless_by_exception
    boost::variant2::variant<string_response, file_response> stringResponse;

    nlohmann::json jsonValue;

    boost::beast::http::header<false, boost::beast::http::fields>& fields()
    {
        return fieldsRef;
    }

    const boost::beast::http::header<false, boost::beast::http::fields>&
        fields() const
    {
        return fieldsRef;
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

    Response() : fieldsRef(stringResponse.emplace<string_response>().base()) {}

    Response(Response&& res) noexcept :
        stringResponse(std::move(res.stringResponse)),
        jsonValue(std::move(res.jsonValue)), completed(res.completed),
        fieldsRef(
            boost::variant2::get_if<string_response>(&stringResponse)->base())
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
        stringResponse = std::move(r.stringResponse);
        fieldsRef =
            boost::variant2::get_if<string_response>(&stringResponse)->base();
        jsonValue = std::move(r.jsonValue);
        r.fieldsRef = r.stringResponse.emplace<string_response>().base();

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
            boost::variant2::get_if<string_response>(&(res.stringResponse));
        if (s == nullptr)
        {
            BMCWEB_LOG_ERROR("Unable to copy a file");
            return;
        }
        string_response* myString =
            boost::variant2::get_if<string_response>(&stringResponse);
        if (myString == nullptr)
        {
            myString = &stringResponse.emplace<string_response>();
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
        string_response* body =
            boost::variant2::get_if<string_response>(&stringResponse);
        if (body == nullptr)
        {
            return nullptr;
        }
        return &body->body();
    }

    boost::optional<uint64_t> payloadSize()
    {
        return boost::variant2::visit(
            [](auto&& res) { return res.payload_size(); }, stringResponse);
    }

    std::string_view getHeaderValue(std::string_view key) const
    {
        return fields()[key];
    }

    void keepAlive(bool k)
    {
        boost::variant2::visit([k](auto&& res) { res.keep_alive(k); },
                               stringResponse);
    }

    bool keepAlive() const
    {
        return boost::variant2::visit(
            [](auto& res) { return res.keep_alive(); }, stringResponse);
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
        string_response* str =
            boost::variant2::get_if<string_response>(&stringResponse);
        if (str != nullptr)
        {
            str->content_length(getContentLength(str->payload_size()));
        }
        file_response* file =
            boost::variant2::get_if<file_response>(&stringResponse);
        if (file != nullptr)
        {
            file->content_length(getContentLength(file->payload_size()));
        }
    }

    void clear()
    {
        BMCWEB_LOG_DEBUG("{} Clearing response containers", logPtr(this));
        stringResponse.emplace<string_response>();
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
        string_response* str =
            boost::variant2::get_if<string_response>(&stringResponse);
        if (str != nullptr)
        {
            str->body() = std::move(bodyPart);
            return;
        }
        string_response& str2 = stringResponse.emplace<string_response>(
            result(), 11, std::move(bodyPart));
        fieldsRef = str2.base();
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
        boost::beast::http::header<false> headTemp = fields();
        file_response& fileResponse =
            stringResponse.emplace<file_response>(std::move(headTemp));
        fieldsRef = fileResponse.base();
        fileResponse.body() = std::move(file);
        return true;
    }

  private:
    std::optional<std::string> expectedHash;
    bool completed = false;
    std::function<void(Response&)> completeRequestHandler;
    std::function<bool()> isAliveHelper;
    std::reference_wrapper<
        boost::beast::http::header<false, boost::beast::http::fields>>
        fieldsRef;
};
} // namespace crow
