#pragma once

#include "http_request.h"
#include "http_server.h"
#include "logging.h"
#include "routing.h"
#include "utility.h"

#include "privileges.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <utility>

#define BMCWEB_ROUTE(app, url)                                                 \
    app.template route<crow::black_magic::get_parameter_tag(url)>(url)

namespace crow
{
#ifdef BMCWEB_ENABLE_SSL
using ssl_context_t = boost::asio::ssl::context;
#endif
class App
{
  public:
    using self_t = App;

    using socket_t = boost::asio::ip::tcp::socket;
    using server_t = Server<App, socket_t>;

    explicit App(std::shared_ptr<boost::asio::io_context> ioIn =
                     std::make_shared<boost::asio::io_context>()) :
        io(std::move(ioIn))
    {}
    ~App()
    {
        this->stop();
    }

    template <typename Adaptor>
    void handleUpgrade(
        const Request& req, Response& res, Adaptor&& adaptor,
        std::optional<boost::beast::ssl_stream<Adaptor&>>&& sslAdaptor)
    {
        router.handleUpgrade<Adaptor>(req, res, std::move(adaptor),
                                      std::move(sslAdaptor));
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

    self_t& add_socket(int existing_socket)
    {
        acceptors.emplace_back(std::make_unique<boost::asio::ip::tcp::acceptor>(
            *io, boost::asio::ip::tcp::v6(), existing_socket));
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

        if (acceptors.empty())
        {
            uint16_t defaultPort = 18080;
            boost::asio::ip::tcp::endpoint ep(
                boost::asio::ip::make_address("0.0.0.0"), defaultPort);
            boost::asio::ip::tcp::acceptor ac2(*io, ep);
            boost::asio::io_context& io2 = *io;
            std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor =
                std::make_unique<boost::asio::ip::tcp::acceptor>(io2, ep);
            acceptors.emplace_back(std::move(acceptor));
        }
        server = std::make_unique<server_t>(this, std::move(acceptors),
                                            sslContext, io);
        server->setTickFunction(tickInterval, tickFunction);
        server->run();
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
    template <typename T, typename... Remain>
    self_t& ssl_file(T&&, Remain&&...)
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
    self_t& ssl(T&&)
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

    template <typename Duration, typename Func>
    self_t& tick(Duration d, Func f)
    {
        tickInterval = std::chrono::duration_cast<std::chrono::milliseconds>(d);
        tickFunction = f;
        return *this;
    }

  private:
    std::shared_ptr<boost::asio::io_context> io;
#ifdef BMCWEB_ENABLE_SSL
    uint16_t portUint = 443;
#else
    uint16_t portUint = 80;
#endif
    std::string bindaddrStr = "::";
    std::vector<std::unique_ptr<boost::asio::ip::tcp::acceptor>> acceptors;
    Router router;

    std::chrono::milliseconds tickInterval{};
    std::function<void()> tickFunction;

    std::unique_ptr<server_t> server;
};
} // namespace crow
using App = crow::App;
