#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>

#include <optional>
#include "timer_queue.hpp"
#include "http_server_class_definition.hpp"
#include "app_class_definition.hpp"

#include <atomic>
#include <chrono>
#include <vector>

namespace crow
{

#ifdef BMCWEB_ENABLE_SSL
using ssl_context_t = boost::asio::ssl::context;
#endif

#ifdef BMCWEB_ENABLE_SSL

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