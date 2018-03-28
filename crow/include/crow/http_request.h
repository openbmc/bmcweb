#pragma once

#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>

#include "crow/common.h"
#include "crow/query_string.h"

namespace crow {

struct request {
  boost::string_view url{};
  query_string url_params{};
  bool is_secure{false};

  const std::string& body;

  void* middleware_context{};
  boost::asio::io_service* io_service{};

  request(boost::beast::http::request<boost::beast::http::string_body>& req)
      : req(req), body(req.body()) {}

  const boost::beast::http::verb method() const { return req.method(); }

  const boost::string_view get_header_value(
      boost::string_view key) const {
    return req[key];
  }

  const boost::string_view get_header_value(
      boost::beast::http::field key) const {
    return req[key];
  }

  const boost::string_view method_string() const {
    return req.method_string();
  }

  const boost::string_view target() const { return req.target(); }

  unsigned version() { return req.version(); }

  bool is_upgrade() { return boost::beast::websocket::is_upgrade(req); }

  bool keep_alive() { return req.keep_alive(); }

 private:
  boost::beast::http::request<boost::beast::http::string_body>& req;
};

}  // namespace crow
