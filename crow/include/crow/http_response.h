#pragma once
#include <string>

#include "nlohmann/json.hpp"
#include "crow/ci_map.h"
#include "crow/http_request.h"
#include "crow/logging.h"

namespace crow {
template <typename Adaptor, typename Handler, typename... Middlewares>
class Connection;
struct response {
  template <typename Adaptor, typename Handler, typename... Middlewares>
  friend class crow::Connection;

  int code{200};
  std::string body;
  nlohmann::json json_value;

  std::shared_ptr<std::string> body_ptr;

  std::string headers;

  void add_header(const std::string& key, const std::string& value) {
    const static std::string seperator = ": ";
    const static std::string crlf = "\r\n";
    headers.append(key);
    headers.append(seperator);
    headers.append(value);
    headers.append(crlf);
  }

  response() = default;
  explicit response(int code) : code(code) {}
  explicit response(std::string body) : body(std::move(body)) {}
  explicit response(const char* body) : body(body) {}
  explicit response(nlohmann::json&& json_value)
      : json_value(std::move(json_value)) {
    json_mode();
  }
  response(int code, const char* body) : code(code), body(body) {}
  response(int code, std::string body) : code(code), body(std::move(body)) {}
  // TODO(ed) make pretty printing JSON configurable
  explicit response(const nlohmann::json& json_value)
      : body(json_value.dump(4)) {
    json_mode();
  }
  response(int code, const nlohmann::json& json_value)
      : code(code), body(json_value.dump(4)) {
    json_mode();
  }

  response(response&& r) {
    CROW_LOG_DEBUG << "Moving response containers";
    *this = std::move(r);
  }

  ~response() { CROW_LOG_DEBUG << "Destroying response"; }

  response& operator=(const response& r) = delete;

  response& operator=(response&& r) noexcept {
    CROW_LOG_DEBUG << "Moving response containers";
    body = std::move(r.body);
    json_value = std::move(r.json_value);
    code = r.code;
    headers = std::move(r.headers);
    completed_ = r.completed_;
    return *this;
  }

  bool is_completed() const noexcept { return completed_; }

  void clear() {
    CROW_LOG_DEBUG << "Clearing response containers";
    body.clear();
    json_value.clear();
    code = 200;
    headers.clear();
    completed_ = false;
    body_ptr.reset();
  }

  void write(const std::string& body_part) { body += body_part; }

  void end() {
    if (!completed_) {
      completed_ = true;

      if (complete_request_handler_) {
        complete_request_handler_();
      }
    }
  }

  void end(const std::string& body_part) {
    body += body_part;
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
