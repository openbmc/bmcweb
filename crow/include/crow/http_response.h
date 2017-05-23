#pragma once
#include <string>

#include "crow/ci_map.h"
#include "crow/http_request.h"
#include "crow/json.h"
#include "crow/logging.h"

namespace crow {
template <typename Adaptor, typename Handler, typename... Middlewares>
class Connection;
struct response {
  template <typename Adaptor, typename Handler, typename... Middlewares>
  friend class crow::Connection;

  int code{200};
  std::string body;
  json::wvalue json_value;

  // `headers' stores HTTP headers.
  //ci_map headers;

  std::string headers;

  void add_header(const std::string& key, const std::string& value) {

    const static std::string seperator = ": ";
    const static std::string crlf = "\r\n";
    headers.append(key);
    headers.append(seperator);
    headers.append(value);
    headers.append(crlf);
  }

  response() {}
  explicit response(int code) : code(code) {}
  response(std::string body) : body(std::move(body)) {}
  response(json::wvalue&& json_value) : json_value(std::move(json_value)) {
    json_mode();
  }
  response(int code, std::string body) : code(code), body(std::move(body)) {}
  response(const json::wvalue& json_value) : body(json::dump(json_value)) {
    json_mode();
  }
  response(int code, const json::wvalue& json_value)
      : code(code), body(json::dump(json_value)) {
    json_mode();
  }

  response(response&& r) {
    CROW_LOG_WARNING << "Moving response containers";
    *this = std::move(r);
  }

  ~response(){
    CROW_LOG_WARNING << "Destroying response";
  }

  response& operator=(const response& r) = delete;

  response& operator=(response&& r) noexcept {
    CROW_LOG_WARNING << "Moving response containers";
    body = std::move(r.body);
    json_value = std::move(r.json_value);
    code = r.code;
    headers = std::move(r.headers);
    completed_ = r.completed_;
    return *this;
  }

  bool is_completed() const noexcept { return completed_; }

  void clear() {
    CROW_LOG_WARNING << "Clearing response containers";
    body.clear();
    json_value.clear();
    code = 200;
    headers.clear();
    completed_ = false;
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
}
