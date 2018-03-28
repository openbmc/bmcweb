#pragma once

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "crow/utility.h"
#include <boost/beast/http/verb.hpp>

namespace crow {
enum class HTTPMethod {
#ifndef DELETE
  DELETE = 0,
  GET,
  HEAD,
  POST,
  PUT,
  CONNECT,
  OPTIONS,
  TRACE,
  PATCH = 24,  // see http_parser_merged.h line 118 for why it is 24
#endif

  Delete = 0,
  Get,
  Head,
  Post,
  Put,
  Connect,
  Options,
  Trace,
  Patch = 24,
};

inline std::string method_name(boost::beast::http::verb method) {
  switch (method) {
    case boost::beast::http::verb::delete_:
      return "DELETE";
    case boost::beast::http::verb::get:
      return "GET";
    case boost::beast::http::verb::head:
      return "HEAD";
    case boost::beast::http::verb::post:
      return "POST";
    case boost::beast::http::verb::put:
      return "PUT";
    case boost::beast::http::verb::connect:
      return "CONNECT";
    case boost::beast::http::verb::options:
      return "OPTIONS";
    case boost::beast::http::verb::trace:
      return "TRACE";
    case boost::beast::http::verb::patch:
      return "PATCH";
  }
  return "invalid";
}

enum class ParamType {
  INT,
  UINT,
  DOUBLE,
  STRING,
  PATH,

  MAX
};

struct routing_params {
  std::vector<int64_t> int_params;
  std::vector<uint64_t> uint_params;
  std::vector<double> double_params;
  std::vector<std::string> string_params;

  void debug_print() const {
    std::cerr << "routing_params" << std::endl;
    for (auto i : int_params) {
      std::cerr << i << ", ";
    }
    std::cerr << std::endl;
    for (auto i : uint_params) {
      std::cerr << i << ", ";
    }
    std::cerr << std::endl;
    for (auto i : double_params) {
      std::cerr << i << ", ";
    }
    std::cerr << std::endl;
    for (auto& i : string_params) {
      std::cerr << i << ", ";
    }
    std::cerr << std::endl;
  }

  template <typename T>
  T get(unsigned) const;
};

template <>
inline int64_t routing_params::get<int64_t>(unsigned index) const {
  return int_params[index];
}

template <>
inline uint64_t routing_params::get<uint64_t>(unsigned index) const {
  return uint_params[index];
}

template <>
inline double routing_params::get<double>(unsigned index) const {
  return double_params[index];
}

template <>
inline std::string routing_params::get<std::string>(unsigned index) const {
  return string_params[index];
}

}  // namespace crow

constexpr boost::beast::http::verb operator"" _method(const char* str,
                                                      size_t /*len*/) {
  using verb = boost::beast::http::verb;
  // clang-format off
  return
    crow::black_magic::is_equ_p(str, "GET", 3) ? verb::get :
    crow::black_magic::is_equ_p(str, "DELETE", 6) ? verb::delete_ :
    crow::black_magic::is_equ_p(str, "HEAD", 4) ? verb::head :
    crow::black_magic::is_equ_p(str, "POST", 4) ? verb::post :
    crow::black_magic::is_equ_p(str, "PUT", 3) ? verb::put :
    crow::black_magic::is_equ_p(str, "OPTIONS", 7) ? verb::options :
    crow::black_magic::is_equ_p(str, "CONNECT", 7) ? verb::connect :
    crow::black_magic::is_equ_p(str, "TRACE", 5) ? verb::trace :
    crow::black_magic::is_equ_p(str, "PATCH", 5) ? verb::patch :
    crow::black_magic::is_equ_p(str, "PURGE", 5) ? verb::purge :
    throw std::runtime_error("invalid http method");
  // clang-format on
}