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

// Create and configure createClientAuthContext context with certificate verification
inline std::shared_ptr<boost::asio::ssl::context> 
createClientAuthContext(const std::string& certFile,
                        const std::string& keyFile,
                        const std::string& trustStorePath)
{
    try {
        auto clientAuthContext = std::make_shared<boost::asio::ssl::context>(
            boost::asio::ssl::context::tls_server);

        clientAuthContext->set_options(
            boost::asio::ssl::context::default_workarounds |
            boost::asio::ssl::context::no_sslv2 |
            boost::asio::ssl::context::no_sslv3 |
            boost::asio::ssl::context::no_tlsv1 |
            boost::asio::ssl::context::no_tlsv1_1 |
            boost::asio::ssl::context::single_dh_use);

        BMCWEB_LOG_INFO("Loading certificate from: {}", certFile);
        clientAuthContext->use_certificate_chain_file(certFile);
        
        BMCWEB_LOG_INFO("Loading private key from: {}", keyFile);
        clientAuthContext->use_private_key_file(keyFile, 
                                                boost::asio::ssl::context::pem);
        
        BMCWEB_LOG_INFO("Loading trust store from: {}", trustStorePath);
        clientAuthContext->add_verify_path(trustStorePath);
        
        clientAuthContext->set_verify_mode(boost::asio::ssl::verify_peer |
                                 boost::asio::ssl::verify_fail_if_no_peer_cert);

        BMCWEB_LOG_INFO("Client auth context created successfully");
        return clientAuthContext;
    }
    catch (const boost::system::system_error& e) {
        BMCWEB_LOG_ERROR("Boost system error: {}", e.what());
        BMCWEB_LOG_ERROR("  Error code: {}", e.code().value());
        BMCWEB_LOG_ERROR("  Error category: {}", e.code().category().name());
        BMCWEB_LOG_ERROR("  Error message: {}", e.code().message());
        return nullptr;  // Return nullptr instead of throwing
    }
    catch (const std::exception& e) {
        BMCWEB_LOG_ERROR("Exception creating client auth context: {}", e.what());
        return nullptr;
    }
    catch (...) {
        BMCWEB_LOG_ERROR("Unknown exception creating client auth context");
        return nullptr;
    }
}

// Implementation of SSL context factory with SNI-based mTLS switching
class SslContextFactoryWithSni : public ISslContextFactory
{
  public:
    // Function type for SNI name checking
    using SniCheckerFunc = std::function<bool(const std::string&)>;

    explicit SslContextFactoryWithSni(
        SniCheckerFunc sniCheckerIn, 
        const std::string& mtlsCertFile,    
        const std::string& mtlsKeyFile,     
        const std::string& mtlsTrustStore) :
        sniChecker(std::move(sniCheckerIn))
    {
        // Create context in constructor body to catch exceptions
        try {
            BMCWEB_LOG_INFO("Creating client auth context...");
            BMCWEB_LOG_INFO("  Cert file: {}", mtlsCertFile);
            BMCWEB_LOG_INFO("  Key file: {}", mtlsKeyFile);
            BMCWEB_LOG_INFO("  Trust store: {}", mtlsTrustStore);
            
            clientAuthContext = createClientAuthContext(mtlsCertFile, 
                                                        mtlsKeyFile, 
                                                        mtlsTrustStore);
            
            if (clientAuthContext) {
                BMCWEB_LOG_INFO("Client auth context created successfully");
            }
            else {
                BMCWEB_LOG_ERROR("Client auth context is null");
            }
        }
        catch (const boost::system::system_error& e) {
            BMCWEB_LOG_ERROR("Boost system error creating client auth context: {}", 
                             e.what());
            BMCWEB_LOG_ERROR("  Error code: {}", e.code().value());
            BMCWEB_LOG_ERROR("  Error message: {}", e.code().message());
        }
        catch (const std::exception& e) {
            BMCWEB_LOG_ERROR("Exception creating client auth context: {}", e.what());
        }
        catch (...) {
            BMCWEB_LOG_ERROR("Unknown exception creating client auth context");
        }
    }
    std::shared_ptr<boost::asio::ssl::context> createContext() override
    {
        return createSslContextWithSniImpl();
    }

  private:
    SniCheckerFunc sniChecker;
    std::shared_ptr<boost::asio::ssl::context> clientAuthContext;
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
        if (sniChecker(sni))
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
