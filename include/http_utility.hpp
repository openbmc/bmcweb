#pragma once
#include <boost/algorithm/string.hpp>

namespace http_helpers {
inline bool request_prefers_html(const crow::request& req) {
  boost::string_view header = req.get_header_value("accept");
  std::vector<std::string> encodings;
  // chrome currently sends 6 accepts headers, firefox sends 4.
  encodings.reserve(6);
  boost::split(encodings, header, boost::is_any_of(", "),
               boost::token_compress_on);
  for (const std::string& encoding : encodings) {
    if (encoding == "text/html") {
      return true;
    } else if (encoding == "application/json") {
      return false;
    }
  }
  return false;
}
}  // namespace http_helpers