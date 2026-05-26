// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "ssl_key_handler.hpp"

#include "bmcweb_config.h"

#include "dbus_utility.hpp"
#include "forward_unauthorized.hpp"
#include "logging.hpp"
#include "ossl_random.hpp"
#include "ossl_wrappers.hpp"
#include "sessions.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/verify_mode.hpp>
#include <boost/beast/core/file_base.hpp>
#include <boost/beast/core/file_posix.hpp>
#include <boost/system/error_code.hpp>

#include <array>
#include <chrono>
#include <format>
#include <string_view>

extern "C"
{
#include <nghttp2/nghttp2.h>
#include <openssl/obj_mac.h>
#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <openssl/types.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
}

#include <bit>
#include <cstddef>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <system_error>
#include <utility>

namespace ensuressl
{

// Mozilla intermediate TLS 1.2 cipher suites v5.7.
// Sourced from: https://ssl-config.mozilla.org/guidelines/5.7.json
constexpr std::string_view mozillaIntermediateCiphers =
    "ECDHE-ECDSA-AES128-GCM-SHA256:"
    "ECDHE-RSA-AES128-GCM-SHA256:"
    "ECDHE-ECDSA-AES256-GCM-SHA384:"
    "ECDHE-RSA-AES256-GCM-SHA384:"
    "ECDHE-ECDSA-CHACHA20-POLY1305:"
    "ECDHE-RSA-CHACHA20-POLY1305:"
    "DHE-RSA-AES128-GCM-SHA256:"
    "DHE-RSA-AES256-GCM-SHA384:"
    "DHE-RSA-CHACHA20-POLY1305";

// Mozilla TLS 1.3 ciphersuites v5.7 (identical in the intermediate and modern
// profiles).
constexpr std::string_view mozillaModernCiphersuites =
    "TLS_AES_128_GCM_SHA256:"
    "TLS_AES_256_GCM_SHA384:"
    "TLS_CHACHA20_POLY1305_SHA256";

// Mozilla recommended elliptic curve groups v5.7 (shared between profiles).
constexpr std::string_view mozillaCurves = "X25519:prime256v1:secp384r1";

constexpr std::array<unsigned char, 6> sessionIdContext = {
    'b', 'm', 'c', 'w', 'e', 'b'};

// Trust chain related errors.`
static bool isTrustChainError(int errnum)
{
    return (errnum == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT) ||
           (errnum == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN) ||
           (errnum == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY) ||
           (errnum == X509_V_ERR_CERT_UNTRUSTED) ||
           (errnum == X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE);
}

static bool validateCertificate(OpenSSLX509& cert)
{
    // Create an empty X509_STORE structure for certificate validation.
    OpenSSLX509Store x509Store;

    // Load Certificate file into the X509 structure.
    OpenSSLX509StoreCTX storeCtx;

    int errCode = storeCtx.init(x509Store, cert);
    if (errCode != 1)
    {
        BMCWEB_LOG_ERROR("Error occurred during X509_STORE_CTX_init call");
        return false;
    }

    errCode = storeCtx.verifyCert();
    if (errCode == 1)
    {
        BMCWEB_LOG_INFO("Certificate verification is success");
        return true;
    }
    if (errCode == 0)
    {
        errCode = storeCtx.getError();
        if (isTrustChainError(errCode))
        {
            BMCWEB_LOG_DEBUG("Ignoring Trust Chain error. Reason: {}",
                             X509_verify_cert_error_string(errCode));
            return true;
        }
        BMCWEB_LOG_ERROR("Certificate verification failed. Reason: {}",
                         X509_verify_cert_error_string(errCode));
        return false;
    }

    BMCWEB_LOG_ERROR(
        "Error occurred during X509_verify_cert call. ErrorCode: {}", errCode);
    return false;
}

std::string verifyOpensslKeyCert(const std::string& filepath)
{
    BMCWEB_LOG_INFO("Checking certs in file {}", filepath);
    boost::beast::file_posix file;
    boost::system::error_code ec;
    file.open(filepath.c_str(), boost::beast::file_mode::read, ec);
    if (ec)
    {
        return "";
    }
    bool certValid = false;
    std::string fileContents;
    fileContents.resize(static_cast<size_t>(file.size(ec)), '\0');
    file.read(fileContents.data(), fileContents.size(), ec);
    if (ec)
    {
        BMCWEB_LOG_ERROR("Failed to read file");
        return "";
    }

    std::optional<OpenSSLEVPKey> pkey =
        OpenSSLEVPKey::readFromPrivateKey(fileContents);
    if (!pkey)
    {
        BMCWEB_LOG_ERROR("Failed to read private key");
        return "";
    }

    std::optional<OpenSSLX509> x509Obj =
        OpenSSLX509::loadFromPEMData(fileContents);
    if (!x509Obj)
    {
        BMCWEB_LOG_ERROR("Failed to load X509 certificate");
        return "";
    }

    certValid = validateCertificate(*x509Obj);
    if (!certValid)
    {
        return "";
    }
    return fileContents;
}

static std::optional<OpenSSLX509> loadCert(const std::string& filePath)
{
    std::filesystem::path certFilePath(filePath);
    std::optional<OpenSSLX509> x509Obj = OpenSSLX509::fromPEMFile(certFilePath);
    if (!x509Obj)
    {
        BMCWEB_LOG_ERROR("Failed to load cert {}", filePath);
        return std::nullopt;
    }
    return x509Obj;
}

static void installCertificate(const std::filesystem::path& certPath)
{
    dbus::utility::async_method_call(
        [certPath](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("Replace Certificate Fail..");
                return;
            }

            BMCWEB_LOG_INFO("Replace HTTPs Certificate Success, "
                            "remove temporary certificate file..");
            std::error_code ec2;
            std::filesystem::remove(certPath.c_str(), ec2);
            if (ec2)
            {
                BMCWEB_LOG_ERROR("Failed to remove certificate");
            }
        },
        "xyz.openbmc_project.Certs.Manager.Server.Https",
        "/xyz/openbmc_project/certs/server/https/1",
        "xyz.openbmc_project.Certs.Replace", "Replace", certPath.string());
}

void regenerateCertificateIfHostnameChanged(const std::string& filepath,
                                            const std::string& hostname)
{
    std::optional<OpenSSLX509> cert = ensuressl::loadCert(filepath);
    if (!cert)
    {
        BMCWEB_LOG_ERROR("Failed to load cert {}", filepath);
        return;
    }

    std::string cnValue = cert->getCommonName();
    if (cnValue.empty())
    {
        BMCWEB_LOG_ERROR("Failed to read subject name");
        return;
    }

    std::optional<OpenSSLEVPKey> pPubKey = cert->getPubKey();
    if (!pPubKey)
    {
        BMCWEB_LOG_ERROR("Failed to get public key");
        return;
    }
    int isSelfSigned = cert->verify(*pPubKey);

    BMCWEB_LOG_DEBUG(
        "Current HTTPs Certificate Subject CN: {}, New HostName: {}, isSelfSigned: {}",
        cnValue, hostname, isSelfSigned);

    std::string comment = cert->getComment();
    BMCWEB_LOG_DEBUG("x509Comment: {}", comment);

    if (ensuressl::x509Comment == comment && isSelfSigned == 1 &&
        cnValue != hostname)
    {
        BMCWEB_LOG_INFO(
            "Ready to generate new HTTPs certificate with subject cn: {}",
            hostname);

        std::string certData = ensuressl::generateSslCertificate(hostname);
        if (certData.empty())
        {
            BMCWEB_LOG_ERROR("Failed to generate cert");
            return;
        }
        ensuressl::writeCertificateToFile("/tmp/hostname_cert.tmp", certData);

        installCertificate("/tmp/hostname_cert.tmp");
    }
}

// Writes a certificate to a path, ignoring errors
void writeCertificateToFile(const std::string& filepath,
                            const std::string& certificate)
{
    boost::system::error_code ec;
    boost::beast::file_posix file;
    file.open(filepath.c_str(), boost::beast::file_mode::write, ec);
    if (!ec)
    {
        file.write(certificate.data(), certificate.size(), ec);
        // ignore result
    }
}

static std::string constructX509(const std::string& cn, OpenSSLEVPKey& pPrivKey)
{
    OpenSSLX509 x509Obj;

    // get a random number from the RNG for the certificate serial
    // number If this is not random, regenerating certs throws browser
    // errors
    bmcweb::OpenSSLGenerator gen;
    std::uniform_int_distribution<int> dis(1, std::numeric_limits<int>::max());
    x509Obj.setSerialNumber(dis(gen));

    // Cert is valid for 10 years
    x509Obj.setValidityPeriodFromNow(std::chrono::years(10));

    // set the public key to the key we just generated
    if (!x509Obj.setPubkey(pPrivKey))
    {
        return "";
    }

    x509Obj.setCountry("US");
    x509Obj.setOrganization("OpenBMC");
    x509Obj.setSubjectName(cn);

    // set the CSR options
    if (!x509Obj.setIssuerNameToSubject())
    {
        return "";
    }

    x509Obj.setVersion(2);
    x509Obj.addExt(NID_basic_constraints, "critical,CA:TRUE");
    std::string subjectAltName = std::format("DNS:{}", cn);
    x509Obj.addExt(NID_subject_alt_name, subjectAltName);
    x509Obj.addExt(NID_subject_key_identifier, "hash");
    x509Obj.addExt(NID_authority_key_identifier, "keyid");
    x509Obj.addExt(NID_key_usage, "digitalSignature, keyEncipherment");
    x509Obj.addExt(NID_ext_key_usage, "serverAuth");
    x509Obj.addExt(NID_netscape_comment, x509Comment);

    // Sign the certificate with our private key
    if (!x509Obj.sign(pPrivKey))
    {
        return "";
    }

    OpenSSLBIO bufio;

    if (!pPrivKey.pemWriteBioPrivateKey(bufio))
    {
        return "";
    }

    if (!x509Obj.pemWriteBioX509(bufio))
    {
        BMCWEB_LOG_ERROR("Failed to write X509.  Ignoring.");
    }

    std::string buffer(bufio.getMemData());

    BMCWEB_LOG_INFO("Cert size is {}", buffer.size());
    return buffer;
}

std::string generateSslCertificate(const std::string& commonName)
{
    BMCWEB_LOG_INFO("Generating EC key");
    std::optional<OpenSSLEVPKey> pPrivKey = OpenSSLEVPKeyCTX::createPKey();
    if (!pPrivKey)
    {
        BMCWEB_LOG_ERROR("Failed to create EC key");
        return "";
    }
    BMCWEB_LOG_INFO("Generating x509 Certificates");
    // Use this code to directly generate a certificate
    return constructX509(commonName, *pPrivKey);
}

std::string ensureOpensslKeyPresentAndValid(const std::string& filepath)
{
    std::string cert = verifyOpensslKeyCert(filepath);

    if (cert.empty())
    {
        BMCWEB_LOG_WARNING("Error in verifying signature, regenerating");
        cert = generateSslCertificate("testhost");
        if (cert.empty())
        {
            BMCWEB_LOG_ERROR("Failed to generate cert");
        }
        else
        {
            writeCertificateToFile(filepath, cert);
        }
    }
    return cert;
}

static std::string ensureCertificate()
{
    namespace fs = std::filesystem;
    // Cleanup older certificate file existing in the system
    fs::path oldcertPath = fs::path("/home/root/server.pem");
    std::error_code ec;
    fs::remove(oldcertPath, ec);
    // Ignore failure to remove;  File might not exist.

    fs::path certPath = "/etc/ssl/certs/https/";
    // if path does not exist create the path so that
    // self signed certificate can be created in the
    // path
    fs::path certFile = certPath / "server.pem";

    if (!fs::exists(certPath, ec))
    {
        fs::create_directories(certPath, ec);
    }
    BMCWEB_LOG_INFO("Building SSL Context file= {}", certFile.string());
    std::string sslPemFile(certFile);
    return ensuressl::ensureOpensslKeyPresentAndValid(sslPemFile);
}

static int nextProtoCallback(SSL* /*unused*/, const unsigned char** data,
                             unsigned int* len, void* /*unused*/)
{
    // First byte is the length.
    constexpr std::string_view h2 = "\x02h2";
    *data = std::bit_cast<const unsigned char*>(h2.data());
    *len = static_cast<unsigned int>(h2.size());
    return SSL_TLSEXT_ERR_OK;
}

static int alpnSelectProtoCallback(
    SSL* /*unused*/, const unsigned char** out, unsigned char* outlen,
    const unsigned char* in, unsigned int inlen, void* /*unused*/)
{
    int rv = nghttp2_select_alpn(out, outlen, in, inlen);
    if (rv == -1)
    {
        return SSL_TLSEXT_ERR_NOACK;
    }
    if (rv == 1)
    {
        BMCWEB_LOG_DEBUG("Selected HTTP2");
    }
    return SSL_TLSEXT_ERR_OK;
}

static bool getSslContext(boost::asio::ssl::context& mSslContext,
                          const std::string& sslPemFile)
{
    auto sslOptions =
        boost::asio::ssl::context::default_workarounds |
        boost::asio::ssl::context::no_sslv2 |
        boost::asio::ssl::context::no_sslv3 |
        boost::asio::ssl::context::single_dh_use |
        boost::asio::ssl::context::no_tlsv1 |
        boost::asio::ssl::context::no_tlsv1_1;
    if constexpr (BMCWEB_TLS_PROFILE == "modern")
    {
        // Mozilla modern is TLS 1.3 only.
        sslOptions |= boost::asio::ssl::context::no_tlsv1_2;
    }
    mSslContext.set_options(sslOptions);

    BMCWEB_LOG_DEBUG("Using default TrustStore location: {}", trustStorePath);
    mSslContext.add_verify_path(trustStorePath);

    if (!sslPemFile.empty())
    {
        boost::system::error_code ec;

        boost::asio::const_buffer buf(sslPemFile.data(), sslPemFile.size());
        mSslContext.use_certificate_chain(buf, ec);
        if (ec)
        {
            return false;
        }
        mSslContext.use_private_key(buf, boost::asio::ssl::context::pem, ec);
        if (ec)
        {
            BMCWEB_LOG_CRITICAL("Failed to open ssl pkey");
            return false;
        }
    }

    OpenSSLSSLCtx sslCtx(mSslContext.native_handle());


    if (!sslCtx.setCurves(mozillaCurves))
    {
        BMCWEB_LOG_ERROR("Error setting TLS curve list");
        return false;
    }

    std::string_view cipherList;
    if constexpr (BMCWEB_TLS_PROFILE == "intermediate")
    {
        cipherList = mozillaIntermediateCiphers;
        if (!sslCtx.setCipherList(cipherList))
        {
            return false;
        }
    }
    else
    {
        cipherList = mozillaModernCiphersuites;
        if (!sslCtx.setCiphersuites(mozillaModernCiphersuites))
        {
            BMCWEB_LOG_ERROR("Error setting ciphersuites");
            return false;
        }
    }

    return true;
}

std::shared_ptr<boost::asio::ssl::context> getSslServerContext()
{
    boost::asio::ssl::context sslCtx(boost::asio::ssl::context::tls_server);

    auto certFile = ensureCertificate();
    if (!getSslContext(sslCtx, certFile))
    {
        BMCWEB_LOG_CRITICAL("Couldn't get server context");
        return nullptr;
    }
    const persistent_data::AuthConfigMethods& c =
        persistent_data::SessionStore::getInstance().getAuthMethodsConfig();

    boost::asio::ssl::verify_mode mode = boost::asio::ssl::verify_none;
    if (c.tlsStrict)
    {
        BMCWEB_LOG_DEBUG("Setting verify peer and fail if no peer cert");
        mode |= boost::asio::ssl::verify_peer;
        mode |= boost::asio::ssl::verify_fail_if_no_peer_cert;
    }
    else if (!forward_unauthorized::hasWebuiRoute())
    {
        // This is a HACK
        // If the webui is installed, and TLSSTrict is false, we don't want to
        // force the mtls popup to occur, which would happen if we requested a
        // client cert by setting verify_peer. But, if the webui isn't
        // installed, we'd like clients to be able to optionally log in with
        // MTLS, which won't happen if we don't expose the MTLS client cert
        // request.  So, in this case detect if the webui is installed, and
        // only request peer authentication if it's not present.
        // This will likely need revisited in the future.
        BMCWEB_LOG_DEBUG("Setting verify peer only");
        mode |= boost::asio::ssl::verify_peer;
    }

    boost::system::error_code ec;
    sslCtx.set_verify_mode(mode, ec);
    if (ec)
    {
        BMCWEB_LOG_DEBUG("Failed to set verify mode {}", ec.message());
        return nullptr;
    }

    SSL_CTX_set_options(sslCtx.native_handle(), SSL_OP_NO_RENEGOTIATION);

    if constexpr (BMCWEB_HTTP2)
    {
        SSL_CTX_set_next_protos_advertised_cb(sslCtx.native_handle(),
                                              nextProtoCallback, nullptr);

        SSL_CTX_set_alpn_select_cb(sslCtx.native_handle(),
                                   alpnSelectProtoCallback, nullptr);
    }

    // Enable server-side in memory session caching, so they can be looked up
    // by ID.
    SSL_CTX_set_session_cache_mode(sslCtx.native_handle(), SSL_SESS_CACHE_BOTH);

    // Set session cache size to prevent session ID DoS attack
    SSL_CTX_sess_set_cache_size(sslCtx.native_handle(), 100);

    // Set the Session ID Context
    // OpenSSL REQUIRES this to be set for the server to support session
    // caching. It prevents sessions from one application context (e.g., a
    // different port/app) from being used in another.
    if (SSL_CTX_set_session_id_context(sslCtx.native_handle(),
                                       sessionIdContext.data(),
                                       sessionIdContext.size()) != 1)
    {
        BMCWEB_LOG_ERROR("Error setting session ID context");
        return nullptr;
    }

    return std::make_shared<boost::asio::ssl::context>(std::move(sslCtx));
}

std::optional<boost::asio::ssl::context> getSSLClientContext(
    VerifyCertificate verifyCertificate)
{
    namespace fs = std::filesystem;

    boost::asio::ssl::context sslCtx(boost::asio::ssl::context::tls_client);

    // NOTE, this path is temporary;  In the future it will need to change to
    // be set per subscription.  Do not rely on this.
    fs::path certPath = "/etc/ssl/certs/https/client.pem";
    std::string cert = verifyOpensslKeyCert(certPath);

    if (!getSslContext(sslCtx, cert))
    {
        return std::nullopt;
    }

    // Add a directory containing certificate authority files to be used
    // for performing verification.
    boost::system::error_code ec;
    sslCtx.set_default_verify_paths(ec);
    if (ec)
    {
        BMCWEB_LOG_ERROR("SSL context set_default_verify failed");
        return std::nullopt;
    }

    int mode = boost::asio::ssl::verify_peer;
    if (verifyCertificate == VerifyCertificate::NoVerify)
    {
        mode = boost::asio::ssl::verify_none;
    }

    // Verify the remote server's certificate
    sslCtx.set_verify_mode(mode, ec);
    if (ec)
    {
        BMCWEB_LOG_ERROR("SSL context set_verify_mode failed");
        return std::nullopt;
    }

    return {std::move(sslCtx)};
}

} // namespace ensuressl
