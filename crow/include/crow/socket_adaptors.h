#pragma once
#include "crow/logging.h"
#include "crow/settings.h"
#include <boost/asio.hpp>
#ifdef CROW_ENABLE_SSL
#include <boost/asio/ssl.hpp>
#endif
namespace crow {
using namespace boost;
using tcp = asio::ip::tcp;

struct SocketAdaptor {
  using secure = std::false_type;
  using context = void;
  SocketAdaptor(boost::asio::io_service& io_service, context* /*unused*/)
      : socket_(io_service) {}

  boost::asio::io_service& get_io_service() { return socket_.get_io_service(); }

  tcp::socket& raw_socket() { return socket_; }

  tcp::socket& socket() { return socket_; }

  tcp::endpoint remote_endpoint() { return socket_.remote_endpoint(); }

  bool is_open() { return socket_.is_open(); }

  void close() { socket_.close(); }

  template <typename F>
  void start(F f) {
    f(boost::system::error_code());
  }

  tcp::socket socket_;
};

struct TestSocketAdaptor {
  using secure = std::false_type;
  using context = void;
  TestSocketAdaptor(boost::asio::io_service& io_service, context* /*unused*/)
      : socket_(io_service) {}

  boost::asio::io_service& get_io_service() { return socket_.get_io_service(); }

  tcp::socket& raw_socket() { return socket_; }

  tcp::socket& socket() { return socket_; }

  tcp::endpoint remote_endpoint() { return socket_.remote_endpoint(); }

  bool is_open() { return socket_.is_open(); }

  void close() { socket_.close(); }

  template <typename F>
  void start(F f) {
    f(boost::system::error_code());
  }

  tcp::socket socket_;
};

#ifdef CROW_ENABLE_SSL
struct SSLAdaptor {
  using secure = std::true_type;
  using context = boost::asio::ssl::context;
  using ssl_socket_t = boost::asio::ssl::stream<tcp::socket>;
  SSLAdaptor(boost::asio::io_service& io_service, context* ctx)
      : ssl_socket_(new ssl_socket_t(io_service, *ctx)) {}

  boost::asio::ssl::stream<tcp::socket>& socket() { return *ssl_socket_; }

  tcp::socket::lowest_layer_type& raw_socket() {
    return ssl_socket_->lowest_layer();
  }

  tcp::endpoint remote_endpoint() { return raw_socket().remote_endpoint(); }

  bool is_open() {
    /*TODO(ed) this is a bit of a cheat.
     There are cases  when running a websocket where ssl_socket_ might have
    std::move() called on it (to transfer ownership to websocket::Connection)
    and be empty.  This (and the check on close()) is a cheat to do something
    sane in this scenario. the correct fix would likely involve changing the
    http parser to return a specific code meaning "has been upgraded" so that
    the do_read function knows not to try to close the connection which would
    fail, because the adapter is gone.  As is, do_read believes the parse
    failed, because is_open now returns False (which could also mean the client
    disconnected during parse)
    UPdate: The parser does in fact have an "is_upgrade" method that is intended
    for exactly this purpose.  Todo is now to make do_read obey the flag
    appropriately so this code can be changed back.
    */
    if (ssl_socket_ != nullptr) {
      return ssl_socket_->lowest_layer().is_open();
    }
    return false;
  }

  void close() {
    if (ssl_socket_ == nullptr) {
      return;
    }
    boost::system::error_code ec;

    // Shut it down
    this->ssl_socket_->lowest_layer().close();
  }

  boost::asio::io_service& get_io_service() {
    return raw_socket().get_io_service();
  }

  template <typename F>
  void start(F f) {
    ssl_socket_->async_handshake(
        boost::asio::ssl::stream_base::server,
        [f](const boost::system::error_code& ec) { f(ec); });
  }

  std::unique_ptr<boost::asio::ssl::stream<tcp::socket>> ssl_socket_;
};
#endif
}  // namespace crow
