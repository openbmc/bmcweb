// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "mutual_tls.hpp"

#include "sessions.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

extern "C"
{
#include <openssl/obj_mac.h>
#include <openssl/types.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509v3.h>
}

#include "logging.hpp"
#include "mutual_tls_meta.hpp"

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ssl/verify_context.hpp>

#include <memory>
#include <string_view>

std::string getUsernameFromCommonName(std::string_view commonName)
{
    const persistent_data::AuthConfigMethods& authMethodsConfig =
        persistent_data::SessionStore::getInstance().getAuthMethodsConfig();
    switch (authMethodsConfig.mTLSCommonNameParsingMode)
    {
        case persistent_data::MTLSCommonNameParseMode::Invalid:
        case persistent_data::MTLSCommonNameParseMode::Whole:
        case persistent_data::MTLSCommonNameParseMode::UserPrincipalName:
        {
            // Not yet supported
            return "";
        }
        case persistent_data::MTLSCommonNameParseMode::CommonName:
        {
            return std::string{commonName};
        }
        case persistent_data::MTLSCommonNameParseMode::Meta:
        {
            // Meta Inc. CommonName parsing
            std::optional<std::string_view> sslUserMeta =
                mtlsMetaParseSslUser(commonName);
            if (!sslUserMeta)
            {
                return "";
            }
            return std::string{*sslUserMeta};
        }
        default:
        {
            return "";
        }
    }
}

std::shared_ptr<persistent_data::UserSession>
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

    std::string commonName;
    // Extract username contained in CommonName
    commonName.resize(256, '\0');

    int length = X509_NAME_get_text_by_NID(
        X509_get_subject_name(peerCert), NID_commonName, commonName.data(),
        static_cast<int>(commonName.size()));
    if (length <= 0)
    {
        BMCWEB_LOG_DEBUG("TLS cannot get common name to create session");
        return nullptr;
    }

    commonName.resize(static_cast<size_t>(length));
    std::string sslUser = getUsernameFromCommonName(commonName);
    if (sslUser.empty())
    {
        BMCWEB_LOG_WARNING("Failed to get user from common name {}",
                           commonName);
        return nullptr;
    }

    std::string unsupportedClientId;
    return persistent_data::SessionStore::getInstance().generateUserSession(
        sslUser, clientIp, unsupportedClientId,
        persistent_data::SessionType::MutualTLS);
}
