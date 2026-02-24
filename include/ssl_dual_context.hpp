#pragma once

#include "logging.hpp"
#include "sessions.hpp"

#include <openssl/ssl.h>

#include <boost/asio/ssl/context.hpp>

#include <bit>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace ensuressl
{

// Forward declarations
struct SslContextPair;
class SslContextExDataIndex;

// Constants
constexpr size_t sniMinLength = 5;

/**
 * @brief Holds both HTTPS and mTLS SSL contexts
 *
 * This structure maintains raw pointers to both contexts.
 * Memory is managed through RAII - the destructor cleans up both contexts.
 */
struct SslContextPair
{
    boost::asio::ssl::context* httpsCtx = nullptr;
    boost::asio::ssl::context* mtlsCtx = nullptr;

    ~SslContextPair()
    {
        delete httpsCtx;
        delete mtlsCtx;
    }

    // Prevent copying
    SslContextPair(const SslContextPair&) = delete;
    SslContextPair& operator=(const SslContextPair&) = delete;

    // Allow moving
    SslContextPair(SslContextPair&&) = default;
    SslContextPair& operator=(SslContextPair&&) = default;

    SslContextPair() = default;
};

/**
 * @brief Manages OpenSSL ex_data index for storing context pair
 *
 * Uses RAII to allocate and free the ex_data index.
 * The index is used to store SslContextPair pointer in SSL_CTX.
 */
class SslContextExDataIndex
{
  public:
    SslContextExDataIndex() :
        index(SSL_CTX_get_ex_new_index(0, nullptr, nullptr, nullptr, nullptr))
    {}

    ~SslContextExDataIndex() = default;

    // Delete copy/move operations
    SslContextExDataIndex(const SslContextExDataIndex&) = delete;
    SslContextExDataIndex& operator=(const SslContextExDataIndex&) = delete;
    SslContextExDataIndex(SslContextExDataIndex&&) = delete;
    SslContextExDataIndex& operator=(SslContextExDataIndex&&) = delete;

    [[nodiscard]] int get() const
    {
        return index;
    }

  private:
    int index;
};

/**
 * @brief Get the global ex_data index
 *
 * @return Reference to the singleton ex_data index
 */
inline SslContextExDataIndex& getExDataIndex()
{
    static SslContextExDataIndex index;
    return index;
}

/**
 * @brief Read a 16-bit big-endian value from buffer
 */
inline uint16_t sniReadUint16(std::span<const uint8_t> data, size_t offset)
{
    if (offset + 2 > data.size())
    {
        return 0;
    }
    return static_cast<uint16_t>((data[offset] << 8) | data[offset + 1]);
}

/**
 * @brief Read an 8-bit value from buffer
 */
inline uint8_t sniReadUint8(std::span<const uint8_t> data, size_t offset)
{
    if (offset >= data.size())
    {
        return 0;
    }
    return data[offset];
}

/**
 * @brief Extract SNI hostname from TLS Client Hello
 *
 * Parses the TLS Client Hello message to extract the Server Name Indication
 * (SNI) hostname. This is used to determine which SSL context to use.
 *
 * @param ssl The SSL connection object
 * @return The SNI hostname, or empty string if not found or on error
 */
inline std::string mtlsSslGetServername(SSL* ssl)
{
    size_t remaining = 0;
    const uint8_t* data = nullptr;

    if (SSL_client_hello_get0_ext(ssl, TLSEXT_TYPE_server_name, &data,
                                  &remaining) == 0)
    {
        return "";
    }

    if (remaining < sniMinLength)
    {
        return "";
    }

    // Use span for safe buffer access
    std::span<const uint8_t> sniData(data, remaining);
    size_t pos = 0;

    // Parse Server Name List Length (2 bytes)
    uint16_t listLength = sniReadUint16(sniData, pos);
    pos += 2;

    if (static_cast<size_t>(listLength) + 2 > sniData.size() || listLength < 3)
    {
        return "";
    }

    // Parse Name Type (1 byte) - should be 0 for hostname
    uint8_t nameType = sniReadUint8(sniData, pos);
    pos += 1;

    if (nameType != 0)
    {
        return "";
    }

    // Parse Hostname Length (2 bytes)
    uint16_t hostnameLength = sniReadUint16(sniData, pos);
    pos += 2;

    if (pos + hostnameLength > sniData.size() || hostnameLength == 0)
    {
        return "";
    }

    // Extract hostname
    return {std::bit_cast<const char*>(&sniData[pos]), hostnameLength};
}

/**
 * @brief Get mTLS certificate
 *
 * @param filepath Path to certificate file
 * @return Certificate content
 */
inline std::string ensureMtlsCertificate()
{
    namespace fs = std::filesystem;
    fs::path certFile = fs::path("/etc/ssl/certs/https") / "server_bmc.pem";

    std::error_code ec;
    if (!fs::exists(certFile, ec))
    {
        BMCWEB_LOG_ERROR(
            "mTLS certificate not found: {}. "
            "Certificate must be pre-provisioned for mTLS support.",
            certFile.string());
        return "";
    }
    std::string sslPemFile(certFile);
    return sslPemFile;
}

/**
 * @brief Switch SSL context while preserving verify callback
 *
 * @param ssl The SSL connection object
 * @param newCtx The new SSL context to switch to
 * @param strict If true ssl_verify_peer else cert verify
 * @param ctxName name of context https or mtls
 * @return void
 */

inline static void switchSslContext(SSL* ssl, SSL_CTX* newCtx, bool strict,
                                    const char* ctxName)
{
    if (ssl == nullptr || newCtx == nullptr)
    {
        BMCWEB_LOG_ERROR("Invalid parameters to switchSslContext");
        return;
    }

    BMCWEB_LOG_DEBUG("Switching to {} context (strict={})", ctxName, strict);

    // Switch to new context
    int (*savedCallback)(int, X509_STORE_CTX*) = SSL_get_verify_callback(ssl);

    SSL_set_SSL_CTX(ssl, newCtx);
    int verifyMode = strict
                         ? (SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT)
                         : SSL_VERIFY_PEER;
    SSL_set_verify(ssl, verifyMode, savedCallback);
    BMCWEB_LOG_DEBUG("Set verify mode to 0x{:x} switchSslContext", verifyMode);
}

/**
 * @brief Client Hello callback for SNI-based context switching
 *
 * This callback is invoked during the TLS handshake when the Client Hello
 * message is received. It examines the SNI hostname and switches to the
 * mTLS context if the hostname matches kMtlsSniHostname.
 *
 * @param ssl The SSL connection object
 * @param al Alert value (unused)
 * @param arg User argument (unused)
 * @return SSL_CLIENT_HELLO_SUCCESS on success, SSL_CLIENT_HELLO_ERROR on
 * failure
 */
inline static int clientHelloCallback(SSL* ssl, int* /*al*/, void* /*arg*/)
{
    BMCWEB_LOG_DEBUG("clientHelloCallback ");
    if (ssl == nullptr)
    {
        return SSL_CLIENT_HELLO_ERROR;
    }
    // Get context pair from ex_data
    SSL_CTX* currentCtx = SSL_get_SSL_CTX(ssl);

    auto* ctxPair = static_cast<SslContextPair*>(
        SSL_CTX_get_ex_data(currentCtx, getExDataIndex().get()));

    if (ctxPair == nullptr)
    {
        BMCWEB_LOG_ERROR("Failed to get context pair from ex_data");
        return SSL_CLIENT_HELLO_ERROR;
    }

    // Get SNI hostname using mtlsSslGetServername
    std::string hostname = mtlsSslGetServername(ssl);

    if (!hostname.empty())
    {
        BMCWEB_LOG_DEBUG("clientHelloCallback SNI Hostname: {}", hostname);

        // Switch to mTLS context if SNI matches
        if (hostname == kMtlsSniHostname)
        {
            if (ctxPair->mtlsCtx != nullptr &&
                (ctxPair->mtlsCtx->native_handle() != nullptr))
            {
                BMCWEB_LOG_DEBUG(
                    "clientHelloCallback Setting mtls context for client cert");
                switchSslContext(ssl, ctxPair->mtlsCtx->native_handle(), true,
                                 "mtlsCtx");
                return SSL_CLIENT_HELLO_SUCCESS;
            }
            BMCWEB_LOG_DEBUG(
                "clientHelloCallback NULL NO mtls context for client cert");
        }
    }
    // Default: HTTPS context
    const persistent_data::AuthConfigMethods& c =
        persistent_data::SessionStore::getInstance().getAuthMethodsConfig();
    if ((ctxPair->httpsCtx != nullptr) &&
        (ctxPair->httpsCtx->native_handle() != nullptr))
    {
        BMCWEB_LOG_DEBUG("Setting https context for tlsStrict {}", c.tlsStrict);
        switchSslContext(ssl, ctxPair->httpsCtx->native_handle(), c.tlsStrict,
                         "httpsCtx");

        return SSL_CLIENT_HELLO_SUCCESS;
    }
    BMCWEB_LOG_DEBUG(" NULL https context for tlsStrict {}", c.tlsStrict);
    return SSL_CLIENT_HELLO_SUCCESS;
}

/**
 * @brief Register ex_data and client hello callback for dual contexts
 *
 * Sets up the context pair pointer in ex_data and registers the
 * client hello callback for SNI-based context switching.
 *
 * @param ctxPair Pointer to the context pair
 */
inline void registerDualContextCallbacks(SslContextPair* ctxPair)
{
    if ((ctxPair == nullptr) || (ctxPair->httpsCtx == nullptr))
    {
        BMCWEB_LOG_ERROR(
            "Invalid context pair in registerDualContextCallbacks");
        return;
    }

    SSL_CTX* rawHttpsCtx = ctxPair->httpsCtx->native_handle();
    SSL_CTX* rawMtlsCtx = (ctxPair->mtlsCtx != nullptr)
                              ? ctxPair->mtlsCtx->native_handle()
                              : nullptr;

    // Register ex_data and client hello callback
    SSL_CTX_set_ex_data(rawHttpsCtx, getExDataIndex().get(), ctxPair);
    SSL_CTX_set_client_hello_cb(rawHttpsCtx, clientHelloCallback, nullptr);

    if (rawMtlsCtx != nullptr)
    {
        SSL_CTX_set_ex_data(rawMtlsCtx, getExDataIndex().get(), ctxPair);
        SSL_CTX_set_client_hello_cb(rawMtlsCtx, clientHelloCallback, nullptr);
    }
}

} // namespace ensuressl
