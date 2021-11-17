#pragma once
#include "logging.hpp"
#include "nlohmann/json.hpp"

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

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

    std::optional<response_type> stringResponse;

    nlohmann::json jsonValue;

    void addHeader(const std::string_view key, const std::string_view value)
    {
        stringResponse->set(key, value);
    }

    void addHeader(boost::beast::http::field key, std::string_view value)
    {
        stringResponse->set(key, value);
    }

    Response() : stringResponse(response_type{})
    {}

    Response(Response&& res) = delete;

    Response& operator=(const Response& r) = delete;

    Response& operator=(Response&& r) = default;

    void result(boost::beast::http::status v)
    {
        stringResponse->result(v);
    }

    boost::beast::http::status result()
    {
        return stringResponse->result();
    }

    unsigned resultInt()
    {
        return stringResponse->result_int();
    }

    std::string_view reason()
    {
        return stringResponse->reason();
    }

    bool isCompleted() const noexcept
    {
        return completed;
    }

    std::string& body()
    {
        return stringResponse->body();
    }

    void keepAlive(bool k)
    {
        stringResponse->keep_alive(k);
    }

    bool keepAlive()
    {
        return stringResponse->keep_alive();
    }

    void preparePayload()
    {
        stringResponse->prepare_payload();
    }

    void clear()
    {
        BMCWEB_LOG_DEBUG << this << " Clearing response containers";
        stringResponse.emplace(response_type{});
        jsonValue.clear();
        completed = false;
    }

    void write(std::string_view bodyPart)
    {
        stringResponse->body() += std::string(bodyPart);
    }

    void end()
    {
        if (completed)
        {
            BMCWEB_LOG_ERROR << this << " Response was ended twice";
            return;
        }
        completed = true;
        BMCWEB_LOG_DEBUG << this << " calling completion handler";
        if (completeRequestHandler)
        {
            BMCWEB_LOG_DEBUG << this << " completion handler was valid";
            completeRequestHandler(*this);
        }
    }

    bool isAlive()
    {
        return isAliveHelper && isAliveHelper();
    }

    void setCompleteRequestHandler(std::function<void(Response&)>&& handler)
    {
        BMCWEB_LOG_DEBUG << this << " setting completion handler";
        completeRequestHandler = std::move(handler);
    }

    std::function<void(Response&)> releaseCompleteRequestHandler()
    {
        BMCWEB_LOG_DEBUG << this << " releasing completion handler"
                         << static_cast<bool>(completeRequestHandler);
        std::function<void(Response&)> ret = completeRequestHandler;
        completeRequestHandler = nullptr;
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

  private:
    bool completed = false;
    std::function<void(Response&)> completeRequestHandler;
    std::function<bool()> isAliveHelper;

    // In case of a JSON object, set the Content-Type header
    void jsonMode()
    {
        addHeader("Content-Type", "application/json");
    }
};
} // namespace crow
