#pragma once

#include <openssl/bio.h>
#include <openssl/dh.h>
#include <openssl/dsa.h>
#include <openssl/dsa.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>

namespace ensuressl
{
static void init_openssl(void);
static void cleanup_openssl(void);
static EVP_PKEY *create_rsa_key(void);
static void handle_openssl_error(void);

inline bool verify_openssl_key_cert(const std::string &filepath)
{
    bool private_key_valid = false;
    bool cert_valid = false;
    FILE *file = fopen(filepath.c_str(), "r");
    if (file != NULL){   
        EVP_PKEY *pkey = PEM_read_PrivateKey(file, NULL, NULL, NULL);
        int rc;
        if (pkey) {
            int type = EVP_PKEY_type(pkey->type);
            switch (type) {
                case EVP_PKEY_RSA:
                case EVP_PKEY_RSA2: {
                    RSA *rsa = EVP_PKEY_get1_RSA(pkey);
                    rc = RSA_check_key(rsa);
                    if (rc == 1) {
                        private_key_valid = true;
                    }

                    //RSA_free(rsa);

                    break;
                }
                default:
                    break;
            }

            if (private_key_valid) {
                X509 *x509 = PEM_read_X509(file, NULL, NULL, NULL);
                unsigned long err = ERR_get_error();

                rc = X509_verify(x509, pkey);
                err = ERR_get_error();
                if (err == 0 && rc == 1) {
                    cert_valid = true;
                }
            }

            EVP_PKEY_free(pkey);
        }
        fclose(file);
    }
    return cert_valid;
}

inline void generate_ssl_certificate(const std::string &filepath)
{
    EVP_PKEY *pPrivKey = NULL;
    FILE *pFile = NULL;
    init_openssl();

    pPrivKey = create_rsa_key();

    // Use this code to directly generate a certificate
    X509 *x509;
    x509 = X509_new();
    if (x509) {
        // TODO get actually random int
        ASN1_INTEGER_set(X509_get_serialNumber(x509), 1584);

        // not before this moment
        X509_gmtime_adj(X509_get_notBefore(x509), 0);
        // Cert is valid for 10 years
        X509_gmtime_adj(X509_get_notAfter(x509), 60L * 60L * 24L * 365L * 10L);

        // set the public key to the key we just generated
        X509_set_pubkey(x509, pPrivKey);

        // Get the subject name
        X509_NAME *name;
        name = X509_get_subject_name(x509);

        X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char *)"US", -1,
                                   -1, 0);
        X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC,
                                   (unsigned char *)"Intel BMC", -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                                   (unsigned char *)"testhost", -1, -1, 0);
        // set the CSR options
        X509_set_issuer_name(x509, name);

        // Sign the certificate with our private key
        X509_sign(x509, pPrivKey, EVP_sha256());

        pFile = fopen(filepath.c_str(), "wt");

        if (pFile) {
            PEM_write_PrivateKey(pFile, pPrivKey, NULL, NULL, 0, 0, NULL);
            PEM_write_X509(pFile, x509);
            fclose(pFile);
            pFile = NULL;
        }

        X509_free(x509);
    }

    if (pPrivKey) {
        EVP_PKEY_free(pPrivKey);
        pPrivKey = NULL;
    }

    //cleanup_openssl();
}

EVP_PKEY *create_rsa_key(void)
{
    RSA *pRSA = NULL;
    EVP_PKEY *pKey = NULL;
    pRSA = RSA_generate_key(2048, RSA_3, NULL, NULL);
    pKey = EVP_PKEY_new();
    if (pRSA && pKey && EVP_PKEY_assign_RSA(pKey, pRSA)) {
        /* pKey owns pRSA from now */
        if (RSA_check_key(pRSA) <= 0) {
            fprintf(stderr, "RSA_check_key failed.\n");
            handle_openssl_error();
            EVP_PKEY_free(pKey);
            pKey = NULL;
        }
    } else {
        handle_openssl_error();
        if (pRSA) {
            RSA_free(pRSA);
            pRSA = NULL;
        }
        if (pKey) {
            EVP_PKEY_free(pKey);
            pKey = NULL;
        }
    }
    return pKey;
}

void init_openssl(void)
{
    if (SSL_library_init()) {
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        RAND_load_file("/dev/urandom", 1024);
    } else
        exit(EXIT_FAILURE);
}

void cleanup_openssl(void)
{
    CRYPTO_cleanup_all_ex_data();
    ERR_free_strings();
    ERR_remove_thread_state(0);
    EVP_cleanup();
}

void handle_openssl_error(void) { ERR_print_errors_fp(stderr); }
inline void ensure_openssl_key_present_and_valid(const std::string &filepath)
{
    bool pem_file_valid = false;

    pem_file_valid = verify_openssl_key_cert(filepath);

    if (!pem_file_valid) {
        generate_ssl_certificate(filepath);
    }
}
}