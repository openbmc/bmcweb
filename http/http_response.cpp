#include "logging.hpp"
#include "http_response_class_decl.hpp"

namespace crow
{

Response::Response() : stringResponse(response_type{})
{}


void Response::addHeader(const std::string_view key, const std::string_view value)
{
    stringResponse->set(key, value);
}

void Response::addHeader(boost::beast::http::field key, std::string_view value)
{
    stringResponse->set(key, value);
}

Response& Response::operator=(Response&& r) noexcept
{
    BMCWEB_LOG_DEBUG << "Moving response containers";
    stringResponse = std::move(r.stringResponse);
    r.stringResponse.emplace(response_type{});
    jsonValue = std::move(r.jsonValue);
    completed = r.completed;
    return *this;
}

boost::beast::http::status Response::result()
{
    return stringResponse->result();
}

void Response::result(boost::beast::http::status v)
{
    stringResponse->result(v);
}

unsigned Response::resultInt()
{
    return stringResponse->result_int();
}

std::string_view Response::reason()
{
    return stringResponse->reason();
}

bool Response::isCompleted() const noexcept
{
    return completed;
}

std::string& Response::body()
{
    return stringResponse->body();
}

void Response::keepAlive(bool k)
{
    stringResponse->keep_alive(k);
}

bool Response::keepAlive()
{
    return stringResponse->keep_alive();
}

void Response::preparePayload()
{
    stringResponse->prepare_payload();
}

void Response::clear()
{
    BMCWEB_LOG_DEBUG << this << " Clearing response containers";
    stringResponse.emplace(response_type{});
    jsonValue.clear();
    completed = false;
}

void Response::write(std::string_view bodyPart)
{
    stringResponse->body() += std::string(bodyPart);
}

void Response::end()
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

void Response::end(std::string_view bodyPart)
{
    write(bodyPart);
    end();
}

bool Response::isAlive()
{
    return isAliveHelper && isAliveHelper();
}

void Response::setCompleteRequestHandler(std::function<void()> newHandler)
{
    completeRequestHandler = std::move(newHandler);
}

void Response::jsonMode()
{
    addHeader("Content-Type", "application/json");
}

}