#pragma once

#include "logging.hpp"
#include "persistent_data.hpp"

#include <openssl/crypto.h>
#include <openssl/ssl.h>

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ssl/verify_context.hpp>

#include <memory>

inline std::shared_ptr<persistent_data::UserSession>
    verifyMtlsUser(const boost::asio::ip::address& clientIp,
                   boost::asio::ssl::verify_context& ctx)
{
    // do nothing if TLS is disabled
    if (!persistent_data::SessionStore::getInstance()
             .getAuthMethodsConfig()
             .tls)
    {
        BMCWEB_LOG_DEBUG << "TLS auth_config is disabled";
        return nullptr;
    }

    X509_STORE_CTX* cts = ctx.native_handle();
    if (cts == nullptr)
    {
        BMCWEB_LOG_DEBUG << "Cannot get native TLS handle.";
        return nullptr;
    }

    // Get certificate
    X509* peerCert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    if (peerCert == nullptr)
    {
        BMCWEB_LOG_DEBUG << "Cannot get current TLS certificate.";
        return nullptr;
    }

    // Check if certificate is OK
    int ctxError = X509_STORE_CTX_get_error(cts);
    if (ctxError != X509_V_OK)
    {
        BMCWEB_LOG_INFO << "Last TLS error is: " << ctxError;
        return nullptr;
    }
    // Check that we have reached final certificate in chain
    int32_t depth = X509_STORE_CTX_get_error_depth(cts);
    if (depth != 0)

    {
        BMCWEB_LOG_DEBUG << "Certificate verification in progress (depth "
                         << depth << "), waiting to reach final depth";
        return nullptr;
    }

    BMCWEB_LOG_DEBUG << "Certificate verification of final depth";

    // Verify KeyUsage
    bool isKeyUsageDigitalSignature = false;
    bool isKeyUsageKeyAgreement = false;

    ASN1_BIT_STRING* usage = static_cast<ASN1_BIT_STRING*>(
        X509_get_ext_d2i(peerCert, NID_key_usage, nullptr, nullptr));

    if (usage == nullptr)
    {
        BMCWEB_LOG_DEBUG << "TLS usage is null";
        return nullptr;
    }

    for (int i = 0; i < usage->length; i++)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        unsigned char usageChar = usage->data[i];
        if (KU_DIGITAL_SIGNATURE & usageChar)
        {
            isKeyUsageDigitalSignature = true;
        }
        if (KU_KEY_AGREEMENT & usageChar)
        {
            isKeyUsageKeyAgreement = true;
        }
    }
    ASN1_BIT_STRING_free(usage);

    if (!isKeyUsageDigitalSignature || !isKeyUsageKeyAgreement)
    {
        BMCWEB_LOG_DEBUG << "Certificate ExtendedKeyUsage does "
                            "not allow provided certificate to "
                            "be used for user authentication";
        return nullptr;
    }

    // Determine that ExtendedKeyUsage includes Client Auth

    stack_st_ASN1_OBJECT* extUsage = static_cast<stack_st_ASN1_OBJECT*>(
        X509_get_ext_d2i(peerCert, NID_ext_key_usage, nullptr, nullptr));

    if (extUsage == nullptr)
    {
        BMCWEB_LOG_DEBUG << "TLS extUsage is null";
        return nullptr;
    }

    bool isExKeyUsageClientAuth = false;
    for (int i = 0; i < sk_ASN1_OBJECT_num(extUsage); i++)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
        int nid = OBJ_obj2nid(sk_ASN1_OBJECT_value(extUsage, i));
        if (NID_client_auth == nid)
        {
            isExKeyUsageClientAuth = true;
            break;
        }
    }
    sk_ASN1_OBJECT_free(extUsage);

    // Certificate has to have proper key usages set
    if (!isExKeyUsageClientAuth)
    {
        BMCWEB_LOG_DEBUG << "Certificate ExtendedKeyUsage does "
                            "not allow provided certificate to "
                            "be used for user authentication";
        return nullptr;
    }
    std::string sslUser;
    // Extract username contained in CommonName
    sslUser.resize(256, '\0');

    int status = X509_NAME_get_text_by_NID(X509_get_subject_name(peerCert),
                                           NID_commonName, sslUser.data(),
                                           static_cast<int>(sslUser.size()));

    if (status == -1)
    {
        BMCWEB_LOG_DEBUG << "TLS cannot get username to create session";
        return nullptr;
    }

    size_t lastChar = sslUser.find('\0');
    if (lastChar == std::string::npos || lastChar == 0)
    {
        BMCWEB_LOG_DEBUG << "Invalid TLS user name";
        return nullptr;
    }
    sslUser.resize(lastChar);
    std::string unsupportedClientId;
    return persistent_data::SessionStore::getInstance().generateUserSession(
        sslUser, clientIp, unsupportedClientId,
        persistent_data::PersistenceType::TIMEOUT);
}
