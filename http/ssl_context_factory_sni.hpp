#pragma once

#include "logging.hpp"
#include "mutual_tls.hpp"
#include "ssl_key_handler.hpp"

#include <openssl/ssl.h>

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ssl/context.hpp>

#include <filesystem>
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

// Callable object for SNI context factory with state
struct SniContextFactoryState
{
    std::function<bool(const std::string&)> sniChecker;
    std::string mtlsCertFile;
    std::string mtlsKeyFile;
    std::string mtlsTrustStore;
    mutable std::optional<boost::asio::ssl::context> clientAuthContext;

    SniContextFactoryState(std::function<bool(const std::string&)> sniCheckerIn,
                           std::string mtlsCertFileIn,
                           std::string mtlsKeyFileIn,
                           std::string mtlsTrustStoreIn) :
        sniChecker(std::move(sniCheckerIn)),
        mtlsCertFile(std::move(mtlsCertFileIn)),
        mtlsKeyFile(std::move(mtlsKeyFileIn)),
        mtlsTrustStore(std::move(mtlsTrustStoreIn))
    {}

    // Copy constructor - context cannot be copied, so reset it
    SniContextFactoryState(const SniContextFactoryState& other) :
        sniChecker(other.sniChecker), mtlsCertFile(other.mtlsCertFile),
        mtlsKeyFile(other.mtlsKeyFile), mtlsTrustStore(other.mtlsTrustStore),
        clientAuthContext(std::nullopt)
    {}

    // Copy assignment operator
    SniContextFactoryState& operator=(const SniContextFactoryState& other)
    {
        if (this != &other)
        {
            sniChecker = other.sniChecker;
            mtlsCertFile = other.mtlsCertFile;
            mtlsKeyFile = other.mtlsKeyFile;
            mtlsTrustStore = other.mtlsTrustStore;
            clientAuthContext = std::nullopt;
        }
        return *this;
    }

    // Move constructor - move the context
    SniContextFactoryState(SniContextFactoryState&& other) noexcept :
        sniChecker(std::move(other.sniChecker)),
        mtlsCertFile(std::move(other.mtlsCertFile)),
        mtlsKeyFile(std::move(other.mtlsKeyFile)),
        mtlsTrustStore(std::move(other.mtlsTrustStore)),
        clientAuthContext(std::move(other.clientAuthContext))
    {}

    // Move assignment operator
    SniContextFactoryState& operator=(SniContextFactoryState&& other) noexcept
    {
        if (this != &other)
        {
            sniChecker = std::move(other.sniChecker);
            mtlsCertFile = std::move(other.mtlsCertFile);
            mtlsKeyFile = std::move(other.mtlsKeyFile);
            mtlsTrustStore = std::move(other.mtlsTrustStore);
            clientAuthContext = std::move(other.clientAuthContext);
        }
        return *this;
    }

    // Operator() to make this a callable object
    std::shared_ptr<boost::asio::ssl::context> operator()() const
    {
        clientAuthContext = std::nullopt;

        // Create primary SSL context (default/non-mTLS) using ensuressl
        auto primaryCtx = ensuressl::getSslServerContext();

        // Set up SNI callback to switch contexts based on hostname
        SSL_CTX_set_tlsext_servername_callback(primaryCtx->native_handle(),
                                               sniCallbackStatic);
        SSL_CTX_set_tlsext_servername_arg(
            primaryCtx->native_handle(),
            const_cast<SniContextFactoryState*>(this));

        return primaryCtx;
    }

    // Verify all required mTLS files exist before any Boost SSL API is called.
    // boost::throw_exception() is overridden in this project to call
    // std::terminate() directly, so missing files must be detected here rather
    // than caught as exceptions.
    bool mtlsFilesExist() const
    {
        if (!std::filesystem::exists(mtlsCertFile))
        {
            BMCWEB_LOG_ERROR(
                "mTLS cert file not found, skipping client auth context: {}",
                mtlsCertFile);
            return false;
        }
        if (!std::filesystem::exists(mtlsKeyFile))
        {
            BMCWEB_LOG_ERROR(
                "mTLS key file not found, skipping client auth context: {}",
                mtlsKeyFile);
            return false;
        }
        if (!std::filesystem::exists(mtlsTrustStore))
        {
            BMCWEB_LOG_ERROR(
                "mTLS trust store not found, skipping client auth context: {}",
                mtlsTrustStore);
            return false;
        }
        return true;
    }

    // Create and configure client auth context with certificate verification.
    bool createClientAuthContext() const
    {
        if (clientAuthContext)
        {
            return true;
        }

        if (!mtlsFilesExist())
        {
            return false;
        }

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

    // Static SNI callback wrapper — must not throw (C callback boundary)
    static int sniCallbackStatic(SSL* ssl, int* ad, void* arg) noexcept
    {
        try
        {
            auto* state = static_cast<SniContextFactoryState*>(arg);
            if (state != nullptr)
            {
                return state->sniCallback(ssl, ad);
            }
        }
        catch (const std::exception& e)
        {
            BMCWEB_LOG_ERROR("SNI callback exception: {}", e.what());
        }
        catch (...)
        {
            BMCWEB_LOG_ERROR("SNI callback: unknown exception");
        }
        return SSL_TLSEXT_ERR_OK;
    }

    // SNI callback function for switching between TLS and mTLS contexts
    int sniCallback(SSL* ssl, int* /*ad*/) const
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

        // Use the sniChecker function to determine if we should switch to mTLS
        if (sniChecker(sni) && createClientAuthContext())
        {
            BMCWEB_LOG_INFO("SNI: Switching to mTLS context for {}", sni);

            SSL_set_SSL_CTX(ssl, clientAuthContext->native_handle());

            // After switching context, explicitly set verify mode and callback
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
};

} // namespace bmcweb
