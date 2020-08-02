#pragma once

#include "async_resp.hpp"
#include "http_request.hpp"
#include "http_server.hpp"
#include "logging.hpp"
#include "privileges.hpp"
#include "routing.hpp"
#include "utility.hpp"

#include <systemd/sd-daemon.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>

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
    using raw_socket_t = boost::asio::ip::tcp::socket;
    using server_type = Server<App, raw_socket_t>;

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
    void handleUpgrade(const std::shared_ptr<Request>& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       Adaptor&& adaptor)
    {
        router.handleUpgrade(req, asyncResp, std::forward<Adaptor>(adaptor));
    }

    void handle(const std::shared_ptr<Request>& req,
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

    void validate()
    {
        router.validate();
    }

    void loadCertificate()
    {
        BMCWEB_LOG_DEBUG("Loading certificate");
        if (!server)
        {
            return;
        }
        server->loadCertificate();
    }

    std::vector<boost::asio::ip::tcp::acceptor> setupSocket()
    {
        std::vector<boost::asio::ip::tcp::acceptor> acceptors;
        char** names = nullptr;
        int listenFdCount = sd_listen_fds_with_names(0, &names);
        BMCWEB_LOG_DEBUG("Got {} sockets to open", listenFdCount);
        for (int socketIndex = 0; socketIndex < listenFdCount; socketIndex++)
        {
            // Assume HTTPS as default
            [[maybe_unused]] HttpType httpType = HttpType::HTTPS;
            if (names != nullptr)
            {
                if (names[socketIndex] != nullptr)
                {
                    std::string socketName(names[socketIndex]);
                    size_t nameStart = socketName.rfind('_');
                    if (nameStart != std::string::npos)
                    {
                        std::string_view name = socketName.substr(nameStart);
                        if (name == "_http")
                        {
                            BMCWEB_LOG_DEBUG("Got http socket");
                            httpType = HttpType::HTTP;
                        }
                        else if (name == "_https")
                        {
                            BMCWEB_LOG_DEBUG("Got https socket");
                            httpType = HttpType::HTTPS;
                        }
                        else if (name == "_both")
                        {
                            BMCWEB_LOG_DEBUG("Got hybrid socket");
                            httpType = HttpType::BOTH;
                        }
                        else
                        {
                            // all other types https
                            BMCWEB_LOG_ERROR(
                                "Unknown socket type {} assuming HTTPS only",
                                socketName);
                        }
                    }
                }
            }

            int listenFd = socketIndex + SD_LISTEN_FDS_START;
            if (sd_is_socket_inet(listenFd, AF_UNSPEC, SOCK_STREAM, 1, 0))
            {
                BMCWEB_LOG_INFO("Starting webserver on socket handle {}",
                                listenFd);
                acceptors.emplace_back(

                    boost::asio::ip::tcp::acceptor(
                        *io, boost::asio::ip::tcp::v6(), listenFd)

                );
            }
        }

        return acceptors;
    }

    void run()
    {
        validate();

        std::vector<boost::asio::ip::tcp::acceptor> acceptors = setupSocket();

        server.emplace(this, std::move(acceptors), sslContext, io);
        server->run();
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

    std::optional<server_type> server;

    Router router;
};
} // namespace crow
using App = crow::App;
