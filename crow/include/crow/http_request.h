#pragma once

#include <boost/asio.hpp>

#include "crow/ci_map.h"
#include "crow/common.h"
#include "crow/query_string.h"

namespace crow {
template <typename T>
inline const std::string& get_header_value(const T& headers,
                                           const std::string& key) {
  if (headers.count(key)) {
    return headers.find(key)->second;
  }
  static std::string empty;
  return empty;
}

struct request {
  HTTPMethod method{HTTPMethod::Get};
  std::string raw_url;
  std::string url;
  query_string url_params;
  ci_map headers;
  std::string body;
  bool is_secure{false};

  void* middleware_context{};
  boost::asio::io_service* io_service{};

  request() {}

  request(HTTPMethod method, std::string raw_url, std::string url,
          query_string url_params, ci_map headers, std::string body)
      : method(method),
        raw_url(std::move(raw_url)),
        url(std::move(url)),
        url_params(std::move(url_params)),
        headers(std::move(headers)),
        body(std::move(body)) {}

  void add_header(std::string key, std::string value) {
    headers.emplace(std::move(key), std::move(value));
  }

  const std::string& get_header_value(const std::string& key) const {
    return crow::get_header_value(headers, key);
  }
};
}  // namespace crow
