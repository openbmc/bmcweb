#pragma once
#include "http_request.hpp"
#include "logging.hpp"
#include "nlohmann/json.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/beast/http/buffer_body.hpp>
#include <boost/beast/http/message.hpp>

#include <string>

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

    Response& operator=(const Response& r) = delete;

    Response& operator=(Response&& r) noexcept
    {
        BMCWEB_LOG_DEBUG << "Moving response containers";
        stringResponse = std::move(r.stringResponse);
        r.stringResponse.emplace(response_type{});
        jsonValue = std::move(r.jsonValue);
        completed = r.completed;
        return *this;
    }

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
            BMCWEB_LOG_ERROR << "Response was ended twice";
            return;
        }
        completed = true;
        BMCWEB_LOG_DEBUG << "calling completion handler";
        if (completeRequestHandler)
        {
            BMCWEB_LOG_DEBUG << "completion handler was valid";
            completeRequestHandler();
        }
    }

    void end(std::string_view bodyPart)
    {
        write(bodyPart);
        end();
    }

    bool isAlive()
    {
        return isAliveHelper && isAliveHelper();
    }

  private:
    bool completed{};
    std::function<void()> completeRequestHandler;
    std::function<bool()> isAliveHelper;

    // In case of a JSON object, set the Content-Type header
    void jsonMode()
    {
        addHeader("Content-Type", "application/json");
    }
};

struct DynamicResponse
{
    using response_type =
        boost::beast::http::response<boost::beast::http::buffer_body>;

    std::optional<response_type> bufferResponse;

    void addHeader(const std::string_view key, const std::string_view value)
    {
        bufferResponse->set(key, value);
    }

    void addHeader(boost::beast::http::field key, std::string_view value)
    {
        bufferResponse->set(key, value);
    }

    DynamicResponse() : bufferResponse(response_type{})
    {}

    DynamicResponse& operator=(const DynamicResponse& r) = delete;

    DynamicResponse& operator=(DynamicResponse&& r) noexcept
    {
        BMCWEB_LOG_DEBUG << "Moving response containers";
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
        BMCWEB_LOG_DEBUG << this << " Clearing response containers";
        bufferResponse.emplace(response_type{});
        completed = false;
    }

    void end()
    {
        if (completed)
        {
            BMCWEB_LOG_DEBUG << "Dynamic response was ended twice";
            return;
        }
        completed = true;
        BMCWEB_LOG_DEBUG << "calling completion handler";
        if (completeRequestHandler)
        {
            BMCWEB_LOG_DEBUG << "completion handler was valid";
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
