#pragma once
#include "nlohmann/json.hpp"

#include <boost/beast/http.hpp>
#include <string>

#include "http_request.h"
#include "logging.h"

namespace crow
{

template <typename Adaptor, typename Handler, typename... Middlewares>
class Connection;

struct Response
{
    template <typename Adaptor, typename Handler, typename... Middlewares>
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
    {
    }

    Response(Response&& r)
    {
        BMCWEB_LOG_DEBUG << "Moving response containers";
        *this = std::move(r);
    }

    ~Response()
    {
        BMCWEB_LOG_DEBUG << this << " Destroying response";
    }

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
    };

    void clear()
    {
        BMCWEB_LOG_DEBUG << this << " Clearing response containers";
        stringResponse.emplace(response_type{});
        jsonValue.clear();
        completed = false;
    }

    void write(std::string_view body_part)
    {
        stringResponse->body() += std::string(body_part);
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

    void end(std::string_view body_part)
    {
        write(body_part);
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
} // namespace crow
