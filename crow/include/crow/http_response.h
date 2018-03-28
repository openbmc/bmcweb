#pragma once
#include <string>

#include "nlohmann/json.hpp"
#include "crow/http_request.h"
#include "crow/logging.h"
#include <boost/beast/http.hpp>

namespace crow {

template <typename Adaptor, typename Handler, typename... Middlewares>
class Connection;

struct response {
  template <typename Adaptor, typename Handler, typename... Middlewares>
  friend class crow::Connection;
  using response_type =
      boost::beast::http::response<boost::beast::http::string_body>;

  boost::optional<response_type> string_response;

  nlohmann::json json_value;

  void add_header(const boost::string_view key,
                  const boost::string_view value) {
    string_response->set(key, value);
  }

  void add_header(boost::beast::http::field key, boost::string_view value) {
    string_response->set(key, value);
  }

  response() : string_response(response_type{}) {}

  explicit response(boost::beast::http::status code)
      : string_response(response_type{}) {}

  explicit response(boost::string_view body_)
      : string_response(response_type{}) {
    string_response->body() = std::string(body_);
  }

  response(boost::beast::http::status code, boost::string_view s)
      : string_response(response_type{}) {
    string_response->result(code);
    string_response->body() = std::string(s);
  }

  response(response&& r) {
    CROW_LOG_DEBUG << "Moving response containers";
    *this = std::move(r);
  }

  ~response() { CROW_LOG_DEBUG << this << " Destroying response"; }

  response& operator=(const response& r) = delete;

  response& operator=(response&& r) noexcept {
    CROW_LOG_DEBUG << "Moving response containers";
    string_response = std::move(r.string_response);
    r.string_response.emplace(response_type{});
    json_value = std::move(r.json_value);
    completed_ = r.completed_;
    return *this;
  }

  void result(boost::beast::http::status v) { string_response->result(v); }

  boost::beast::http::status result() { return string_response->result(); }

  unsigned result_int() { return string_response->result_int(); }

  boost::string_view reason() { return string_response->reason(); }

  bool is_completed() const noexcept { return completed_; }

  std::string& body() { return string_response->body(); }

  void keep_alive(bool k) { string_response->keep_alive(k); }

  void prepare_payload() { string_response->prepare_payload(); };

  void clear() {
    CROW_LOG_DEBUG << this << " Clearing response containers";
    string_response.emplace(response_type{});
    json_value.clear();
    completed_ = false;
  }

  void write(boost::string_view body_part) {
    string_response->body() += std::string(body_part);
  }

  void end() {
    if (!completed_) {
      completed_ = true;

      if (complete_request_handler_) {
        complete_request_handler_();
      }
    }
  }

  void end(boost::string_view body_part) {
    write(body_part);
    end();
  }

  bool is_alive() { return is_alive_helper_ && is_alive_helper_(); }

 private:
  bool completed_{};
  std::function<void()> complete_request_handler_;
  std::function<bool()> is_alive_helper_;

  // In case of a JSON object, set the Content-Type header
  void json_mode() { add_header("Content-Type", "application/json"); }
};
}  // namespace crow
