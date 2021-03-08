#pragma once
#include "http_request.hpp"

#include <boost/algorithm/string.hpp>

namespace http_helpers
{
inline std::string parseAccept(const crow::Request& req)
{
    std::string_view acceptHeader = req.getHeaderValue("Accept");
    // The iterators in boost/http/rfc7230.hpp end the string if '/' is found,
    // so replace it with arbitrary character '|' which is not part of the
    // Accept header syntax.
    return boost::replace_all_copy(std::string(acceptHeader), "/", "|");
}

inline bool requestPrefersHtml(const crow::Request& req)
{
    for (const auto& param : boost::beast::http::ext_list{parseAccept(req)})
    {
        if (param.first == "text|html")
        {
            return true;
        }
        if (param.first == "application|json")
        {
            return false;
        }
    }
    return false;
}

inline bool isOctetAccepted(const crow::Request& req)
{
    for (const auto& param : boost::beast::http::ext_list{parseAccept(req)})
    {
        if (param.first == "*|*" || param.first == "application|octet-stream")
        {
            return true;
        }
    }
    return false;
}

inline std::string urlEncode(const std::string_view value)
{
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (const char c : value)
    {
        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << std::uppercase;
        escaped << '%' << std::setw(2)
                << static_cast<int>(static_cast<unsigned char>(c));
        escaped << std::nouppercase;
    }

    return escaped.str();
}
} // namespace http_helpers
