#pragma once
#include "logging.hpp"
#include "utils/hex_utils.hpp"

#include <boost/beast/http/basic_dynamic_body.hpp>
#include <boost/beast/http/buffer_body.hpp>
#include <boost/beast/http/file_body.hpp>
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
    using string_body_response_type =
        boost::beast::http::response<boost::beast::http::string_body>;
    using file_body_response_type =
        boost::beast::http::response<boost::beast::http::file_body>;

    using response_type =
        std::variant<string_body_response_type, file_body_response_type>;

    std::optional<response_type> genericResponse;

    nlohmann::json jsonValue;

    void addHeader(const std::string_view key, const std::string_view value)
    {
        std::visit([key, value](auto&& res) { res.set(key, value); },
                   genericResponse.value());
    }

    void addHeader(boost::beast::http::field key, std::string_view value)
    {
        std::visit([key, value](auto&& res) { res.set(key, value); },
                   genericResponse.value());
    }
    void clearHeader(boost::beast::http::field key)
    {
        std::visit([key](auto&& res) { res.erase(key); },
                   genericResponse.value());
    }

    Response() : genericResponse(string_body_response_type{}) {}

    Response(Response&& res) noexcept :
        genericResponse(std::move(res.genericResponse)),
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
        // genericResponse = std::move(r.genericResponse);
        // r.genericResponse.emplace(response_type{});
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
        std::visit([v](auto&& res) { res.result(v); }, genericResponse.value());
    }

    void result(boost::beast::http::status v)
    {
        std::visit([v](auto&& res) { res.result(v); }, genericResponse.value());
    }

    boost::beast::http::status result() const
    {
        return std::visit([](auto&& res) { return res.result(); },
                          genericResponse.value());
    }

    unsigned resultInt() const
    {
        return std::visit([](auto&& res) { return res.result_int(); },
                          genericResponse.value());
    }

    std::string_view reason() const
    {
        return std::visit([](auto&& res) { return res.reason(); },
                          genericResponse.value());
    }

    bool isCompleted() const noexcept
    {
        return completed;
    }
    boost::beast::http::fields fields()
    {
        return std::visit([](auto&& res) { return res.base(); },
                          genericResponse.value());
    }
    std::string& body()
    {
        if (!std::holds_alternative<string_body_response_type>(
                genericResponse.value()))
        {
            string_body_response_type stringbody{};
            std::visit([&stringbody](
                           auto&& other) { stringbody.base() = other.base(); },
                       genericResponse.value());

            genericResponse.emplace(stringbody);
        }
        return std::get<string_body_response_type>(*genericResponse).body();
    }

    std::string_view getHeaderValue(std::string_view key) const
    {
        return std::visit([key](auto&& res) { return res.base()[key]; },
                          genericResponse.value());
    }

    void keepAlive(bool k)
    {
        return std::visit([k](auto&& res) { res.keep_alive(k); },
                          genericResponse.value());
    }

    bool keepAlive() const
    {
        return std::visit([](auto&& res) { return res.keep_alive(); },
                          genericResponse.value());
    }

    void preparePayload()
    {
        return std::visit([](auto&& res) { return res.prepare_payload(); },
                          genericResponse.value());
    }

    void clear()
    {
        BMCWEB_LOG_DEBUG("{} Clearing response containers", logPtr(this));

        genericResponse.emplace(response_type{});
        jsonValue.clear();
        completed = false;
        expectedHash = std::nullopt;
    }

    void write(std::string_view bodyPart)
    {
        body() += std::string(bodyPart);
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
