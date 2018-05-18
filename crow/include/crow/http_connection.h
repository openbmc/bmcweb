#pragma once
#include <array>
#include <atomic>
#include <chrono>
#include <regex>
#include <vector>
#include "crow/http_response.h"
#include "crow/logging.h"
#include "crow/middleware_context.h"
#include "crow/settings.h"
#include "crow/socket_adaptors.h"
#include "crow/timer_queue.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>

#ifdef CROW_ENABLE_SSL
#include <boost/asio/ssl.hpp>
#endif

namespace crow {

inline bool is_browser(const crow::request& req) {
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

inline void escape_html(std::string& data) {
  std::string buffer;
  // less than 5% of characters should be larger, so reserve a buffer of the
  // right size
  buffer.reserve(data.size() * 1.05);
  for (size_t pos = 0; pos != data.size(); ++pos) {
    switch (data[pos]) {
      case '&':
        buffer.append("&amp;");
        break;
      case '\"':
        buffer.append("&quot;");
        break;
      case '\'':
        buffer.append("&apos;");
        break;
      case '<':
        buffer.append("&lt;");
        break;
      case '>':
        buffer.append("&gt;");
        break;
      default:
        buffer.append(&data[pos], 1);
        break;
    }
  }
  data.swap(buffer);
}

inline void convert_to_links(std::string& s) {
  const static std::regex r{
      "(&quot;@odata\\.((id)|(context))&quot;[ \\n]*:[ "
      "\\n]*)(&quot;((?!&quot;).*)&quot;)"};
  s = std::regex_replace(s, r, "$1<a href=\"$6\">$5</a>");
}

inline void pretty_print_json(crow::response& res) {
  std::string value = res.json_value.dump(4);
  escape_html(value);
  convert_to_links(value);
  res.body() =
      "<html>\n"
      "<head>\n"
      "<title>Redfish API</title>\n"
      "<link rel=\"stylesheet\" type=\"text/css\" "
      "href=\"/styles/default.css\">\n"
      "<script src=\"/highlight.pack.js\"></script>"
      "<script>hljs.initHighlightingOnLoad();</script>"
      "</head>\n"
      "<body>\n"
      "<div style=\"max-width: 576px;margin:0 auto;\">\n"
      "<img src=\"/DMTF_Redfish_logo_2017.svg\" alt=\"redfish\" "
      "height=\"406px\" "
      "width=\"576px\">\n"
      "<br>\n"
      "<pre>\n"
      "<code class=\"json\">" +
      value +
      "</code>\n"
      "</pre>\n"
      "</div>\n"
      "</body>\n"
      "</html>\n";
}

using namespace boost;
using tcp = asio::ip::tcp;

namespace detail {
template <typename MW>
struct check_before_handle_arity_3_const {
  template <typename T, void (T::*)(request&, response&, typename MW::context&)
                            const = &T::before_handle>
  struct get {};
};

template <typename MW>
struct check_before_handle_arity_3 {
  template <typename T, void (T::*)(request&, response&,
                                    typename MW::context&) = &T::before_handle>
  struct get {};
};

template <typename MW>
struct check_after_handle_arity_3_const {
  template <typename T, void (T::*)(request&, response&, typename MW::context&)
                            const = &T::after_handle>
  struct get {};
};

template <typename MW>
struct check_after_handle_arity_3 {
  template <typename T, void (T::*)(request&, response&,
                                    typename MW::context&) = &T::after_handle>
  struct get {};
};

template <typename T>
struct is_before_handle_arity_3_impl {
  template <typename C>
  static std::true_type f(
      typename check_before_handle_arity_3_const<T>::template get<C>*);

  template <typename C>
  static std::true_type f(
      typename check_before_handle_arity_3<T>::template get<C>*);

  template <typename C>
  static std::false_type f(...);

 public:
  static const bool value = decltype(f<T>(nullptr))::value;
};

template <typename T>
struct is_after_handle_arity_3_impl {
  template <typename C>
  static std::true_type f(
      typename check_after_handle_arity_3_const<T>::template get<C>*);

  template <typename C>
  static std::true_type f(
      typename check_after_handle_arity_3<T>::template get<C>*);

  template <typename C>
  static std::false_type f(...);

 public:
  static const bool value = decltype(f<T>(nullptr))::value;
};

template <typename MW, typename Context, typename ParentContext>
typename std::enable_if<!is_before_handle_arity_3_impl<MW>::value>::type
before_handler_call(MW& mw, request& req, response& res, Context& ctx,
                    ParentContext& /*parent_ctx*/) {
  mw.before_handle(req, res, ctx.template get<MW>(), ctx);
}

template <typename MW, typename Context, typename ParentContext>
typename std::enable_if<is_before_handle_arity_3_impl<MW>::value>::type
before_handler_call(MW& mw, request& req, response& res, Context& ctx,
                    ParentContext& /*parent_ctx*/) {
  mw.before_handle(req, res, ctx.template get<MW>());
}

template <typename MW, typename Context, typename ParentContext>
typename std::enable_if<!is_after_handle_arity_3_impl<MW>::value>::type
after_handler_call(MW& mw, request& req, response& res, Context& ctx,
                   ParentContext& /*parent_ctx*/) {
  mw.after_handle(req, res, ctx.template get<MW>(), ctx);
}

template <typename MW, typename Context, typename ParentContext>
typename std::enable_if<is_after_handle_arity_3_impl<MW>::value>::type
after_handler_call(MW& mw, request& req, response& res, Context& ctx,
                   ParentContext& /*parent_ctx*/) {
  mw.after_handle(req, res, ctx.template get<MW>());
}

template <int N, typename Context, typename Container, typename CurrentMW,
          typename... Middlewares>
bool middleware_call_helper(Container& middlewares, request& req, response& res,
                            Context& ctx) {
  using parent_context_t = typename Context::template partial<N - 1>;
  before_handler_call<CurrentMW, Context, parent_context_t>(
      std::get<N>(middlewares), req, res, ctx,
      static_cast<parent_context_t&>(ctx));

  if (res.is_completed()) {
    after_handler_call<CurrentMW, Context, parent_context_t>(
        std::get<N>(middlewares), req, res, ctx,
        static_cast<parent_context_t&>(ctx));
    return true;
  }

  if (middleware_call_helper<N + 1, Context, Container, Middlewares...>(
          middlewares, req, res, ctx)) {
    after_handler_call<CurrentMW, Context, parent_context_t>(
        std::get<N>(middlewares), req, res, ctx,
        static_cast<parent_context_t&>(ctx));
    return true;
  }

  return false;
}

template <int N, typename Context, typename Container>
bool middleware_call_helper(Container& /*middlewares*/, request& /*req*/,
                            response& /*res*/, Context& /*ctx*/) {
  return false;
}

template <int N, typename Context, typename Container>
typename std::enable_if<(N < 0)>::type after_handlers_call_helper(
    Container& /*middlewares*/, Context& /*context*/, request& /*req*/,
    response& /*res*/) {}

template <int N, typename Context, typename Container>
typename std::enable_if<(N == 0)>::type after_handlers_call_helper(
    Container& middlewares, Context& ctx, request& req, response& res) {
  using parent_context_t = typename Context::template partial<N - 1>;
  using CurrentMW = typename std::tuple_element<
      N, typename std::remove_reference<Container>::type>::type;
  after_handler_call<CurrentMW, Context, parent_context_t>(
      std::get<N>(middlewares), req, res, ctx,
      static_cast<parent_context_t&>(ctx));
}

template <int N, typename Context, typename Container>
typename std::enable_if<(N > 0)>::type after_handlers_call_helper(
    Container& middlewares, Context& ctx, request& req, response& res) {
  using parent_context_t = typename Context::template partial<N - 1>;
  using CurrentMW = typename std::tuple_element<
      N, typename std::remove_reference<Container>::type>::type;
  after_handler_call<CurrentMW, Context, parent_context_t>(
      std::get<N>(middlewares), req, res, ctx,
      static_cast<parent_context_t&>(ctx));
  after_handlers_call_helper<N - 1, Context, Container>(middlewares, ctx, req,
                                                        res);
}
}  // namespace detail

#ifdef CROW_ENABLE_DEBUG
static std::atomic<int> connectionCount;
#endif
template <typename Adaptor, typename Handler, typename... Middlewares>
class Connection {
 public:
  Connection(boost::asio::io_service& io_service, Handler* handler,
             const std::string& server_name,
             std::tuple<Middlewares...>* middlewares,
             std::function<std::string()>& get_cached_date_str_f,
             detail::timer_queue& timer_queue,
             typename Adaptor::context* adaptor_ctx_)
      : adaptor_(io_service, adaptor_ctx_),
        handler_(handler),
        server_name_(server_name),
        middlewares_(middlewares),
        get_cached_date_str(get_cached_date_str_f),
        timer_queue(timer_queue) {
    parser_.emplace(std::piecewise_construct, std::make_tuple());

    parser_->body_limit(1024 * 1024 * 1);  // 1MB
    req_.emplace(parser_->get());
#ifdef CROW_ENABLE_DEBUG
    connectionCount++;
    CROW_LOG_DEBUG << this << " Connection open, total " << connectionCount;
#endif
  }

  ~Connection() {
    res.complete_request_handler_ = nullptr;
    cancel_deadline_timer();
#ifdef CROW_ENABLE_DEBUG
    connectionCount--;
    CROW_LOG_DEBUG << this << " Connection closed, total " << connectionCount;
#endif
  }

  decltype(std::declval<Adaptor>().raw_socket()) & socket() {
    return adaptor_.raw_socket();
  }

  void start() {
    adaptor_.start([this](const boost::system::error_code& ec) {
      if (!ec) {
        start_deadline();

        do_read_headers();
      } else {
        check_destroy();
      }
    });
  }

  void handle() {
    cancel_deadline_timer();
    bool is_invalid_request = false;
    const boost::string_view connection =
        req_->get_header_value(boost::beast::http::field::connection);

    // Check for HTTP version 1.1.
    if (req_->version() == 11) {
      if (req_->get_header_value(boost::beast::http::field::host).empty()) {
        is_invalid_request = true;
        res = response(boost::beast::http::status::bad_request);
      }
    }

    CROW_LOG_INFO << "Request: " << adaptor_.remote_endpoint() << " " << this
                  << " HTTP/" << req_->version() / 10 << "."
                  << req_->version() % 10 << ' ' << req_->method_string() << " "
                  << req_->target();

    need_to_call_after_handlers_ = false;

    if (!is_invalid_request) {
      res.complete_request_handler_ = [] {};
      res.is_alive_helper_ = [this]() -> bool { return adaptor_.is_open(); };

      ctx_ = detail::context<Middlewares...>();
      req_->middleware_context = (void*)&ctx_;
      req_->io_service = &adaptor_.get_io_service();
      detail::middleware_call_helper<0, decltype(ctx_), decltype(*middlewares_),
                                     Middlewares...>(*middlewares_, *req_, res,
                                                     ctx_);

      if (!res.completed_) {
        if (req_->is_upgrade() &&
            boost::iequals(
                req_->get_header_value(boost::beast::http::field::upgrade),
                "websocket")) {
          handler_->handle_upgrade(*req_, res, std::move(adaptor_));
          return;
        }
        res.complete_request_handler_ = [this] { this->complete_request(); };
        need_to_call_after_handlers_ = true;
        handler_->handle(*req_, res);
        if (req_->keep_alive()) {
          res.add_header("connection", "Keep-Alive");
        }
      } else {
        complete_request();
      }
    } else {
      complete_request();
    }
  }

  void complete_request() {
    CROW_LOG_INFO << "Response: " << this << ' ' << req_->url << ' '
                  << res.result_int() << " keepalive=" << req_->keep_alive();

    if (need_to_call_after_handlers_) {
      need_to_call_after_handlers_ = false;

      // call all after_handler of middlewares
      detail::after_handlers_call_helper<((int)sizeof...(Middlewares) - 1),
                                         decltype(ctx_),
                                         decltype(*middlewares_)>(
          *middlewares_, ctx_, *req_, res);
    }

    // auto self = this->shared_from_this();
    res.complete_request_handler_ = nullptr;

    if (!adaptor_.is_open()) {
      // CROW_LOG_DEBUG << this << " delete (socket is closed) " << is_reading
      // << ' ' << is_writing;
      // delete this;
      return;
    }
    if (res.body().empty() && !res.json_value.empty()) {
      if (is_browser(*req_)) {
        pretty_print_json(res);
      } else {
        res.json_mode();
        res.body() = res.json_value.dump(2);
      }
    }

    if (res.result_int() >= 400 && res.body().empty()) {
      res.body() = std::string(res.reason());
    }
    res.add_header(boost::beast::http::field::server, server_name_);
    res.add_header(boost::beast::http::field::date, get_cached_date_str());

    res.keep_alive(req_->keep_alive());

    do_write();
  }

 private:
  void do_read_headers() {
    // auto self = this->shared_from_this();
    is_reading = true;
    CROW_LOG_DEBUG << this << " do_read_headers";

    // Clean up any previous connection.
    boost::beast::http::async_read_header(
        adaptor_.socket(), buffer_, *parser_,
        [this](const boost::system::error_code& ec,
               std::size_t bytes_transferred) {
          is_reading = false;
          CROW_LOG_ERROR << this << " async_read_header " << bytes_transferred
                         << " Bytes";
          bool error_while_reading = false;
          if (ec) {
            error_while_reading = true;
            CROW_LOG_ERROR << this << " Error while reading: " << ec.message();
          } else {
            // if the adaptor isn't open anymore, and wasn't handed to a
            // websocket, treat as an error
            if (!adaptor_.is_open() && !req_->is_upgrade()) {
              error_while_reading = true;
            }
          }

          if (error_while_reading) {
            cancel_deadline_timer();
            adaptor_.close();
            CROW_LOG_DEBUG << this << " from read(1)";
            check_destroy();
            return;
          }

          // Compute the url parameters for the request
          req_->url = req_->target();
          std::size_t index = req_->url.find("?");
          if (index != boost::string_view::npos) {
            req_->url = req_->url.substr(0, index - 1);
          }
          req_->url_params = query_string(std::string(req_->target()));
          do_read();
        });
  }

  void do_read() {
    // auto self = this->shared_from_this();
    is_reading = true;
    CROW_LOG_DEBUG << this << " do_read";

    boost::beast::http::async_read(
        adaptor_.socket(), buffer_, *parser_,
        [this](const boost::system::error_code& ec,
               std::size_t bytes_transferred) {
          CROW_LOG_ERROR << this << " async_read " << bytes_transferred
                         << " Bytes";
          is_reading = false;

          bool error_while_reading = false;
          if (ec) {
            CROW_LOG_ERROR << "Error while reading: " << ec.message();
            error_while_reading = true;
          } else {
            if (!adaptor_.is_open()) {
              error_while_reading = true;
            }
          }
          if (error_while_reading) {
            cancel_deadline_timer();
            adaptor_.close();
            CROW_LOG_DEBUG << this << " from read(1)";
            check_destroy();
            return;
          }
          handle();
        });
  }

  void do_write() {
    // auto self = this->shared_from_this();
    is_writing = true;
    CROW_LOG_DEBUG << "Doing Write";
    res.prepare_payload();
    serializer_.emplace(*res.string_response);
    boost::beast::http::async_write(
        adaptor_.socket(), *serializer_,
        [&](const boost::system::error_code& ec,
            std::size_t bytes_transferred) {
          is_writing = false;
          CROW_LOG_DEBUG << this << " Wrote " << bytes_transferred << " bytes";

          if (ec) {
            CROW_LOG_DEBUG << this << " from write(2)";
            check_destroy();
            return;
          }
          if (!req_->keep_alive()) {
            adaptor_.close();
            CROW_LOG_DEBUG << this << " from write(1)";
            check_destroy();
            return;
          }

          serializer_.reset();
          CROW_LOG_DEBUG << this << " Clearing response";
          res.clear();
          parser_.emplace(std::piecewise_construct, std::make_tuple());
          buffer_.consume(buffer_.size());

          req_.emplace(parser_->get());
          do_read_headers();
        });
  }

  void check_destroy() {
    CROW_LOG_DEBUG << this << " is_reading " << is_reading << " is_writing "
                   << is_writing;
    if (!is_reading && !is_writing) {
      CROW_LOG_DEBUG << this << " delete (idle) ";
      delete this;
    }
  }

  void cancel_deadline_timer() {
    CROW_LOG_DEBUG << this << " timer cancelled: " << &timer_queue << ' '
                   << timer_cancel_key_;
    timer_queue.cancel(timer_cancel_key_);
  }

  void start_deadline() {
    cancel_deadline_timer();

    timer_cancel_key_ = timer_queue.add([this] {
      if (!adaptor_.is_open()) {
        return;
      }
      adaptor_.close();
    });
    CROW_LOG_DEBUG << this << " timer added: " << &timer_queue << ' '
                   << timer_cancel_key_;
  }

 private:
  Adaptor adaptor_;
  Handler* handler_;

  // Making this a boost::optional allows it to be efficiently destroyed and
  // re-created on connection reset
  boost::optional<
      boost::beast::http::request_parser<boost::beast::http::string_body>>
      parser_;

  boost::beast::flat_buffer buffer_{8192};

  boost::optional<
      boost::beast::http::response_serializer<boost::beast::http::string_body>>
      serializer_;

  boost::optional<crow::request> req_;
  crow::response res;

  const std::string& server_name_;

  int timer_cancel_key_;

  bool is_reading{};
  bool is_writing{};
  bool need_to_call_after_handlers_{};
  bool need_to_start_read_after_complete_{};
  bool add_keep_alive_{};

  std::tuple<Middlewares...>* middlewares_;
  detail::context<Middlewares...> ctx_;

  std::function<std::string()>& get_cached_date_str;
  detail::timer_queue& timer_queue;
};  // namespace crow
}  // namespace crow
