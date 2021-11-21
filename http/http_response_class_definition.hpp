#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <optional>
#include <nlohmann/json.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

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

    void addHeader(const std::string_view key, const std::string_view value);

    void addHeader(boost::beast::http::field key, std::string_view value);

    Response();

    Response& operator=(const Response& r) = delete;

    Response& operator=(Response&& r) noexcept;

    void result(boost::beast::http::status v);

    boost::beast::http::status result();

    unsigned resultInt();

    std::string_view reason();

    bool isCompleted() const noexcept;

    std::string& body();

    void keepAlive(bool k);

    bool keepAlive();

    void preparePayload();

    void clear();

    void write(std::string_view bodyPart);

    void end();

    void end(std::string_view bodyPart);

    bool isAlive();

    void setCompleteRequestHandler(std::function<void()> newHandler);

  private:
    bool completed{};
    std::function<void()> completeRequestHandler;
    std::function<bool()> isAliveHelper;

    // In case of a JSON object, set the Content-Type header
    void jsonMode();
};
} // namespace crow
