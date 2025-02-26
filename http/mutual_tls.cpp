// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "mutual_tls.hpp"

#include "bmcweb_config.h"

#include "identity.hpp"
#include "mutual_tls_private.hpp"
#include "sessions.hpp"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

extern "C"
{
#include <openssl/asn1.h>
#include <openssl/obj_mac.h>
#include <openssl/objects.h>
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

std::string getCommonNameFromCert(X509* cert)
{
    std::string commonName;
    // Extract username contained in CommonName
    commonName.resize(256, '\0');
    int length = X509_NAME_get_text_by_NID(
        X509_get_subject_name(cert), NID_commonName, commonName.data(),
        static_cast<int>(commonName.size()));
    if (length <= 0)
    {
        BMCWEB_LOG_DEBUG("TLS cannot get common name to create session");
        return "";
    }
    commonName.resize(static_cast<size_t>(length));
    return commonName;
}

bool isUPNMatch(std::string_view upn, std::string_view hostname)
{
    // UPN format: <username>@<domain> (e.g. user@domain.com)
    // https://learn.microsoft.com/en-us/windows/win32/ad/naming-properties#userprincipalname
    size_t upnDomainPos = upn.find('@');
    if (upnDomainPos == std::string_view::npos)
    {
        return false;
    }

    // The hostname should match the domain part of the UPN
    std::string_view upnDomain = upn.substr(upnDomainPos + 1);
    while (true)
    {
        std::string_view upnDomainMatching = upnDomain;
        size_t dotUPNPos = upnDomain.find_last_of('.');
        if (dotUPNPos != std::string_view::npos)
        {
            upnDomainMatching = upnDomain.substr(dotUPNPos + 1);
        }

        std::string_view hostDomainMatching = hostname;
        size_t dotHostPos = hostname.find_last_of('.');
        if (dotHostPos != std::string_view::npos)
        {
            hostDomainMatching = hostname.substr(dotHostPos + 1);
        }

        if (upnDomainMatching != hostDomainMatching)
        {
            return false;
        }

        if (dotUPNPos == std::string_view::npos)
        {
            return true;
        }

        upnDomain = upnDomain.substr(0, dotUPNPos);
        hostname = hostname.substr(0, dotHostPos);
    }
}

std::string getUPNFromCert(X509* peerCert, std::string_view hostname)
{
    GENERAL_NAMES* gs = static_cast<GENERAL_NAMES*>(
        X509_get_ext_d2i(peerCert, NID_subject_alt_name, nullptr, nullptr));
    if (gs == nullptr)
    {
        return "";
    }

    std::string ret;
    for (int i = 0; i < sk_GENERAL_NAME_num(gs); i++)
    {
        GENERAL_NAME* g = sk_GENERAL_NAME_value(gs, i);
        if (g->type != GEN_OTHERNAME)
        {
            continue;
        }

        // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
        int nid = OBJ_obj2nid(g->d.otherName->type_id);
        if (nid != NID_ms_upn)
        {
            continue;
        }

        int type = g->d.otherName->value->type;
        if (type != V_ASN1_UTF8STRING)
        {
            continue;
        }

        char* upnChar =
            std::bit_cast<char*>(g->d.otherName->value->value.utf8string->data);
        unsigned int upnLen = static_cast<unsigned int>(
            g->d.otherName->value->value.utf8string->length);
        // NOLINTEND(cppcoreguidelines-pro-type-union-access)

        std::string upn = std::string(upnChar, upnLen);
        if (!isUPNMatch(upn, hostname))
        {
            continue;
        }

        size_t upnDomainPos = upn.find('@');
        ret = upn.substr(0, upnDomainPos);
        break;
    }
    GENERAL_NAMES_free(gs);
    return ret;
}

std::string getMetaUserNameFromCert(X509* cert)
{
    // Meta Inc. CommonName parsing
    std::optional<std::string_view> sslUserMeta =
        mtlsMetaParseSslUser(getCommonNameFromCert(cert));
    if (!sslUserMeta)
    {
        return "";
    }
    return std::string{*sslUserMeta};
}

std::string getUsernameFromCert(X509* cert)
{
    const persistent_data::AuthConfigMethods& authMethodsConfig =
        persistent_data::SessionStore::getInstance().getAuthMethodsConfig();
    switch (authMethodsConfig.mTLSCommonNameParsingMode)
    {
        case persistent_data::MTLSCommonNameParseMode::Invalid:
        case persistent_data::MTLSCommonNameParseMode::Whole:
        {
            // Not yet supported
            return "";
        }
        case persistent_data::MTLSCommonNameParseMode::UserPrincipalName:
        {
            std::string hostname = getHostName();
            if (hostname.empty())
            {
                BMCWEB_LOG_WARNING("Failed to get hostname");
                return "";
            }
            return getUPNFromCert(cert, hostname);
        }
        case persistent_data::MTLSCommonNameParseMode::CommonName:
        {
            return getCommonNameFromCert(cert);
        }
        case persistent_data::MTLSCommonNameParseMode::Meta:
        {
            if constexpr (BMCWEB_META_TLS_COMMON_NAME_PARSING)
            {
                return getMetaUserNameFromCert(cert);
            }
        }
        default:
        {
            return "";
        }
    }
}

std::shared_ptr<persistent_data::UserSession> verifyMtlsUser(
    const boost::asio::ip::address& clientIp,
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

    std::string sslUser = getUsernameFromCert(peerCert);
    if (sslUser.empty())
    {
        BMCWEB_LOG_WARNING("Failed to get user from peer certificate");
        return nullptr;
    }

    std::string unsupportedClientId;
    return persistent_data::SessionStore::getInstance().generateUserSession(
        sslUser, clientIp, unsupportedClientId,
        persistent_data::SessionType::MutualTLS);
}
