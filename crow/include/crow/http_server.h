#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <utility>
#include <vector>
#include "crow/timer_queue.h"
#include "crow/http_connection.h"
#include "crow/logging.h"
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#ifdef CROW_ENABLE_SSL
#include <boost/asio/ssl.hpp>
#endif

namespace crow {
using namespace boost;
using tcp = asio::ip::tcp;

template <typename Handler, typename Adaptor = SocketAdaptor,
          typename... Middlewares>
class Server {
 public:
  Server(Handler* handler, std::unique_ptr<tcp::acceptor>&& acceptor,
         std::tuple<Middlewares...>* middlewares = nullptr,
         typename Adaptor::context* adaptor_ctx = nullptr,
         std::shared_ptr<boost::asio::io_service> io =
             std::make_shared<boost::asio::io_service>())
      : io_service_(std::move(io)),
        acceptor_(std::move(acceptor)),
        signals_(*io_service_, SIGINT, SIGTERM),
        tick_timer_(*io_service_),
        handler_(handler),
        middlewares_(middlewares),
        adaptor_ctx_(adaptor_ctx) {}

  Server(Handler* handler, const std::string& bindaddr, uint16_t port,
         std::tuple<Middlewares...>* middlewares = nullptr,
         typename Adaptor::context* adaptor_ctx = nullptr,
         std::shared_ptr<boost::asio::io_service> io =
             std::make_shared<boost::asio::io_service>())
      : Server(handler,
               std::make_unique<tcp::acceptor>(
                   *io,
                   tcp::endpoint(
                       boost::asio::ip::address::from_string(bindaddr), port)),
               middlewares, adaptor_ctx, io) {}

  Server(Handler* handler, int existing_socket,
         std::tuple<Middlewares...>* middlewares = nullptr,
         typename Adaptor::context* adaptor_ctx = nullptr,
         std::shared_ptr<boost::asio::io_service> io =
             std::make_shared<boost::asio::io_service>())
      : Server(handler,
               std::make_unique<tcp::acceptor>(*io, boost::asio::ip::tcp::v6(),
                                               existing_socket),
               middlewares, adaptor_ctx, io) {}

  void set_tick_function(std::chrono::milliseconds d, std::function<void()> f) {
    tick_interval_ = d;
    tick_function_ = f;
  }

  void on_tick() {
    tick_function_();
    tick_timer_.expires_from_now(
        boost::posix_time::milliseconds(tick_interval_.count()));
    tick_timer_.async_wait([this](const boost::system::error_code& ec) {
      if (ec) {
        return;
      }
      on_tick();
    });
  }

  void update_date_str() {
    auto last_time_t = time(0);
    tm my_tm{};

#ifdef _MSC_VER
    gmtime_s(&my_tm, &last_time_t);
#else
    gmtime_r(&last_time_t, &my_tm);
#endif
    date_str.resize(100);
    size_t date_str_sz =
        strftime(&date_str[0], 99, "%a, %d %b %Y %H:%M:%S GMT", &my_tm);
    date_str.resize(date_str_sz);
  };

  void run() {
    update_date_str();

    get_cached_date_str_ = [this]() -> std::string {
      static std::chrono::time_point<std::chrono::steady_clock>
          last_date_update = std::chrono::steady_clock::now();
      if (std::chrono::steady_clock::now() - last_date_update >=
          std::chrono::seconds(10)) {
        last_date_update = std::chrono::steady_clock::now();
        update_date_str();
      }
      return this->date_str;
    };

    boost::asio::deadline_timer timer(*io_service_);
    timer.expires_from_now(boost::posix_time::seconds(1));

    std::function<void(const boost::system::error_code& ec)> handler;
    handler = [&](const boost::system::error_code& ec) {
      if (ec) {
        return;
      }
      timer_queue_.process();
      timer.expires_from_now(boost::posix_time::seconds(1));
      timer.async_wait(handler);
    };
    timer.async_wait(handler);

    if (tick_function_ && tick_interval_.count() > 0) {
      tick_timer_.expires_from_now(
          boost::posix_time::milliseconds(tick_interval_.count()));
      tick_timer_.async_wait([this](const boost::system::error_code& ec) {
        if (ec) {
          return;
        }
        on_tick();
      });
    }

    CROW_LOG_INFO << server_name_ << " server is running, local endpoint "
                  << acceptor_->local_endpoint();

    signals_.async_wait([&](const boost::system::error_code& /*error*/,
                            int /*signal_number*/) { stop(); });

    do_accept();
  }

  void stop() { io_service_->stop(); }

  void do_accept() {
    auto p = new Connection<Adaptor, Handler, Middlewares...>(
        *io_service_, handler_, server_name_, middlewares_,
        get_cached_date_str_, timer_queue_, adaptor_ctx_);
    acceptor_->async_accept(p->socket(),
                            [this, p](boost::system::error_code ec) {
                              if (!ec) {
                                this->io_service_->post([p] { p->start(); });
                              } else {
                                delete p;
                              }
                              do_accept();
                            });
  }

 private:
  std::shared_ptr<asio::io_service> io_service_;
  detail::timer_queue timer_queue_;
  std::function<std::string()> get_cached_date_str_;
  std::unique_ptr<tcp::acceptor> acceptor_;
  boost::asio::signal_set signals_;
  boost::asio::deadline_timer tick_timer_;

  std::string date_str;

  Handler* handler_;
  std::string server_name_ = "iBMC";

  std::chrono::milliseconds tick_interval_{};
  std::function<void()> tick_function_;

  std::tuple<Middlewares...>* middlewares_;

#ifdef CROW_ENABLE_SSL
  bool use_ssl_{false};
  boost::asio::ssl::context ssl_context_{boost::asio::ssl::context::sslv23};
#endif
  typename Adaptor::context* adaptor_ctx_;
};  // namespace crow
}  // namespace crow
