#pragma once
#include "http_request.h"
#include "logging.h"

#include "nlohmann/json.hpp"

#include <boost/beast/http.hpp>

#include <string>

namespace crow
{

template <typename Adaptor, typename Handler>
class Connection;

struct Response
{
    template <typename Adaptor, typename Handler>
    friend class crow::Connection;

    boost::beast::http::header<false> headers;

    std::variant<boost::beast::http::response<boost::beast::http::string_body>,
                 boost::beast::http::response<boost::beast::http::file_body>,
                 boost::beast::http::response<boost::beast::http::buffer_body>>
        response;

    nlohmann::json jsonValue;

    void addHeader(const std::string_view key, const std::string_view value)
    {

        headers.set(key, value);
    }

    void addHeader(boost::beast::http::field key, std::string_view value)
    {
        headers.set(key, value);
    }

    Response()
    {}

    ~Response()
    {
        BMCWEB_LOG_DEBUG << this << " Destroying response";
    }

    void result(boost::beast::http::status v)
    {
        headers.result(v);
    }

    boost::beast::http::status result()
    {
        return headers.result();
    }

    unsigned resultInt()
    {
        return headers.result_int();
    }

    std::string_view reason()
    {
        return headers.reason();
    }

    bool isCompleted() const noexcept
    {
        return completed;
    }

    std::string& body()
    {
        auto* strRes = std::get_if<
            boost::beast::http::response<boost::beast::http::string_body>>(
            &response);
        if (strRes == nullptr)
        {

            auto& res = response.emplace<boost::beast::http::response<
                boost::beast::http::string_body>>();
            return res.body();
        }
        return strRes->body();
    }

    void keepAlive(bool k)
    {
        keepAliveValue = k;
    }

    bool keepAlive()
    {
        return keepAliveValue;
    }

    void preparePayload()
    {
        std::visit(
            [this](auto& val) {
                val.base() = std::move(headers);
                val.keep_alive(keepAliveValue);
                val.prepare_payload();
            },
            response);
    }

    void clear()
    {
        BMCWEB_LOG_DEBUG << this << " Clearing response containers";
        completed = false;
        keepAliveValue = false;
        response.emplace<
            boost::beast::http::response<boost::beast::http::string_body>>();
        jsonValue.clear();
    }

    void end()
    {
        if (completed)
        {
            BMCWEB_LOG_ERROR << "Response was ended twice";
            return;
        }
        completed = true;
        if (completeRequestHandler)
        {
            BMCWEB_LOG_DEBUG << "calling completion handler";
            BMCWEB_LOG_DEBUG << "completion handler was valid";
            completeRequestHandler();
        }
    }

    bool isAlive()
    {
        return isAliveHelper && isAliveHelper();
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
        auto& fileResponse = response.emplace<
            boost::beast::http::response<boost::beast::http::file_body>>();
        fileResponse.body() = std::move(file);
        return true;
    }

  private:
    bool completed{};
    bool keepAliveValue{};
    std::function<void()> completeRequestHandler;
    std::function<bool(const std::string_view)> writeHandler;
    std::function<bool()> isAliveHelper;
}; // namespace crow
} // namespace crow
