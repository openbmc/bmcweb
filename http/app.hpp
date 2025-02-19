// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "http_connect_types.hpp"
#include "http_request.hpp"
#include "http_server.hpp"
#include "io_context_singleton.hpp"
#include "logging.hpp"
#include "routing.hpp"
#include "routing/dynamicrule.hpp"
#include "str_utility.hpp"

#include <sys/socket.h>
#include <systemd/sd-daemon.h>

#include <boost/asio/ip/tcp.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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

    static HttpType getHttpType(std::string_view socketTypeString)
    {
        if (socketTypeString == "http")
        {
            BMCWEB_LOG_DEBUG("Got http socket");
            return HttpType::HTTP;
        }
        if (socketTypeString == "https")
        {
            BMCWEB_LOG_DEBUG("Got https socket");
            return HttpType::HTTPS;
        }
        if (socketTypeString == "both")
        {
            BMCWEB_LOG_DEBUG("Got hybrid socket");
            return HttpType::BOTH;
        }

        // all other types https
        BMCWEB_LOG_ERROR("Unknown http type={} assuming HTTPS only",
                         socketTypeString);
        return HttpType::HTTPS;
    }

    static std::vector<Acceptor> setupSocket()
    {
        std::vector<Acceptor> acceptors;
        char** names = nullptr;
        int listenFdCount = sd_listen_fds_with_names(0, &names);
        BMCWEB_LOG_DEBUG("Got {} sockets to open", listenFdCount);

        if (listenFdCount < 0)
        {
            BMCWEB_LOG_CRITICAL("Failed to read socket files");
            return acceptors;
        }
        int socketIndex = 0;
        for (char* name :
             std::span<char*>(names, static_cast<size_t>(listenFdCount)))
        {
            if (name == nullptr)
            {
                continue;
            }
            // name looks like bmcweb_443_https_auth
            // Assume HTTPS as default
            std::string socketName(name);

            std::vector<std::string> socknameComponents;
            bmcweb::split(socknameComponents, socketName, '_');
            HttpType httpType = getHttpType(socknameComponents[2]);

            int listenFd = socketIndex + SD_LISTEN_FDS_START;
            if (sd_is_socket_inet(listenFd, AF_UNSPEC, SOCK_STREAM, 1, 0) > 0)
            {
                BMCWEB_LOG_INFO("Starting webserver on socket handle {}",
                                listenFd);
                acceptors.emplace_back(Acceptor{
                    boost::asio::ip::tcp::acceptor(
                        getIoContext(), boost::asio::ip::tcp::v6(), listenFd),
                    httpType});
            }
            socketIndex++;
        }

        if (acceptors.empty())
        {
            constexpr int defaultPort = 18080;
            BMCWEB_LOG_INFO("Starting webserver on port {}", defaultPort);
            using boost::asio::ip::tcp;
            tcp::endpoint end(tcp::v6(), defaultPort);
            tcp::acceptor acc(getIoContext(), end);
            acceptors.emplace_back(std::move(acc), HttpType::HTTPS);
        }

        return acceptors;
    }

    void run()
    {
        validate();

        std::vector<Acceptor> acceptors = setupSocket();

        server.emplace(this, std::move(acceptors));
        server->run();
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

    std::optional<server_type> server;

    Router router;
};
} // namespace crow
using App = crow::App;
