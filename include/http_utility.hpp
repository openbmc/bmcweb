#pragma once
#include "http_request.hpp"

#include <boost/algorithm/string.hpp>

namespace http_helpers
{
inline bool requestPrefersHtml(const crow::Request& req)
{
    std::string_view header = req.getHeaderValue("accept");
    std::vector<std::string> encodings;
    // chrome currently sends 6 accepts headers, firefox sends 4.
    encodings.reserve(6);
    boost::split(encodings, header, boost::is_any_of(", "),
                 boost::token_compress_on);
    for (const std::string& encoding : encodings)
    {
        if (encoding == "text/html")
        {
            return true;
        }
        if (encoding == "application/json")
        {
            return false;
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
