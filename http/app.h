#pragma once

#include "privileges.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <utility>

#include "http_request.h"
#include "http_server.h"
#include "logging.h"
#include "middleware_context.h"
#include "routing.h"
#include "utility.h"

#define BMCWEB_ROUTE(app, url)                                                 \
    app.template route<crow::black_magic::get_parameter_tag(url)>(url)

namespace crow
{
#ifdef BMCWEB_ENABLE_SSL
using ssl_context_t = boost::asio::ssl::context;
#endif
template <typename... Middlewares> class Crow
{
  public:
    using self_t = Crow;

#ifdef BMCWEB_ENABLE_SSL
    using ssl_socket_t = boost::beast::ssl_stream<boost::asio::ip::tcp::socket>;
    using ssl_server_t = Server<Crow, ssl_socket_t, Middlewares...>;
#else
    using socket_t = boost::asio::ip::tcp::socket;
    using server_t = Server<Crow, socket_t, Middlewares...>;
#endif

    explicit Crow(std::shared_ptr<boost::asio::io_context> ioIn =
                      std::make_shared<boost::asio::io_context>()) :
        io(std::move(ioIn))
    {
    }
    ~Crow()
    {
        this->stop();
    }

    template <typename Adaptor>
    void handleUpgrade(const Request& req, Response& res, Adaptor&& adaptor)
    {
        router.handleUpgrade(req, res, std::move(adaptor));
    }

    void handle(Request& req, Response& res)
    {
        router.handle(req, res);
    }

    DynamicRule& routeDynamic(std::string&& rule)
    {
        return router.newRuleDynamic(rule);
    }

    template <uint64_t Tag> auto& route(std::string&& rule)
    {
        return router.newRuleTagged<Tag>(std::move(rule));
    }

    self_t& socket(int existing_socket)
    {
        socketFd = existing_socket;
        return *this;
    }

    self_t& port(std::uint16_t port)
    {
        portUint = port;
        return *this;
    }

    self_t& bindaddr(std::string bindaddr)
    {
        bindaddrStr = bindaddr;
        return *this;
    }

    void validate()
    {
        router.validate();
    }

    void run()
    {
        validate();
#ifdef BMCWEB_ENABLE_SSL
        if (-1 == socketFd)
        {
            sslServer = std::move(std::make_unique<ssl_server_t>(
                this, bindaddrStr, portUint, sslContext, &middlewares, io));
        }
        else
        {
            sslServer = std::move(std::make_unique<ssl_server_t>(
                this, socketFd, sslContext, &middlewares, io));
        }
        sslServer->setTickFunction(tickInterval, tickFunction);
        sslServer->run();

#else

        if (-1 == socketFd)
        {
            server = std::move(std::make_unique<server_t>(
                this, bindaddrStr, portUint, nullptr, &middlewares, io));
        }
        else
        {
            server = std::move(std::make_unique<server_t>(
                this, socketFd, nullptr, &middlewares, io));
        }
        server->setTickFunction(tickInterval, tickFunction);
        server->run();

#endif
    }

    void stop()
    {
        io->stop();
    }

    void debugPrint()
    {
        BMCWEB_LOG_DEBUG << "Routing:";
        router.debugPrint();
    }

    std::vector<const std::string*> getRoutes()
    {
        const std::string root("");
        return router.getRoutes(root);
    }
    std::vector<const std::string*> getRoutes(const std::string& parent)
    {
        return router.getRoutes(parent);
    }

#ifdef BMCWEB_ENABLE_SSL
    self_t& sslFile(const std::string& crt_filename,
                    const std::string& key_filename)
    {
        sslContext = std::make_shared<ssl_context_t>(
            boost::asio::ssl::context::tls_server);
        sslContext->set_verify_mode(boost::asio::ssl::verify_peer);
        sslContext->use_certificate_file(crt_filename, ssl_context_t::pem);
        sslContext->use_private_key_file(key_filename, ssl_context_t::pem);
        sslContext->set_options(boost::asio::ssl::context::default_workarounds |
                                boost::asio::ssl::context::no_sslv2 |
                                boost::asio::ssl::context::no_sslv3 |
                                boost::asio::ssl::context::no_tlsv1 |
                                boost::asio::ssl::context::no_tlsv1_1);
        return *this;
    }

    self_t& sslFile(const std::string& pem_filename)
    {
        sslContext = std::make_shared<ssl_context_t>(
            boost::asio::ssl::context::tls_server);
        sslContext->set_verify_mode(boost::asio::ssl::verify_peer);
        sslContext->load_verify_file(pem_filename);
        sslContext->set_options(boost::asio::ssl::context::default_workarounds |
                                boost::asio::ssl::context::no_sslv2 |
                                boost::asio::ssl::context::no_sslv3 |
                                boost::asio::ssl::context::no_tlsv1 |
                                boost::asio::ssl::context::no_tlsv1_1);
        return *this;
    }

    self_t& ssl(std::shared_ptr<boost::asio::ssl::context>&& ctx)
    {
        sslContext = std::move(ctx);
        BMCWEB_LOG_INFO << "app::ssl context use_count="
                        << sslContext.use_count();
        return *this;
    }

    std::shared_ptr<ssl_context_t> sslContext = nullptr;

#else
    template <typename T, typename... Remain> self_t& ssl_file(T&&, Remain&&...)
    {
        // We can't call .ssl() member function unless BMCWEB_ENABLE_SSL is
        // defined.
        static_assert(
            // make static_assert dependent to T; always false
            std::is_base_of<T, void>::value,
            "Define BMCWEB_ENABLE_SSL to enable ssl support.");
        return *this;
    }

    template <typename T> self_t& ssl(T&&)
    {
        // We can't call .ssl() member function unless BMCWEB_ENABLE_SSL is
        // defined.
        static_assert(
            // make static_assert dependent to T; always false
            std::is_base_of<T, void>::value,
            "Define BMCWEB_ENABLE_SSL to enable ssl support.");
        return *this;
    }
#endif

    // middleware
    using context_t = detail::Context<Middlewares...>;
    template <typename T> typename T::Context& getContext(const Request& req)
    {
        static_assert(black_magic::Contains<T, Middlewares...>::value,
                      "App doesn't have the specified middleware type.");
        auto& ctx = *reinterpret_cast<context_t*>(req.middlewareContext);
        return ctx.template get<T>();
    }

    template <typename T> T& getMiddleware()
    {
        return utility::getElementByType<T, Middlewares...>(middlewares);
    }

    template <typename Duration, typename Func> self_t& tick(Duration d, Func f)
    {
        tickInterval = std::chrono::duration_cast<std::chrono::milliseconds>(d);
        tickFunction = f;
        return *this;
    }

  private:
    std::shared_ptr<asio::io_context> io;
#ifdef BMCWEB_ENABLE_SSL
    uint16_t portUint = 443;
#else
    uint16_t portUint = 80;
#endif
    std::string bindaddrStr = "::";
    int socketFd = -1;
    Router router;

    std::chrono::milliseconds tickInterval{};
    std::function<void()> tickFunction;

    std::tuple<Middlewares...> middlewares;

#ifdef BMCWEB_ENABLE_SSL
    std::unique_ptr<ssl_server_t> sslServer;
#else
    std::unique_ptr<server_t> server;
#endif
};
template <typename... Middlewares> using App = Crow<Middlewares...>;
using SimpleApp = Crow<>;
} // namespace crow
