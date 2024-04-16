#pragma once

#include "logging.hpp"
#include "mutual_tls_meta.hpp"
#include "persistent_data.hpp"

extern "C"
{
#include <openssl/crypto.h>
#include <openssl/ssl.h>
}

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ssl/verify_context.hpp>

#include <memory>
#include <span>

inline std::shared_ptr<persistent_data::UserSession>
    verifyMtlsUser(const boost::asio::ip::address& clientIp,
                   boost::asio::ssl::verify_context& ctx)
{
    // do nothing if TLS is disabled
    if (!persistent_data::SessionStore::getInstance()
             .getAuthMethodsConfig()
             .tls)
    {
        BMCWEB_LOG_DEBUG("TLS auth_config is disabled");
        return nullptr;
    }

    X509_STORE_CTX* cts = ctx.native_handle();
    if (cts == nullptr)
    {
        BMCWEB_LOG_DEBUG("Cannot get native TLS handle.");
        return nullptr;
    }

    // Get certificate
    X509* peerCert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    if (peerCert == nullptr)
    {
        BMCWEB_LOG_DEBUG("Cannot get current TLS certificate.");
        return nullptr;
    }

    // Check if certificate is OK
    int ctxError = X509_STORE_CTX_get_error(cts);
    if (ctxError != X509_V_OK)
    {
        BMCWEB_LOG_INFO("Last TLS error is: {}", ctxError);
        return nullptr;
    }

    // Check that we have reached final certificate in chain
    int32_t depth = X509_STORE_CTX_get_error_depth(cts);
    if (depth != 0)
    {
        BMCWEB_LOG_DEBUG(
            "Certificate verification in progress (depth {}), waiting to reach final depth",
            depth);
        return nullptr;
    }

    BMCWEB_LOG_DEBUG("Certificate verification of final depth");

    if (X509_check_purpose(peerCert, X509_PURPOSE_SSL_CLIENT, 0) != 1)
    {
        BMCWEB_LOG_DEBUG(
            "Chain does not allow certificate to be used for SSL client authentication");
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
        BMCWEB_LOG_DEBUG("TLS cannot get username to create session");
        return nullptr;
    }

    size_t lastChar = sslUser.find('\0');
    if (lastChar == std::string::npos || lastChar == 0)
    {
        BMCWEB_LOG_DEBUG("Invalid TLS user name");
        return nullptr;
    }
    sslUser.resize(lastChar);

    // Meta Inc. CommonName parsing
    if constexpr (BMCWEB_MUTUAL_TLS_COMMON_NAME_PARSING == "meta")
    {
        std::optional<std::string_view> sslUserMeta =
            mtlsMetaParseSslUser(sslUser);
        if (!sslUserMeta)
        {
            return nullptr;
        }
        sslUser = *sslUserMeta;
    }

    std::string unsupportedClientId;
    return persistent_data::SessionStore::getInstance().generateUserSession(
        sslUser, clientIp, unsupportedClientId,
        persistent_data::SessionType::MutualTLS);
}
