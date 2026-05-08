#pragma once

#include "logging.hpp"
#include "mutual_tls.hpp"
#include "ssl_context_factory.hpp"

#include <openssl/ssl.h>

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ssl/context.hpp>

#include <functional>
#include <memory>
#include <string>

namespace bmcweb
{

inline int tlsVerifyCallback([[maybe_unused]] int preverified,
                             [[maybe_unused]] X509_STORE_CTX* ctx)
{
    BMCWEB_LOG_DEBUG("tlsVerifyCallback called with preverified {}",
                     preverified);

    if (!preverified)
    {
        BMCWEB_LOG_ERROR("mTLS: Client certificate verification failed");
    }

    return preverified;
}

// Create and configure createClientAuthContext context with certificate
// verification
inline bool createClientAuthContext()
{
    if (clientAuthContext)
    {
        return true;
    }
    try
    {
        clientAuthContext.emplace(boost::asio::ssl::context::tls_server);
        clientAuthContext->set_options(
            boost::asio::ssl::context::default_workarounds |
            boost::asio::ssl::context::no_sslv2 |
            boost::asio::ssl::context::no_sslv3 |
            boost::asio::ssl::context::no_tlsv1 |
            boost::asio::ssl::context::no_tlsv1_1 |
            boost::asio::ssl::context::single_dh_use);

        BMCWEB_LOG_INFO("Loading certificate from: {}", mtlsCertFile);
        clientAuthContext->use_certificate_chain_file(mtlsCertFile);

        BMCWEB_LOG_INFO("Loading private key from: {}", mtlsKeyFile);
        clientAuthContext->use_private_key_file(mtlsKeyFile,
                                                boost::asio::ssl::context::pem);

        BMCWEB_LOG_INFO("Loading trust store from: {}", mtlsTrustStore);
        clientAuthContext->add_verify_path(mtlsTrustStore);

        clientAuthContext->set_verify_mode(
            boost::asio::ssl::verify_peer |
            boost::asio::ssl::verify_fail_if_no_peer_cert);

        BMCWEB_LOG_INFO("Client auth context created successfully");
        return true;
    }
    catch (const std::exception& e)
    {
        BMCWEB_LOG_ERROR("Exception creating client auth context: {}",
                         e.what());
        clientAuthContext = std::nullopt;
        return false;
    }
    catch (...)
    {
        BMCWEB_LOG_ERROR("Unknown exception creating client auth context");
        clientAuthContext = std::nullopt;
        return false;
    }
}

// Implementation of SSL context factory with SNI-based mTLS switching
class SslContextFactoryWithSni : public ISslContextFactory
{
  public:
    // Function type for SNI name checking
    using SniCheckerFunc = std::function<bool(const std::string&)>;

    explicit SslContextFactoryWithSni(
        SniCheckerFunc sniCheckerIn, const std::string& mtlsCertFile,
        const std::string& mtlsKeyFile, const std::string& mtlsTrustStore) :
        sniChecker(std::move(sniCheckerIn)), mtlsCertFile(mtlsCertFile),
        mtlsKeyFile(mtlsKeyFile), mtlsTrustStore(mtlsTrustStore)
    {}

    std::shared_ptr<boost::asio::ssl::context> createContext() override
    {
        clientAuthContext = std::nullopt;
        return createSslContextWithSniImpl();
    }

  private:
    SniCheckerFunc sniChecker;
    std::string mtlsCertFile;
    std::string mtlsKeyFile;
    std::string mtlsTrustStore;
    std::optional<boost::asio::ssl::context> clientAuthContext;

    // Static SNI callback wrapper that delegates to member function
    static int sniCallbackStatic(SSL* ssl, int* ad, void* arg)
    {
        auto* factory = static_cast<SslContextFactoryWithSni*>(arg);
        if (factory != nullptr)
        {
            return factory->sniCallback(ssl, ad);
        }
        return SSL_TLSEXT_ERR_OK;
    }

    // Member SNI callback function for switching between TLS and mTLS contexts
    int sniCallback(SSL* ssl, int* /*ad*/)
    {
        const char* servername =
            SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);

        if (servername == nullptr)
        {
            BMCWEB_LOG_DEBUG("SNI: No server name provided");
            return SSL_TLSEXT_ERR_OK;
        }

        std::string sni(servername);
        BMCWEB_LOG_DEBUG("SNI: Received server name: {}", sni);

        if (!sniChecker)
        {
            BMCWEB_LOG_DEBUG("SNI: No SNI checker function configured");
            return SSL_TLSEXT_ERR_OK;
        }

        // Use the checker function to determine if we should switch to mTLS
        if (sniChecker(sni) && createClientAuthContext())
        {
            BMCWEB_LOG_INFO("SNI: Switching to mTLS context for {}", sni);

            SSL_set_SSL_CTX(ssl, clientAuthContext->native_handle());

            // After switching context, explicitly set verify mode and
            // callback The verify callback is NOT automatically inherited from
            // the context
            SSL_set_verify(ssl,
                           SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                           tlsVerifyCallback);

            BMCWEB_LOG_INFO(
                "SNI: Set verify mode and callback on SSL object for mTLS");
            BMCWEB_LOG_DEBUG(
                "SNI: Switched to mTLS context and configured callbacks");
            return SSL_TLSEXT_ERR_OK;
        }

        BMCWEB_LOG_DEBUG("SNI: Using standard TLS for {}", sni);
        return SSL_TLSEXT_ERR_OK;
    }

    std::shared_ptr<boost::asio::ssl::context> createSslContextWithSniImpl()
    {
        // Create primary SSL context (default/non-mTLS) using ensuressl
        auto primaryCtx = ensuressl::getSslServerContext();

        // Set up SNI callback to switch contexts based on hostname
        // mTLS context will be created on-demand in the SNI callback
        SSL_CTX_set_tlsext_servername_callback(primaryCtx->native_handle(),
                                               sniCallbackStatic);
        SSL_CTX_set_tlsext_servername_arg(primaryCtx->native_handle(), this);

        return primaryCtx;
    }
};

} // namespace bmcweb
