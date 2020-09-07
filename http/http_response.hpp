#pragma once
#include "http_request.hpp"
#include "logging.hpp"
#include "nlohmann/json.hpp"

#include <boost/beast/http/buffer_body.hpp>
#include <boost/beast/http/file_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

#include <string>

namespace crow
{

template <typename Adaptor, typename Handler>
class Connection;

struct Response
{
    template <typename Adaptor, typename Handler>
    friend class crow::Connection;

    std::variant<boost::beast::http::response<boost::beast::http::string_body>,
                 boost::beast::http::response<boost::beast::http::file_body>,
                 boost::beast::http::response<boost::beast::http::buffer_body>>
        response;

    boost::beast::http::header<false>& headers;

    nlohmann::json jsonValue;

    void addHeader(const std::string_view key, const std::string_view value)
    {

        headers.set(key, value);
    }

    void addHeader(boost::beast::http::field key, std::string_view value)
    {
        headers.set(key, value);
    }

    Response() :
        headers(
            std::get_if<
                boost::beast::http::response<boost::beast::http::string_body>>(
                &response)
                ->base())
    {}

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
        boost::beast::http::response<boost::beast::http::string_body>* strRes =
            std::get_if<
                boost::beast::http::response<boost::beast::http::string_body>>(
                &response);
        if (strRes == nullptr)
        {

            boost::beast::http::response<boost::beast::http::string_body>& res =
                response.emplace<boost::beast::http::response<
                    boost::beast::http::string_body>>();
            headers = res.base();
            return res.body();
        }
        return strRes->body();
    }

    void keepAlive(bool k)
    {
        return std::visit([k](auto& val) { return val.keep_alive(k); },
                          response);
    }

    bool keepAlive()
    {
        return std::visit([](auto& val) { return val.keep_alive(); }, response);
    }

    void preparePayload()
    {
        std::visit(
            [this](auto& val) {
                val.base() = std::move(headers);
                val.prepare_payload();
            },
            response);
    }

    void clear()
    {
        BMCWEB_LOG_DEBUG << this << " Clearing response containers";
        completed = false;
        keepAliveValue = false;
        boost::beast::http::response<boost::beast::http::string_body>& res =
            response.emplace<boost::beast::http::response<
                boost::beast::http::string_body>>();
        headers = res.base();
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
        boost::beast::http::header headTemp = std::visit(
            [this](auto& val) { return std::move(val.base()); }, response);
        boost::beast::http::response<boost::beast::http::file_body>&
            fileResponse = response.emplace<
                boost::beast::http::response<boost::beast::http::file_body>>(
                std::move(headTemp));
        headers = fileResponse.base();
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
