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
#include <format>
#include <string_view>

extern "C"
{
#include <nghttp2/nghttp2.h>
#include <openssl/asn1.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/obj_mac.h>
#include <openssl/pem.h>
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

// Mozilla intermediate cipher suites v5.7
// Sourced from: https://ssl-config.mozilla.org/guidelines/5.7.json
constexpr const char* mozillaIntermediate =
    "ECDHE-ECDSA-AES128-GCM-SHA256:"
    "ECDHE-RSA-AES128-GCM-SHA256:"
    "ECDHE-ECDSA-AES256-GCM-SHA384:"
    "ECDHE-RSA-AES256-GCM-SHA384:"
    "ECDHE-ECDSA-CHACHA20-POLY1305:"
    "ECDHE-RSA-CHACHA20-POLY1305:"
    "DHE-RSA-AES128-GCM-SHA256:"
    "DHE-RSA-AES256-GCM-SHA384:"
    "DHE-RSA-CHACHA20-POLY1305";

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

    OpenSSLEVPKey pkey(fileContents);

    OpenSSLX509 x509Obj(fileContents);
    certValid = validateCertificate(x509Obj);

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

    const int maxKeySize = 256;
    std::array<char, maxKeySize> cnBuffer{};

    int cnLength = X509_NAME_get_text_by_NID(
        X509_get_subject_name(cert->get()), NID_commonName, cnBuffer.data(),
        cnBuffer.size());
    if (cnLength == -1)
    {
        BMCWEB_LOG_ERROR("Failed to read NID_commonName");
        return;
    }
    std::string_view cnValue(std::begin(cnBuffer),
                             static_cast<size_t>(cnLength));

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

    OpenSSLASN1String asn1(static_cast<ASN1_IA5STRING*>(
        X509_get_ext_d2i(cert->get(), NID_netscape_comment, nullptr, nullptr)));

    std::string_view comment = asn1.getAsString();
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

static std::string constructX509(const std::string& cn, EVP_PKEY* pPrivKey)
{
    std::string buffer;
    OpenSSLX509 x509Obj;
    X509* x509 = x509Obj.get();

    // get a random number from the RNG for the certificate serial
    // number If this is not random, regenerating certs throws browser
    // errors
    bmcweb::OpenSSLGenerator gen;
    std::uniform_int_distribution<int> dis(1, std::numeric_limits<int>::max());
    int serial = dis(gen);

    ASN1_INTEGER_set(X509_get_serialNumber(x509), serial);

    // not before this moment
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    // Cert is valid for 10 years
    X509_gmtime_adj(X509_get_notAfter(x509), 60L * 60L * 24L * 365L * 10L);

    // set the public key to the key we just generated
    X509_set_pubkey(x509, pPrivKey);

    // get the subject name
    X509_NAME* name = X509_get_subject_name(x509);

    std::array<unsigned char, 2> country = {'U', 'S'};
    std::array<unsigned char, 8> company = {'O', 'p', 'e', 'n', 'B', 'M', 'C'};
    const unsigned char* cnPtr = std::bit_cast<const unsigned char*>(cn.data());
    int cnLength = static_cast<int>(cn.size());

    X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, country.data(),
                               country.size(), -1, 0);
    X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, company.data(),
                               company.size(), -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, cnPtr, cnLength, -1,
                               0);
    // set the CSR options
    X509_set_issuer_name(x509, name);

    X509_set_version(x509, 2);
    x509Obj.addExt(NID_basic_constraints, "critical,CA:TRUE");
    std::string subjectAltName = std::format("DNS:{}", cn);
    x509Obj.addExt(NID_subject_alt_name, subjectAltName.c_str());
    x509Obj.addExt(NID_subject_key_identifier, "hash");
    x509Obj.addExt(NID_authority_key_identifier, "keyid");
    x509Obj.addExt(NID_key_usage, "digitalSignature, keyEncipherment");
    x509Obj.addExt(NID_ext_key_usage, "serverAuth");
    x509Obj.addExt(NID_netscape_comment, x509Comment);

    // Sign the certificate with our private key
    X509_sign(x509, pPrivKey, EVP_sha256());

    OpenSSLBIO bufio;

    int pkeyRet = PEM_write_bio_PrivateKey(bufio.get(), pPrivKey, nullptr,
                                           nullptr, 0, nullptr, nullptr);
    if (pkeyRet <= 0)
    {
        BMCWEB_LOG_ERROR("Failed to write pkey with code {}.  Ignoring.",
                         pkeyRet);
    }

    pkeyRet = PEM_write_bio_X509(bufio.get(), x509);
    if (pkeyRet <= 0)
    {
        BMCWEB_LOG_ERROR("Failed to write X509 with code {}.  Ignoring.",
                         pkeyRet);
    }
    buffer += bufio.getMemData();

    BMCWEB_LOG_INFO("Cert size is {}", buffer.size());
    return buffer;
}

static EVP_PKEY* createEcKey()
{
    EVP_PKEY* pKey = nullptr;

    // Create context for curve parameter generation.
    std::unique_ptr<EVP_PKEY_CTX, decltype(&::EVP_PKEY_CTX_free)> ctx{
        EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr), &::EVP_PKEY_CTX_free};
    if (!ctx)
    {
        return nullptr;
    }

    // Set up curve parameters.
    EVP_PKEY* params = nullptr;
    if ((EVP_PKEY_paramgen_init(ctx.get()) <= 0) ||
        (EVP_PKEY_CTX_set_ec_param_enc(ctx.get(), OPENSSL_EC_NAMED_CURVE) <=
         0) ||
        (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx.get(), NID_secp384r1) <=
         0) ||
        (EVP_PKEY_paramgen(ctx.get(), &params) <= 0))
    {
        return nullptr;
    }

    // Set up RAII holder for params.
    std::unique_ptr<EVP_PKEY, decltype(&::EVP_PKEY_free)> pparams{
        params, &::EVP_PKEY_free};

    // Set new context for key generation, using curve parameters.
    ctx.reset(EVP_PKEY_CTX_new_from_pkey(nullptr, params, nullptr));
    if (!ctx || (EVP_PKEY_keygen_init(ctx.get()) <= 0))
    {
        return nullptr;
    }

    // Generate key.
    if (EVP_PKEY_keygen(ctx.get(), &pKey) <= 0)
    {
        return nullptr;
    }

    return pKey;
}

std::string generateSslCertificate(const std::string& cn)
{
    BMCWEB_LOG_INFO("Generating new keys");

    std::string buffer;
    BMCWEB_LOG_INFO("Generating EC key");
    EVP_PKEY* pPrivKey = createEcKey();
    if (pPrivKey != nullptr)
    {
        BMCWEB_LOG_INFO("Generating x509 Certificates");
        // Use this code to directly generate a certificate
        buffer = constructX509(cn, pPrivKey);
    }

    EVP_PKEY_free(pPrivKey);

    return buffer;
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
    mSslContext.set_options(
        boost::asio::ssl::context::default_workarounds |
        boost::asio::ssl::context::no_sslv2 |
        boost::asio::ssl::context::no_sslv3 |
        boost::asio::ssl::context::single_dh_use |
        boost::asio::ssl::context::no_tlsv1 |
        boost::asio::ssl::context::no_tlsv1_1);

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

    if (SSL_CTX_set_cipher_list(mSslContext.native_handle(),
                                mozillaIntermediate) != 1)
    {
        BMCWEB_LOG_ERROR("Error setting cipher list");
        return false;
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

    if (SSL_CTX_set_cipher_list(sslCtx.native_handle(), mozillaIntermediate) !=
        1)
    {
        BMCWEB_LOG_ERROR("SSL_CTX_set_cipher_list failed");
        return std::nullopt;
    }

    return {std::move(sslCtx)};
}

} // namespace ensuressl
