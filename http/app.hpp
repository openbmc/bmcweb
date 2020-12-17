#pragma once

#include "http_request.hpp"
#include "http_server.hpp"
#include "logging.hpp"
#include "privileges.hpp"
#include "routing.hpp"
#include "utility.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <utility>

#define BMCWEB_ROUTE(app, url)                                                 \
    app.template route<crow::black_magic::getParameterTag(url)>(url)

namespace crow
{
#ifdef BMCWEB_ENABLE_SSL
using ssl_context_t = boost::asio::ssl::context;
#endif
class App
{
  public:
#ifdef BMCWEB_ENABLE_SSL
    using ssl_socket_t = boost::beast::ssl_stream<boost::asio::ip::tcp::socket>;
    using ssl_server_t = Server<App, ssl_socket_t>;
#else
    using socket_t = boost::asio::ip::tcp::socket;
    using server_t = Server<App, socket_t>;
#endif

    explicit App(std::shared_ptr<boost::asio::io_context> ioIn =
                     std::make_shared<boost::asio::io_context>()) :
        io(std::move(ioIn))
    {}
    ~App()
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

    template <uint64_t Tag>
    auto& route(std::string&& rule)
    {
        return router.newRuleTagged<Tag>(std::move(rule));
    }

    App& socket(int existingSocket)
    {
        socketFd = existingSocket;
        return *this;
    }

    App& port(std::uint16_t port)
    {
        portUint = port;
        return *this;
    }

    App& bindaddr(std::string bindaddr)
    {
        bindaddrStr = std::move(bindaddr);
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
            sslServer = std::make_unique<ssl_server_t>(
                this, bindaddrStr, portUint, sslContext, io);
        }
        else
        {
            sslServer =
                std::make_unique<ssl_server_t>(this, socketFd, sslContext, io);
        }
        sslServer->run();

#else

        if (-1 == socketFd)
        {
            server = std::move(std::make_unique<server_t>(
                this, bindaddrStr, portUint, nullptr, io));
        }
        else
        {
            server = std::move(
                std::make_unique<server_t>(this, socketFd, nullptr, io));
        }
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
    App& sslFile(const std::string& crtFilename, const std::string& keyFilename)
    {
        sslContext = std::make_shared<ssl_context_t>(
            boost::asio::ssl::context::tls_server);
        sslContext->set_verify_mode(boost::asio::ssl::verify_peer);
        sslContext->use_certificate_file(crtFilename, ssl_context_t::pem);
        sslContext->use_private_key_file(keyFilename, ssl_context_t::pem);
        sslContext->set_options(boost::asio::ssl::context::default_workarounds |
                                boost::asio::ssl::context::no_sslv2 |
                                boost::asio::ssl::context::no_sslv3 |
                                boost::asio::ssl::context::no_tlsv1 |
                                boost::asio::ssl::context::no_tlsv1_1);
        return *this;
    }

    App& sslFile(const std::string& pemFilename)
    {
        sslContext = std::make_shared<ssl_context_t>(
            boost::asio::ssl::context::tls_server);
        sslContext->set_verify_mode(boost::asio::ssl::verify_peer);
        sslContext->load_verify_file(pemFilename);
        sslContext->set_options(boost::asio::ssl::context::default_workarounds |
                                boost::asio::ssl::context::no_sslv2 |
                                boost::asio::ssl::context::no_sslv3 |
                                boost::asio::ssl::context::no_tlsv1 |
                                boost::asio::ssl::context::no_tlsv1_1);
        return *this;
    }

    App& ssl(std::shared_ptr<boost::asio::ssl::context>&& ctx)
    {
        sslContext = std::move(ctx);
        BMCWEB_LOG_INFO << "app::ssl context use_count="
                        << sslContext.use_count();
        return *this;
    }

    std::shared_ptr<ssl_context_t> sslContext = nullptr;

#else
    template <typename T, typename... Remain>
    App& ssl_file(T&&, Remain&&...)
    {
        // We can't call .ssl() member function unless BMCWEB_ENABLE_SSL is
        // defined.
        static_assert(
            // make static_assert dependent to T; always false
            std::is_base_of<T, void>::value,
            "Define BMCWEB_ENABLE_SSL to enable ssl support.");
        return *this;
    }

    template <typename T>
    App& ssl(T&&)
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

  private:
    std::shared_ptr<boost::asio::io_context> io;
#ifdef BMCWEB_ENABLE_SSL
    uint16_t portUint = 443;
#else
    uint16_t portUint = 80;
#endif
    std::string bindaddrStr = "::";
    int socketFd = -1;
    Router router;

#ifdef BMCWEB_ENABLE_SSL
    std::unique_ptr<ssl_server_t> sslServer;
#else
    std::unique_ptr<server_t> server;
#endif
};
} // namespace crow
using App = crow::App;
