#pragma once
#ifdef CROW_ENABLE_SSL

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
#include <boost/asio.hpp>
#include <g3log/g3log.hpp>
#include <random>

namespace ensuressl {
static void init_openssl(void);
static void cleanup_openssl(void);
static EVP_PKEY *create_rsa_key(void);
static EVP_PKEY *create_ec_key(void);
static void handle_openssl_error(void);

inline bool verify_openssl_key_cert(const std::string &filepath) {
  bool private_key_valid = false;
  bool cert_valid = false;

  LOG(DEBUG) << "Checking certs in file " << filepath;

  FILE *file = fopen(filepath.c_str(), "r");
  if (file != NULL) {
    EVP_PKEY *pkey = PEM_read_PrivateKey(file, NULL, NULL, NULL);
    int rc;
    if (pkey) {
      int type = EVP_PKEY_type(pkey->type);
      switch (type) {
        case EVP_PKEY_RSA:
        case EVP_PKEY_RSA2: {
          LOG(DEBUG) << "Found an RSA key";
          RSA *rsa = EVP_PKEY_get1_RSA(pkey);
          if (rsa) {
            if (RSA_check_key(rsa) == 1) {
              // private_key_valid = true;
            } else {
              LOG(WARNING) << "Key not valid error number " << ERR_get_error();
            }
            RSA_free(rsa);
          }
          break;
        }
        case EVP_PKEY_EC: {
          LOG(DEBUG) << "Found an EC key";
          EC_KEY *ec = EVP_PKEY_get1_EC_KEY(pkey);
          if (ec) {
            if (EC_KEY_check_key(ec) == 1) {
              private_key_valid = true;
            } else {
              LOG(WARNING) << "Key not valid error number " << ERR_get_error();
            }
            EC_KEY_free(ec);
          }
          break;
        }
        default:
          LOG(WARNING) << "Found an unrecognized key type " << type;
          break;
      }

      if (private_key_valid) {
        X509 *x509 = PEM_read_X509(file, NULL, NULL, NULL);
        if (!x509) {
          LOG(DEBUG) << "error getting x509 cert " << ERR_get_error();
        } else {
          rc = X509_verify(x509, pkey);
          if (rc == 1) {
            cert_valid = true;
          } else {
            LOG(WARNING) << "Error in verifying private key signature "
                         << ERR_get_error();
          }
        }
      }

      EVP_PKEY_free(pkey);
    }
    fclose(file);
  }
  return cert_valid;
}

inline void generate_ssl_certificate(const std::string &filepath) {
  FILE *pFile = NULL;
  LOG(WARNING) << "Generating new keys";
  init_openssl();

  // LOG(WARNING) << "Generating RSA key";
  // EVP_PKEY *pRsaPrivKey = create_rsa_key();

  LOG(WARNING) << "Generating EC key";
  EVP_PKEY *pRsaPrivKey = create_ec_key();
  if (pRsaPrivKey) {
    LOG(WARNING) << "Generating x509 Certificate";
    // Use this code to directly generate a certificate
    X509 *x509;
    x509 = X509_new();
    if (x509) {
      // Get a random number from the RNG for the certificate serial number
      // If this is not random, regenerating certs throws broswer errors
      std::random_device rd;
      int serial = rd();

      ASN1_INTEGER_set(X509_get_serialNumber(x509), serial);

      // not before this moment
      X509_gmtime_adj(X509_get_notBefore(x509), 0);
      // Cert is valid for 10 years
      X509_gmtime_adj(X509_get_notAfter(x509), 60L * 60L * 24L * 365L * 10L);

      // set the public key to the key we just generated
      X509_set_pubkey(x509, pRsaPrivKey);

      // Get the subject name
      X509_NAME *name;
      name = X509_get_subject_name(x509);

      X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char *)"US",
                                 -1, -1, 0);
      X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC,
                                 (unsigned char *)"Intel BMC", -1, -1, 0);
      X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                                 (unsigned char *)"testhost", -1, -1, 0);
      // set the CSR options
      X509_set_issuer_name(x509, name);

      // Sign the certificate with our private key
      X509_sign(x509, pRsaPrivKey, EVP_sha256());

      pFile = fopen(filepath.c_str(), "wt");

      if (pFile) {
        PEM_write_PrivateKey(pFile, pRsaPrivKey, NULL, NULL, 0, 0, NULL);

        PEM_write_X509(pFile, x509);
        fclose(pFile);
        pFile = NULL;
      }

      X509_free(x509);
    }

    EVP_PKEY_free(pRsaPrivKey);
    pRsaPrivKey = NULL;
  }

  // cleanup_openssl();
}

EVP_PKEY *create_rsa_key(void) {
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

EVP_PKEY *create_ec_key(void) {
  EVP_PKEY *pKey = NULL;
  int eccgrp = 0;
  eccgrp = OBJ_txt2nid("prime256v1");

  EC_KEY *myecc = EC_KEY_new_by_curve_name(eccgrp);
  if (myecc) {
    EC_KEY_set_asn1_flag(myecc, OPENSSL_EC_NAMED_CURVE);
    EC_KEY_generate_key(myecc);
    pKey = EVP_PKEY_new();
    if (pKey) {
      if (EVP_PKEY_assign_EC_KEY(pKey, myecc)) {
        /* pKey owns pRSA from now */
        if (EC_KEY_check_key(myecc) <= 0) {
          fprintf(stderr, "EC_check_key failed.\n");
        }
      }
    }
  }
  return pKey;
}

void init_openssl(void) {
  if (SSL_library_init()) {
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    RAND_load_file("/dev/urandom", 1024);
  } else
    exit(EXIT_FAILURE);
}

void cleanup_openssl(void) {
  CRYPTO_cleanup_all_ex_data();
  ERR_free_strings();
  ERR_remove_thread_state(0);
  EVP_cleanup();
}

void handle_openssl_error(void) { ERR_print_errors_fp(stderr); }
inline void ensure_openssl_key_present_and_valid(const std::string &filepath) {
  bool pem_file_valid = false;

  pem_file_valid = verify_openssl_key_cert(filepath);

  if (!pem_file_valid) {
    LOG(WARNING) << "Error in verifying signature, regenerating";
    generate_ssl_certificate(filepath);
  }
}

boost::asio::ssl::context get_ssl_context(std::string ssl_pem_file) {
  boost::asio::ssl::context m_ssl_context{boost::asio::ssl::context::sslv23};
  m_ssl_context.set_options(boost::asio::ssl::context::default_workarounds |
                            boost::asio::ssl::context::no_sslv2 |
                            boost::asio::ssl::context::no_sslv3 |
                            boost::asio::ssl::context::single_dh_use |
                            boost::asio::ssl::context::no_tlsv1 |
                            boost::asio::ssl::context::no_tlsv1_1);

  // m_ssl_context.set_verify_mode(boost::asio::ssl::verify_peer);
  m_ssl_context.use_certificate_file(ssl_pem_file,
                                     boost::asio::ssl::context::pem);
  m_ssl_context.use_private_key_file(ssl_pem_file,
                                     boost::asio::ssl::context::pem);

  // Set up EC curves to auto (boost asio doesn't have a method for this)
  // There is a pull request to add this.  Once this is included in an asio
  // drop, use the right way
  // http://stackoverflow.com/questions/18929049/boost-asio-with-ecdsa-certificate-issue
  if (SSL_CTX_set_ecdh_auto(m_ssl_context.native_handle(), 1) != 1) {
    CROW_LOG_ERROR << "Error setting tmp ecdh list\n";
  }

  // From mozilla "compatibility"
  std::string ciphers =
      "ECDHE-ECDSA-CHACHA20-POLY1305:"
      "ECDHE-RSA-CHACHA20-POLY1305:"
      "ECDHE-ECDSA-AES128-GCM-SHA256:"
      "ECDHE-RSA-AES128-GCM-SHA256:"
      "ECDHE-ECDSA-AES256-GCM-SHA384:"
      "ECDHE-RSA-AES256-GCM-SHA384:"
      "DHE-RSA-AES128-GCM-SHA256:"
      "DHE-RSA-AES256-GCM-SHA384:"
      "ECDHE-ECDSA-AES128-SHA256:"
      "ECDHE-RSA-AES128-SHA256:"
      "ECDHE-ECDSA-AES128-SHA:"
      "ECDHE-RSA-AES256-SHA384:"
      "ECDHE-RSA-AES128-SHA:"
      "ECDHE-ECDSA-AES256-SHA384:"
      "ECDHE-ECDSA-AES256-SHA:"
      "ECDHE-RSA-AES256-SHA:"
      "DHE-RSA-AES128-SHA256:"
      "DHE-RSA-AES128-SHA:"
      "DHE-RSA-AES256-SHA256:"
      "DHE-RSA-AES256-SHA:"
      "ECDHE-ECDSA-DES-CBC3-SHA:"
      "ECDHE-RSA-DES-CBC3-SHA:"
      "EDH-RSA-DES-CBC3-SHA:"
      "AES128-GCM-SHA256:"
      "AES256-GCM-SHA384:"
      "AES128-SHA256:"
      "AES256-SHA256:"
      "AES128-SHA:"
      "AES256-SHA:"
      "DES-CBC3-SHA:"
      "!DSS";

  // From mozilla "modern"
  std::string modern_ciphers =
      "ECDHE-ECDSA-AES256-GCM-SHA384:"
      "ECDHE-RSA-AES256-GCM-SHA384:"
      "ECDHE-ECDSA-CHACHA20-POLY1305:"
      "ECDHE-RSA-CHACHA20-POLY1305:"
      "ECDHE-ECDSA-AES128-GCM-SHA256:"
      "ECDHE-RSA-AES128-GCM-SHA256:"
      "ECDHE-ECDSA-AES256-SHA384:"
      "ECDHE-RSA-AES256-SHA384:"
      "ECDHE-ECDSA-AES128-SHA256:"
      "ECDHE-RSA-AES128-SHA256";

  std::string lighttp_ciphers = "AES128+EECDH:AES128+EDH:!aNULL:!eNULL";

  if (SSL_CTX_set_cipher_list(m_ssl_context.native_handle(), ciphers.c_str()) !=
      1) {
    CROW_LOG_ERROR << "Error setting cipher list\n";
  }
  return m_ssl_context;
}
}

#endif