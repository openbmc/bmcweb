#pragma once

#include "logging.hpp"
#include "random.hpp"

#include <openssl/bio.h>
#include <openssl/dh.h>
#include <openssl/dsa.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>

#include <boost/asio/ssl/context.hpp>

#include <optional>
#include <random>
#include <string>
#include <string_view>

namespace ensuressl
{
constexpr char const* x509Comment = "Generated from OpenBMC service";
const char* trustStorePath = "/etc/ssl/certs/authority";
static void initOpenssl();
static EVP_PKEY* createEcKey();

// Trust chain related errors.`
inline bool isTrustChainError(int errnum)
{
    return (errnum == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT) ||
           (errnum == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN) ||
           (errnum == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY) ||
           (errnum == X509_V_ERR_CERT_UNTRUSTED) ||
           (errnum == X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE);
}

inline bool validateCertificate(X509* const cert)
{
    // Create an empty X509_STORE structure for certificate validation.
    X509_STORE* x509Store = X509_STORE_new();
    if (x509Store == nullptr)
    {
        BMCWEB_LOG_ERROR << "Error occurred during X509_STORE_new call";
        return false;
    }

    // Load Certificate file into the X509 structure.
    X509_STORE_CTX* storeCtx = X509_STORE_CTX_new();
    if (storeCtx == nullptr)
    {
        BMCWEB_LOG_ERROR << "Error occurred during X509_STORE_CTX_new call";
        X509_STORE_free(x509Store);
        return false;
    }

    int errCode = X509_STORE_CTX_init(storeCtx, x509Store, cert, nullptr);
    if (errCode != 1)
    {
        BMCWEB_LOG_ERROR << "Error occurred during X509_STORE_CTX_init call";
        X509_STORE_CTX_free(storeCtx);
        X509_STORE_free(x509Store);
        return false;
    }

    errCode = X509_verify_cert(storeCtx);
    if (errCode == 1)
    {
        BMCWEB_LOG_INFO << "Certificate verification is success";
        X509_STORE_CTX_free(storeCtx);
        X509_STORE_free(x509Store);
        return true;
    }
    if (errCode == 0)
    {
        errCode = X509_STORE_CTX_get_error(storeCtx);
        X509_STORE_CTX_free(storeCtx);
        X509_STORE_free(x509Store);
        if (isTrustChainError(errCode))
        {
            BMCWEB_LOG_DEBUG << "Ignoring Trust Chain error. Reason: "
                             << X509_verify_cert_error_string(errCode);
            return true;
        }
        BMCWEB_LOG_ERROR << "Certificate verification failed. Reason: "
                         << X509_verify_cert_error_string(errCode);
        return false;
    }

    BMCWEB_LOG_ERROR
        << "Error occurred during X509_verify_cert call. ErrorCode: "
        << errCode;
    X509_STORE_CTX_free(storeCtx);
    X509_STORE_free(x509Store);
    return false;
}

inline bool verifyOpensslKeyCert(const std::string& filepath)
{
    bool privateKeyValid = false;
    bool certValid = false;

    std::cout << "Checking certs in file " << filepath << "\n";

    FILE* file = fopen(filepath.c_str(), "r");
    if (file != nullptr)
    {
        EVP_PKEY* pkey = PEM_read_PrivateKey(file, nullptr, nullptr, nullptr);
        if (pkey != nullptr)
        {
#if (OPENSSL_VERSION_NUMBER < 0x30000000L)
            RSA* rsa = EVP_PKEY_get1_RSA(pkey);
            if (rsa != nullptr)
            {
                std::cout << "Found an RSA key\n";
                if (RSA_check_key(rsa) == 1)
                {
                    privateKeyValid = true;
                }
                else
                {
                    std::cerr << "Key not valid error number "
                              << ERR_get_error() << "\n";
                }
                RSA_free(rsa);
            }
            else
            {
                EC_KEY* ec = EVP_PKEY_get1_EC_KEY(pkey);
                if (ec != nullptr)
                {
                    std::cout << "Found an EC key\n";
                    if (EC_KEY_check_key(ec) == 1)
                    {
                        privateKeyValid = true;
                    }
                    else
                    {
                        std::cerr << "Key not valid error number "
                                  << ERR_get_error() << "\n";
                    }
                    EC_KEY_free(ec);
                }
            }
#else
            EVP_PKEY_CTX* pkeyCtx =
                EVP_PKEY_CTX_new_from_pkey(nullptr, pkey, nullptr);

            if (pkeyCtx == nullptr)
            {
                std::cerr << "Unable to allocate pkeyCtx " << ERR_get_error()
                          << "\n";
            }
            else if (EVP_PKEY_check(pkeyCtx) == 1)
            {
                privateKeyValid = true;
            }
            else
            {

                std::cerr << "Key not valid error number " << ERR_get_error()
                          << "\n";
            }
#endif

            if (privateKeyValid)
            {
                // If the order is certificate followed by key in input file
                // then, certificate read will fail. So, setting the file
                // pointer to point beginning of file to avoid certificate and
                // key order issue.
                fseek(file, 0, SEEK_SET);

                X509* x509 = PEM_read_X509(file, nullptr, nullptr, nullptr);
                if (x509 == nullptr)
                {
                    std::cout << "error getting x509 cert " << ERR_get_error()
                              << "\n";
                }
                else
                {
                    certValid = validateCertificate(x509);
                    X509_free(x509);
                }
            }

#if (OPENSSL_VERSION_NUMBER > 0x30000000L)
            EVP_PKEY_CTX_free(pkeyCtx);
#endif
            EVP_PKEY_free(pkey);
        }
        fclose(file);
    }
    return certValid;
}

inline X509* loadCert(const std::string& filePath)
{
    BIO* certFileBio = BIO_new_file(filePath.c_str(), "rb");
    if (certFileBio == nullptr)
    {
        BMCWEB_LOG_ERROR << "Error occured during BIO_new_file call, "
                         << "FILE= " << filePath;
        return nullptr;
    }

    X509* cert = X509_new();
    if (cert == nullptr)
    {
        BMCWEB_LOG_ERROR << "Error occured during X509_new call, "
                         << ERR_get_error();
        BIO_free(certFileBio);
        return nullptr;
    }

    if (PEM_read_bio_X509(certFileBio, &cert, nullptr, nullptr) == nullptr)
    {
        BMCWEB_LOG_ERROR << "Error occured during PEM_read_bio_X509 call, "
                         << "FILE= " << filePath;

        BIO_free(certFileBio);
        X509_free(cert);
        return nullptr;
    }
    BIO_free(certFileBio);
    return cert;
}

inline int addExt(X509* cert, int nid, const char* value)
{
    X509_EXTENSION* ex = nullptr;
    X509V3_CTX ctx{};
    X509V3_set_ctx(&ctx, cert, cert, nullptr, nullptr, 0);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    ex = X509V3_EXT_conf_nid(nullptr, &ctx, nid, const_cast<char*>(value));
    if (ex == nullptr)
    {
        BMCWEB_LOG_ERROR << "Error: In X509V3_EXT_conf_nidn: " << value;
        return -1;
    }
    X509_add_ext(cert, ex, -1);
    X509_EXTENSION_free(ex);
    return 0;
}

inline void generateSslCertificate(const std::string& filepath,
                                   const std::string& cn)
{
    FILE* pFile = nullptr;
    std::cout << "Generating new keys\n";
    initOpenssl();

    std::cerr << "Generating EC key\n";
    EVP_PKEY* pPrivKey = createEcKey();
    if (pPrivKey != nullptr)
    {
        std::cerr << "Generating x509 Certificate\n";
        // Use this code to directly generate a certificate
        X509* x509 = X509_new();
        if (x509 != nullptr)
        {
            // get a random number from the RNG for the certificate serial
            // number If this is not random, regenerating certs throws broswer
            // errors
            bmcweb::OpenSSLGenerator gen;
            std::uniform_int_distribution<int> dis(
                1, std::numeric_limits<int>::max());
            int serial = dis(gen);

            ASN1_INTEGER_set(X509_get_serialNumber(x509), serial);

            // not before this moment
            X509_gmtime_adj(X509_get_notBefore(x509), 0);
            // Cert is valid for 10 years
            X509_gmtime_adj(X509_get_notAfter(x509),
                            60L * 60L * 24L * 365L * 10L);

            // set the public key to the key we just generated
            X509_set_pubkey(x509, pPrivKey);

            // get the subject name
            X509_NAME* name = X509_get_subject_name(x509);

            using x509String = const unsigned char;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            x509String* country = reinterpret_cast<x509String*>("US");
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            x509String* company = reinterpret_cast<x509String*>("OpenBMC");
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            x509String* cnStr = reinterpret_cast<x509String*>(cn.c_str());

            X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, country, -1, -1,
                                       0);
            X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, company, -1, -1,
                                       0);
            X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, cnStr, -1, -1,
                                       0);
            // set the CSR options
            X509_set_issuer_name(x509, name);

            X509_set_version(x509, 2);
            addExt(x509, NID_basic_constraints, ("critical,CA:TRUE"));
            addExt(x509, NID_subject_alt_name, ("DNS:" + cn).c_str());
            addExt(x509, NID_subject_key_identifier, ("hash"));
            addExt(x509, NID_authority_key_identifier, ("keyid"));
            addExt(x509, NID_key_usage, ("digitalSignature, keyEncipherment"));
            addExt(x509, NID_ext_key_usage, ("serverAuth"));
            addExt(x509, NID_netscape_comment, (x509Comment));

            // Sign the certificate with our private key
            X509_sign(x509, pPrivKey, EVP_sha256());

            pFile = fopen(filepath.c_str(), "wt");

            if (pFile != nullptr)
            {
                PEM_write_PrivateKey(pFile, pPrivKey, nullptr, nullptr, 0,
                                     nullptr, nullptr);

                PEM_write_X509(pFile, x509);
                fclose(pFile);
                pFile = nullptr;
            }

            X509_free(x509);
        }

        EVP_PKEY_free(pPrivKey);
        pPrivKey = nullptr;
    }

    // cleanup_openssl();
}

EVP_PKEY* createEcKey()
{
    EVP_PKEY* pKey = nullptr;

#if (OPENSSL_VERSION_NUMBER < 0x30000000L)
    int eccgrp = 0;
    eccgrp = OBJ_txt2nid("secp384r1");

    EC_KEY* myecc = EC_KEY_new_by_curve_name(eccgrp);
    if (myecc != nullptr)
    {
        EC_KEY_set_asn1_flag(myecc, OPENSSL_EC_NAMED_CURVE);
        EC_KEY_generate_key(myecc);
        pKey = EVP_PKEY_new();
        if (pKey != nullptr)
        {
            if (EVP_PKEY_assign(pKey, EVP_PKEY_EC, myecc) != 0)
            {
                /* pKey owns myecc from now */
                if (EC_KEY_check_key(myecc) <= 0)
                {
                    std::cerr << "EC_check_key failed.\n";
                }
            }
        }
    }
#else
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
#endif

    return pKey;
}

void initOpenssl()
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    RAND_load_file("/dev/urandom", 1024);
#endif
}

inline void ensureOpensslKeyPresentAndValid(const std::string& filepath)
{
    bool pemFileValid = false;

    pemFileValid = verifyOpensslKeyCert(filepath);

    if (!pemFileValid)
    {
        std::cerr << "Error in verifying signature, regenerating\n";
        generateSslCertificate(filepath, "testhost");
    }
}

inline std::shared_ptr<boost::asio::ssl::context>
    getSslContext(const std::string& sslPemFile)
{
    std::shared_ptr<boost::asio::ssl::context> mSslContext =
        std::make_shared<boost::asio::ssl::context>(
            boost::asio::ssl::context::tls_server);
    mSslContext->set_options(boost::asio::ssl::context::default_workarounds |
                             boost::asio::ssl::context::no_sslv2 |
                             boost::asio::ssl::context::no_sslv3 |
                             boost::asio::ssl::context::single_dh_use |
                             boost::asio::ssl::context::no_tlsv1 |
                             boost::asio::ssl::context::no_tlsv1_1);

    // BIG WARNING: This needs to stay disabled, as there will always be
    // unauthenticated endpoints
    // mSslContext->set_verify_mode(boost::asio::ssl::verify_peer);

    SSL_CTX_set_options(mSslContext->native_handle(), SSL_OP_NO_RENEGOTIATION);

    BMCWEB_LOG_DEBUG << "Using default TrustStore location: " << trustStorePath;
    mSslContext->add_verify_path(trustStorePath);

    mSslContext->use_certificate_file(sslPemFile,
                                      boost::asio::ssl::context::pem);
    mSslContext->use_private_key_file(sslPemFile,
                                      boost::asio::ssl::context::pem);

    // Set up EC curves to auto (boost asio doesn't have a method for this)
    // There is a pull request to add this.  Once this is included in an asio
    // drop, use the right way
    // http://stackoverflow.com/questions/18929049/boost-asio-with-ecdsa-certificate-issue
    if (SSL_CTX_set_ecdh_auto(mSslContext->native_handle(), 1) != 1)
    {
        BMCWEB_LOG_ERROR << "Error setting tmp ecdh list\n";
    }

    std::string mozillaModern = "ECDHE-ECDSA-AES256-GCM-SHA384:"
                                "ECDHE-RSA-AES256-GCM-SHA384:"
                                "ECDHE-ECDSA-CHACHA20-POLY1305:"
                                "ECDHE-RSA-CHACHA20-POLY1305:"
                                "ECDHE-ECDSA-AES128-GCM-SHA256:"
                                "ECDHE-RSA-AES128-GCM-SHA256:"
                                "ECDHE-ECDSA-AES256-SHA384:"
                                "ECDHE-RSA-AES256-SHA384:"
                                "ECDHE-ECDSA-AES128-SHA256:"
                                "ECDHE-RSA-AES128-SHA256";

    if (SSL_CTX_set_cipher_list(mSslContext->native_handle(),
                                mozillaModern.c_str()) != 1)
    {
        BMCWEB_LOG_ERROR << "Error setting cipher list\n";
    }
    return mSslContext;
}

inline std::optional<boost::asio::ssl::context> getSSLClientContext()
{
    boost::asio::ssl::context sslCtx(boost::asio::ssl::context::tls_client);

    boost::system::error_code ec;

    // Support only TLS v1.2 & v1.3
    sslCtx.set_options(boost::asio::ssl::context::default_workarounds |
                           boost::asio::ssl::context::no_sslv2 |
                           boost::asio::ssl::context::no_sslv3 |
                           boost::asio::ssl::context::single_dh_use |
                           boost::asio::ssl::context::no_tlsv1 |
                           boost::asio::ssl::context::no_tlsv1_1,
                       ec);
    if (ec)
    {
        BMCWEB_LOG_ERROR << "SSL context set_options failed";
        return std::nullopt;
    }

    // Add a directory containing certificate authority files to be used
    // for performing verification.
    sslCtx.set_default_verify_paths(ec);
    if (ec)
    {
        BMCWEB_LOG_ERROR << "SSL context set_default_verify failed";
        return std::nullopt;
    }

    // Verify the remote server's certificate
    sslCtx.set_verify_mode(boost::asio::ssl::verify_peer, ec);
    if (ec)
    {
        BMCWEB_LOG_ERROR << "SSL context set_verify_mode failed";
        return std::nullopt;
    }

    // All cipher suites are set as per OWASP datasheet.
    // https://cheatsheetseries.owasp.org/cheatsheets/TLS_Cipher_String_Cheat_Sheet.html
    const char* sslCiphers = "ECDHE-ECDSA-AES128-GCM-SHA256:"
                             "ECDHE-RSA-AES128-GCM-SHA256:"
                             "ECDHE-ECDSA-AES256-GCM-SHA384:"
                             "ECDHE-RSA-AES256-GCM-SHA384:"
                             "ECDHE-ECDSA-CHACHA20-POLY1305:"
                             "ECDHE-RSA-CHACHA20-POLY1305:"
                             "DHE-RSA-AES128-GCM-SHA256:"
                             "DHE-RSA-AES256-GCM-SHA384"
                             "TLS_AES_128_GCM_SHA256:"
                             "TLS_AES_256_GCM_SHA384:"
                             "TLS_CHACHA20_POLY1305_SHA256";

    if (SSL_CTX_set_cipher_list(sslCtx.native_handle(), sslCiphers) != 1)
    {
        BMCWEB_LOG_ERROR << "SSL_CTX_set_cipher_list failed";
        return std::nullopt;
    }

    return sslCtx;
}

} // namespace ensuressl
