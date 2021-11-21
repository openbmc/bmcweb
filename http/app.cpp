#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>

#include <optional>
#include "timer_queue.hpp"
#include "http_server_class_decl.hpp"
#include "app_class_decl.hpp"

#include <atomic>
#include <chrono>
#include <vector>

namespace crow
{

#ifdef BMCWEB_ENABLE_SSL
using ssl_context_t = boost::asio::ssl::context;
#endif

#ifdef BMCWEB_ENABLE_SSL

App::App(std::shared_ptr<boost::asio::io_context> ioIn) :
    io(std::move(ioIn))
{}

App::~App()
{
    this->stop();
}

App& App::socket(int existingSocket)
{
    socketFd = existingSocket;
    return *this;
}

App& App::port(std::uint16_t port)
{
    portUint = port;
    return *this;
}

App& App::bindaddr(std::string bindaddr)
{
    bindaddrStr = std::move(bindaddr);
    return *this;
}

void App::validate()
{
    router.validate();
}

void App::run()
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

void App::stop()
{
    io->stop();
}

void App::debugPrint()
{
    BMCWEB_LOG_DEBUG << "Routing:";
    router.debugPrint();
}

std::vector<const std::string*> App::getRoutes()
{
    const std::string root("");
    return router.getRoutes(root);
}
std::vector<const std::string*> App::getRoutes(const std::string& parent)
{
    return router.getRoutes(parent);
}

// #ifdef BMCWEB_ENABLE_SSL
App& App::sslFile(const std::string& crtFilename, const std::string& keyFilename)
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

App& App::sslFile(const std::string& pemFilename)
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

App& App::ssl(std::shared_ptr<boost::asio::ssl::context>&& ctx)
{
    sslContext = std::move(ctx);
    BMCWEB_LOG_INFO << "app::ssl context use_count="
                    << sslContext.use_count();
    return *this;
}



#else

#endif

}