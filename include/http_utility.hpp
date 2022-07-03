#pragma once
#include "http_request.hpp"

#include <ctype.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/constants.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/type_index/type_index_facade.hpp>

#include <iomanip>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

// IWYU pragma: no_include <boost/algorithm/string/detail/classification.hpp>

namespace http_helpers
{
inline std::vector<std::string> parseAccept(std::string_view header)
{
    std::vector<std::string> encodings;
    // chrome currently sends 6 accepts headers, firefox sends 4.
    encodings.reserve(6);
    boost::split(encodings, header, boost::is_any_of(", "),
                 boost::token_compress_on);

    return encodings;
}

inline bool requestPrefersHtml(std::string_view header)
{
    for (const std::string& encoding : parseAccept(header))
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

inline bool isOctetAccepted(std::string_view header)
{
    for (const std::string& encoding : parseAccept(header))
    {
        if (encoding == "*/*" || encoding == "application/octet-stream")
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
        if ((isalnum(c) != 0) || c == '-' || c == '_' || c == '.' || c == '~')
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
