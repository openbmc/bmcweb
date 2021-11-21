#pragma once
#include "http_request_class_decl.hpp"
#include "http_server_class_decl.hpp"
#include "http_response_class_decl.hpp"
#include "../include/async_resp_class_decl.hpp"
#include "routing_class_decl.hpp"

#define BMCWEB_ROUTE(app, url)                                                 \
    app.template route<crow::black_magic::getParameterTag(url)>(url)

namespace crow {

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
                     std::make_shared<boost::asio::io_context>());

    ~App();

    template <typename Adaptor>
    void handleUpgrade(const Request& req, Response& res, Adaptor&& adaptor)
    {
        router.handleUpgrade(req, res, std::move(adaptor));
    }
    
    void handle(Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
    {
        router.handle(req, asyncResp);
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

    App& socket(int existingSocket);

    App& port(std::uint16_t port);

    App& bindaddr(std::string bindaddr);

    void validate();

    void run();

    void stop();

    void debugPrint();

    std::vector<const std::string*> getRoutes();

    std::vector<const std::string*> getRoutes(const std::string& parent);

#ifdef BMCWEB_ENABLE_SSL
    App& sslFile(const std::string& crtFilename, const std::string& keyFilename);

    App& sslFile(const std::string& pemFilename);

    App& ssl(std::shared_ptr<boost::asio::ssl::context>&& ctx);

    std::shared_ptr<ssl_context_t> sslContext = nullptr;

#else
    template <typename T, typename... Remain>
    App& ssl_file(T&&, Remain&&...);

    template <typename T>
    App& ssl(T&&);
#endif

  private:
    std::shared_ptr<boost::asio::io_context> io;
#ifdef BMCWEB_ENABLE_SSL
    uint16_t portUint = 443;
#else
    uint16_t portUint = 80;
#endif
    std::string bindaddrStr = "0.0.0.0";
    int socketFd = -1;
    Router router;

#ifdef BMCWEB_ENABLE_SSL
    std::unique_ptr<ssl_server_t> sslServer;
#else
    std::unique_ptr<server_t> server;
#endif
};
}