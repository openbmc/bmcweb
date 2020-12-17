#pragma once
#ifdef BMCWEB_ENABLE_SSL

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

#include <random>

namespace ensuressl
{
constexpr char const* trustStorePath = "/etc/ssl/certs/authority";
constexpr char const* x509Comment = "Generated from OpenBMC service";
static void initOpenssl();
static EVP_PKEY* createEcKey();

// Trust chain related errors.`
inline bool isTrustChainError(int errnum)
{
    if ((errnum == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT) ||
        (errnum == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN) ||
        (errnum == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY) ||
        (errnum == X509_V_ERR_CERT_UNTRUSTED) ||
        (errnum == X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE))
    {
        return true;
    }
    return false;
}

inline bool validateCertificate(X509* const cert)
{
    // Create an empty X509_STORE structure for certificate validation.
    X509_STORE* x509Store = X509_STORE_new();
    if (!x509Store)
    {
        BMCWEB_LOG_ERROR << "Error occurred during X509_STORE_new call";
        return false;
    }

    // Load Certificate file into the X509 structure.
    X509_STORE_CTX* storeCtx = X509_STORE_CTX_new();
    if (!storeCtx)
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

            EVP_PKEY_free(pkey);
        }
        fclose(file);
    }
    return certValid;
}

inline X509* loadCert(const std::string& filePath)
{
    BIO* certFileBio = BIO_new_file(filePath.c_str(), "rb");
    if (!certFileBio)
    {
        BMCWEB_LOG_ERROR << "Error occured during BIO_new_file call, "
                         << "FILE= " << filePath;
        return nullptr;
    }

    X509* cert = X509_new();
    if (!cert)
    {
        BMCWEB_LOG_ERROR << "Error occured during X509_new call, "
                         << ERR_get_error();
        BIO_free(certFileBio);
        return nullptr;
    }

    if (!PEM_read_bio_X509(certFileBio, &cert, nullptr, nullptr))
    {
        BMCWEB_LOG_ERROR << "Error occured during PEM_read_bio_X509 call, "
                         << "FILE= " << filePath;

        BIO_free(certFileBio);
        X509_free(cert);
        return nullptr;
    }
    return cert;
}

inline int addExt(X509* cert, int nid, const char* value)
{
    X509_EXTENSION* ex = nullptr;
    X509V3_CTX ctx;
    X509V3_set_ctx_nodb(&ctx);
    X509V3_set_ctx(&ctx, cert, cert, nullptr, nullptr, 0);
    ex = X509V3_EXT_conf_nid(nullptr, &ctx, nid, const_cast<char*>(value));
    if (!ex)
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
        X509* x509;
        x509 = X509_new();
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
            X509_NAME* name;
            name = X509_get_subject_name(x509);

            X509_NAME_add_entry_by_txt(
                name, "C", MBSTRING_ASC,
                reinterpret_cast<const unsigned char*>("US"), -1, -1, 0);
            X509_NAME_add_entry_by_txt(
                name, "O", MBSTRING_ASC,
                reinterpret_cast<const unsigned char*>("OpenBMC"), -1, -1, 0);
            X509_NAME_add_entry_by_txt(
                name, "CN", MBSTRING_ASC,
                reinterpret_cast<const unsigned char*>(cn.c_str()), -1, -1, 0);
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
            if (EVP_PKEY_assign_EC_KEY(pKey, myecc))
            {
                /* pKey owns myecc from now */
                if (EC_KEY_check_key(myecc) <= 0)
                {
                    fprintf(stderr, "EC_check_key failed.\n");
                }
            }
        }
    }
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
} // namespace ensuressl

#endif
