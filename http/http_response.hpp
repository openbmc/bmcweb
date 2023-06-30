#pragma once
#include "logging.hpp"
#include "utils/hex_utils.hpp"

#include <boost/beast/http/message.hpp>
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
    using response_type =
        boost::beast::http::response<boost::beast::http::string_body>;

    response_type stringResponse;

    nlohmann::json jsonValue;

    void addHeader(std::string_view key, std::string_view value)
    {
        stringResponse.insert(key, value);
    }

    void addHeader(boost::beast::http::field key, std::string_view value)
    {
        stringResponse.insert(key, value);
    }

    void clearHeader(boost::beast::http::field key)
    {
        stringResponse.erase(key);
    }

    Response() = default;

    Response(Response&& res) noexcept :
        stringResponse(std::move(res.stringResponse)),
        jsonValue(std::move(res.jsonValue)), completed(res.completed)
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
        r.stringResponse.clear();
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
        stringResponse.result(v);
    }

    void result(boost::beast::http::status v)
    {
        stringResponse.result(v);
    }

    boost::beast::http::status result() const
    {
        return stringResponse.result();
    }

    unsigned resultInt() const
    {
        return stringResponse.result_int();
    }

    std::string_view reason() const
    {
        return stringResponse.reason();
    }

    bool isCompleted() const noexcept
    {
        return completed;
    }

    std::string& body()
    {
        return stringResponse.body();
    }

    std::string_view getHeaderValue(std::string_view key) const
    {
        return stringResponse.base()[key];
    }

    void keepAlive(bool k)
    {
        stringResponse.keep_alive(k);
    }

    bool keepAlive() const
    {
        return stringResponse.keep_alive();
    }

    void preparePayload()
    {
        // This code is a throw-free equivalent to
        // beast::http::message::prepare_payload
        boost::optional<uint64_t> pSize = stringResponse.payload_size();
        using boost::beast::http::status;
        using boost::beast::http::status_class;
        using boost::beast::http::to_status_class;
        if (!pSize)
        {
            pSize = 0;
        }
        else
        {
            bool is1XXReturn = to_status_class(stringResponse.result()) ==
                               status_class::informational;
            if (*pSize > 0 &&
                (is1XXReturn || stringResponse.result() == status::no_content ||
                 stringResponse.result() == status::not_modified))
            {
                BMCWEB_LOG_CRITICAL(
                    "{} Response content provided but code was no-content or not_modified, which aren't allowed to have a body",
                    logPtr(this));
                pSize = 0;
                body().clear();
            }
        }
        stringResponse.content_length(*pSize);
    }

    void clear()
    {
        BMCWEB_LOG_DEBUG("{} Clearing response containers", logPtr(this));
        stringResponse.clear();
        stringResponse.body().shrink_to_fit();
        jsonValue = nullptr;
        completed = false;
        expectedHash = std::nullopt;
    }

    void write(std::string_view bodyPart)
    {
        stringResponse.body() += std::string(bodyPart);
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

  private:
    std::optional<std::string> expectedHash;
    bool completed = false;
    std::function<void(Response&)> completeRequestHandler;
    std::function<bool()> isAliveHelper;
};
} // namespace crow
