#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include "crow/http_request.h"
#include "crow/http_server.h"
#include "crow/logging.h"
#include "crow/middleware_context.h"
#include "crow/routing.h"
#include "crow/settings.h"
#include "crow/utility.h"

#define CROW_ROUTE(app, url) \
  app.template route<crow::black_magic::get_parameter_tag(url)>(url)

namespace crow {
#ifdef CROW_ENABLE_SSL
using ssl_context_t = boost::asio::ssl::context;
#endif
template <typename... Middlewares>
class Crow {
 public:
  using self_t = Crow;
  using server_t = Server<Crow, SocketAdaptor, Middlewares...>;
#ifdef CROW_ENABLE_SSL
  using ssl_server_t = Server<Crow, SSLAdaptor, Middlewares...>;
#endif
  explicit Crow(std::shared_ptr<boost::asio::io_service> io =
                    std::make_shared<boost::asio::io_service>())
      : io_(std::move(io)) {}
  ~Crow() { this->stop(); }

  template <typename Adaptor>
  void handle_upgrade(const request& req, response& res, Adaptor&& adaptor) {
    router_.handle_upgrade(req, res, adaptor);
  }

  void handle(const request& req, response& res) { router_.handle(req, res); }

  DynamicRule& route_dynamic(std::string&& rule) {
    return router_.new_rule_dynamic(rule);
  }

  template <uint64_t Tag>
  auto route(std::string&& rule) -> typename std::result_of<
      decltype (&Router::new_rule_tagged<Tag>)(Router, std::string&&)>::type {
    return router_.new_rule_tagged<Tag>(std::move(rule));
  }

  self_t& port(std::uint16_t port) {
    port_ = port;
    return *this;
  }

  self_t& bindaddr(std::string bindaddr) {
    bindaddr_ = bindaddr;
    return *this;
  }

  self_t& multithreaded() {
    return concurrency(std::thread::hardware_concurrency());
  }

  self_t& concurrency(std::uint16_t concurrency) {
    if (concurrency < 1) {
      concurrency = 1;
    }
    concurrency_ = concurrency;
    return *this;
  }

  void validate() { router_.validate(); }

  void run() {
    validate();
#ifdef CROW_ENABLE_SSL
    if (use_ssl_) {
      ssl_server_ = std::move(
          std::make_unique<ssl_server_t>(this, bindaddr_, port_, &middlewares_,
                                         concurrency_, &ssl_context_, io_));
      ssl_server_->set_tick_function(tick_interval_, tick_function_);
      ssl_server_->run();
    } else
#endif
    {
      server_ = std::move(std::make_unique<server_t>(
          this, bindaddr_, port_, &middlewares_, concurrency_, nullptr, io_));
      server_->set_tick_function(tick_interval_, tick_function_);
      server_->run();
    }
  }

  void stop() {
#ifdef CROW_ENABLE_SSL
    if (use_ssl_) {
      if (ssl_server_ != nullptr) {
        ssl_server_->stop();
      }
    } else
#endif
    {
      if (server_ != nullptr) {
        server_->stop();
      }
    }
  }

  void debug_print() {
    CROW_LOG_DEBUG << "Routing:";
    router_.debug_print();
  }

  std::vector<const std::string*> get_routes() {
    // TODO(ed) Should this be /?
    const std::string root("");
    return router_.get_routes(root);
  }
  std::vector<const std::string*> get_routes(const std::string& parent) {
    return router_.get_routes(parent);
  }

#ifdef CROW_ENABLE_SSL
  self_t& ssl_file(const std::string& crt_filename,
                   const std::string& key_filename) {
    use_ssl_ = true;
    ssl_context_.set_verify_mode(boost::asio::ssl::verify_peer);
    ssl_context_.use_certificate_file(crt_filename, ssl_context_t::pem);
    ssl_context_.use_private_key_file(key_filename, ssl_context_t::pem);
    ssl_context_.set_options(boost::asio::ssl::context::default_workarounds |
                             boost::asio::ssl::context::no_sslv2 |
                             boost::asio::ssl::context::no_sslv3);
    return *this;
  }

  self_t& ssl_file(const std::string& pem_filename) {
    use_ssl_ = true;
    ssl_context_.set_verify_mode(boost::asio::ssl::verify_peer);
    ssl_context_.load_verify_file(pem_filename);
    ssl_context_.set_options(boost::asio::ssl::context::default_workarounds |
                             boost::asio::ssl::context::no_sslv2 |
                             boost::asio::ssl::context::no_sslv3);
    return *this;
  }

  self_t& ssl(boost::asio::ssl::context&& ctx) {
    use_ssl_ = true;
    ssl_context_ = std::move(ctx);
    return *this;
  }

  bool use_ssl_{false};
  ssl_context_t ssl_context_{boost::asio::ssl::context::sslv23};

#else
  template <typename T, typename... Remain>
  self_t& ssl_file(T&&, Remain&&...) {
    // We can't call .ssl() member function unless CROW_ENABLE_SSL is defined.
    static_assert(
        // make static_assert dependent to T; always false
        std::is_base_of<T, void>::value,
        "Define CROW_ENABLE_SSL to enable ssl support.");
    return *this;
  }

  template <typename T>
  self_t& ssl(T&&) {
    // We can't call .ssl() member function unless CROW_ENABLE_SSL is defined.
    static_assert(
        // make static_assert dependent to T; always false
        std::is_base_of<T, void>::value,
        "Define CROW_ENABLE_SSL to enable ssl support.");
    return *this;
  }
#endif

  // middleware
  using context_t = detail::context<Middlewares...>;
  template <typename T>
  typename T::context& get_context(const request& req) {
    static_assert(black_magic::contains<T, Middlewares...>::value,
                  "App doesn't have the specified middleware type.");
    auto& ctx = *reinterpret_cast<context_t*>(req.middleware_context);
    return ctx.template get<T>();
  }

  template <typename T>
  T& get_middleware() {
    return utility::get_element_by_type<T, Middlewares...>(middlewares_);
  }

  template <typename Duration, typename Func>
  self_t& tick(Duration d, Func f) {
    tick_interval_ = std::chrono::duration_cast<std::chrono::milliseconds>(d);
    tick_function_ = f;
    return *this;
  }

 private:
  std::shared_ptr<asio::io_service> io_;
  uint16_t port_ = 80;
  uint16_t concurrency_ = 1;
  std::string bindaddr_ = "0.0.0.0";
  Router router_;

  std::chrono::milliseconds tick_interval_{};
  std::function<void()> tick_function_;

  std::tuple<Middlewares...> middlewares_;

#ifdef CROW_ENABLE_SSL
  std::unique_ptr<ssl_server_t> ssl_server_;
#endif
  std::unique_ptr<server_t> server_;
};
template <typename... Middlewares>
using App = Crow<Middlewares...>;
using SimpleApp = Crow<>;
}  // namespace crow
