#pragma once

#include "http_request.h"

#include <sessions.hpp>
#include <ssl_key_handler.hpp>

#include <functional>

namespace tls
{

class TlsSessionHandler
{
  public:
    TlsSessionHandler() = delete;
    TlsSessionHandler(const TlsSessionHandler&) = delete;
    TlsSessionHandler& operator=(const TlsSessionHandler&) = delete;
    TlsSessionHandler(TlsSessionHandler&&) = delete;
    TlsSessionHandler& operator=(TlsSessionHandler&&) = delete;

    TlsSessionHandler(const std::string& serverName,
                      crow::Request::Adaptor& adaptor)
    {
        auto ca_available = !std::filesystem::is_empty(
            std::filesystem::path(ensuressl::trustStorePath));
        if (ca_available && crow::persistent_data::SessionStore::getInstance()
                                .getAuthMethodsConfig()
                                .tls)
        {
            adaptor.set_verify_mode(boost::asio::ssl::verify_peer);
            SSL_set_session_id_context(
                adaptor.native_handle(),
                reinterpret_cast<const unsigned char*>(serverName.c_str()),
                static_cast<unsigned int>(serverName.length()));
            BMCWEB_LOG_DEBUG << this << " TLS is enabled on this connection.";
        }

        adaptor.set_verify_callback(createSessionFromCert());
    }

    virtual ~TlsSessionHandler()
    {
        if (auto sp = session.lock())
        {
            BMCWEB_LOG_DEBUG << this
                             << " Removing TLS session: " << sp->uniqueId;
            crow::persistent_data::SessionStore::getInstance().removeSession(
                sp);
        }
    }

    const std::shared_ptr<crow::persistent_data::UserSession> getUserSession()
    {
        if (auto sp = session.lock())
        {
            return sp;
        }
        else
        {
            return nullptr;
        }
    }

  private:
    std::weak_ptr<crow::persistent_data::UserSession> session;

    std::function<bool(bool preverified, boost::asio::ssl::verify_context& ctx)>
        createSessionFromCert()
    {

        return [this](bool preverified, boost::asio::ssl::verify_context& ctx) {
            // do nothing if TLS is disabled
            if (!crow::persistent_data::SessionStore::getInstance()
                     .getAuthMethodsConfig()
                     .tls)
            {
                BMCWEB_LOG_DEBUG << this << " TLS auth_config is disabled";
                return true;
            }

            // We always return true to allow full auth flow
            if (!preverified)
            {
                BMCWEB_LOG_DEBUG << this << " TLS preverification failed.";
                return true;
            }

            X509_STORE_CTX* cts = ctx.native_handle();
            if (cts == nullptr)
            {
                BMCWEB_LOG_DEBUG << this << " Cannot get native TLS handle.";
                return true;
            }

            // Get certificate
            X509* peerCert =
                X509_STORE_CTX_get_current_cert(ctx.native_handle());
            if (peerCert == nullptr)
            {
                BMCWEB_LOG_DEBUG << this
                                 << " Cannot get current TLS certificate.";
                return true;
            }

            // Check if certificate is OK
            int error = X509_STORE_CTX_get_error(cts);
            if (error != X509_V_OK)
            {
                BMCWEB_LOG_INFO << this << " Last TLS error is: " << error;
                return true;
            }
            // Check that we have reached final certificate in chain
            int32_t depth = X509_STORE_CTX_get_error_depth(cts);
            if (depth != 0)

            {
                BMCWEB_LOG_DEBUG
                    << this << " Certificate verification in progress (depth "
                    << depth << "), waiting to reach final depth";
                return true;
            }

            BMCWEB_LOG_DEBUG << this
                             << " Certificate verification of final depth";

            // Verify KeyUsage
            bool isKeyUsageDigitalSignature = false;
            bool isKeyUsageKeyAgreement = false;

            ASN1_BIT_STRING* usage = static_cast<ASN1_BIT_STRING*>(
                X509_get_ext_d2i(peerCert, NID_key_usage, NULL, NULL));

            if (usage == nullptr)
            {
                BMCWEB_LOG_DEBUG << this << " TLS usage is null";
                return true;
            }

            for (int i = 0; i < usage->length; i++)
            {
                if (KU_DIGITAL_SIGNATURE & usage->data[i])
                {
                    isKeyUsageDigitalSignature = true;
                }
                if (KU_KEY_AGREEMENT & usage->data[i])
                {
                    isKeyUsageKeyAgreement = true;
                }
            }

            if (!isKeyUsageDigitalSignature || !isKeyUsageKeyAgreement)
            {
                BMCWEB_LOG_DEBUG << this
                                 << " Certificate ExtendedKeyUsage does "
                                    "not allow provided certificate to "
                                    "be used for user authentication";
                return true;
            }
            ASN1_BIT_STRING_free(usage);

            // Determine that ExtendedKeyUsage includes Client Auth

            stack_st_ASN1_OBJECT* extUsage = static_cast<stack_st_ASN1_OBJECT*>(
                X509_get_ext_d2i(peerCert, NID_ext_key_usage, NULL, NULL));

            if (extUsage == nullptr)
            {
                BMCWEB_LOG_DEBUG << this << " TLS extUsage is null";
                return true;
            }

            bool isExKeyUsageClientAuth = false;
            for (int i = 0; i < sk_ASN1_OBJECT_num(extUsage); i++)
            {
                if (NID_client_auth ==
                    OBJ_obj2nid(sk_ASN1_OBJECT_value(extUsage, i)))
                {
                    isExKeyUsageClientAuth = true;
                    break;
                }
            }
            sk_ASN1_OBJECT_free(extUsage);

            // Certificate has to have proper key usages set
            if (!isExKeyUsageClientAuth)
            {
                BMCWEB_LOG_DEBUG << this
                                 << " Certificate ExtendedKeyUsage does "
                                    "not allow provided certificate to "
                                    "be used for user authentication";
                return true;
            }
            std::string sslUser;
            // Extract username contained in CommonName
            sslUser.resize(256, '\0');

            int status = X509_NAME_get_text_by_NID(
                X509_get_subject_name(peerCert), NID_commonName, sslUser.data(),
                static_cast<int>(sslUser.size()));

            if (status == -1)
            {
                BMCWEB_LOG_DEBUG
                    << this << " TLS cannot get username to create session";
                return true;
            }

            size_t lastChar = sslUser.find('\0');
            if (lastChar == std::string::npos || lastChar == 0)
            {
                BMCWEB_LOG_DEBUG << this << " Invalid TLS user name";
                return true;
            }
            sslUser.resize(lastChar);

            session = crow::persistent_data::SessionStore::getInstance()
                          .generateUserSession(
                              sslUser,
                              crow::persistent_data::PersistenceType::TIMEOUT);
            if (auto sp = session.lock())
            {
                BMCWEB_LOG_DEBUG << this
                                 << " Generating TLS session: " << sp->uniqueId;
            }
            return true;
        };
    }
};

class NoTlsSupport
{
  public:
    NoTlsSupport() = delete;
    NoTlsSupport(const NoTlsSupport&) = delete;
    NoTlsSupport& operator=(const NoTlsSupport&) = delete;
    NoTlsSupport(NoTlsSupport&&) = delete;
    NoTlsSupport& operator=(NoTlsSupport&&) = delete;
    NoTlsSupport(const std::string& serverName,
                 crow::Request::Adaptor& adaptor){};
    virtual ~NoTlsSupport() = default;

    const std::shared_ptr<crow::persistent_data::UserSession> getUserSession()
    {
        return nullptr;
    }
};

#ifdef BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION
using TlsHandler = tls::TlsSessionHandler;
#else
using TlsHandler = tls::NoTlsSupport;
#endif // BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION

} // namespace tls
