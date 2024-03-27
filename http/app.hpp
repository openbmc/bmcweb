#pragma once

#include "async_resp.hpp"
#include "http_request.hpp"
#include "http_server.hpp"
#include "logging.hpp"
#include "privileges.hpp"
#include "routing.hpp"
#include "utility.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <utility>

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage, clang-diagnostic-unused-macros)
#define BMCWEB_ROUTE(app, url)                                                 \
    app.template route<crow::utility::getParameterTag(url)>(url)

namespace crow
{
class App
{
  public:
    using ssl_socket_t = boost::beast::ssl_stream<boost::asio::ip::tcp::socket>;
    using ssl_server_t = Server<App, ssl_socket_t>;
    using socket_t = boost::asio::ip::tcp::socket;
    using server_t = Server<App, socket_t>;

    explicit App(std::shared_ptr<boost::asio::io_context> ioIn =
                     std::make_shared<boost::asio::io_context>()) :
        io(std::move(ioIn))
    {}
    ~App()
    {
        stop();
    }

    App(const App&) = delete;
    App(App&&) = delete;
    App& operator=(const App&) = delete;
    App& operator=(const App&&) = delete;

    template <typename Adaptor>
    void handleUpgrade(Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       Adaptor&& adaptor)
    {
        router.handleUpgrade(req, asyncResp, std::forward<Adaptor>(adaptor));
    }

    void handle(Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
    {
        router.handle(req, asyncResp);
    }

    DynamicRule& routeDynamic(const std::string& rule)
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
            sslServer = std::make_unique<ssl_server_t>(this, portUint,
                                                       sslContext, io);
        }
        else
        {
            sslServer = std::make_unique<ssl_server_t>(this, socketFd,
                                                       sslContext, io);
        }
        sslServer->run();

#else

        if (-1 == socketFd)
        {
            server = std::make_unique<server_t>(this, portUint, nullptr, io);
        }
        else
        {
            server = std::make_unique<server_t>(this, socketFd, nullptr, io);
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
        BMCWEB_LOG_DEBUG("Routing:");
        router.debugPrint();
    }

    std::vector<const std::string*> getRoutes()
    {
        const std::string root;
        return router.getRoutes(root);
    }
    std::vector<const std::string*> getRoutes(const std::string& parent)
    {
        return router.getRoutes(parent);
    }

    App& ssl(std::shared_ptr<boost::asio::ssl::context>&& ctx)
    {
        sslContext = std::move(ctx);
        BMCWEB_LOG_INFO("app::ssl context use_count={}",
                        sslContext.use_count());
        return *this;
    }

    std::shared_ptr<boost::asio::ssl::context> sslContext = nullptr;

    boost::asio::io_context& ioContext()
    {
        return *io;
    }

  private:
    std::shared_ptr<boost::asio::io_context> io;
#ifdef BMCWEB_ENABLE_SSL
    uint16_t portUint = 443;
#else
    uint16_t portUint = 80;
#endif
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
