// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once

#include <boost/asio/ssl/context.hpp>

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace ensuressl
{

enum class VerifyCertificate
{
    NoVerify = 0,
    Verify = 1
};

constexpr const char* trustStorePath = "/etc/ssl/certs/authority";
constexpr const char* x509Comment = "Generated from OpenBMC service";

std::string verifyOpensslKeyCert(const std::string& filepath);

void regenerateCertificateIfHostnameChanged(const std::string& filepath,
                                            const std::string& hostname);

std::string generateSslCertificate(const std::string& commonName);

void writeCertificateToFile(const std::string& filepath,
                            const std::string& certificate);

std::string ensureOpensslKeyPresentAndValid(const std::string& filepath);

std::shared_ptr<boost::asio::ssl::context> getSslServerContext();

std::optional<boost::asio::ssl::context> getSSLClientContext(
    VerifyCertificate verifyCertificate);

/**
 * @brief Loads a private key from a URI via the OpenSSL OSSL_STORE API and
 *        installs it into the SSL context.
 *
 * The certificate must already be set on the context.
 *
 * @param sslCtx The SSL context to install the private key into.
 * @param uri    The key location: a file:// URI or a provider-backed scheme
 *               such as a TPM handle:.
 * @return true on success, false on failure.
 */
bool loadPrivateKeyUriIntoContext(boost::asio::ssl::context& sslCtx,
                                  std::string_view uri);

/**
 * @brief Resolves a certificate location to a filesystem path usable with
 *        use_certificate_chain_file.
 *
 * Provider schemes (e.g. a TPM handle:) are handled separately via
 * loadCertPemFromUri.
 *
 * @param uri A file:// URI or a bare absolute filesystem path.
 * @return The filesystem path, or nullopt for provider schemes and other
 *         unsupported schemes.
 */
std::optional<std::string> fileUriToPath(std::string_view uri);

/**
 * @brief Determines whether a certificate location is a provider object.
 *
 * A provider object (e.g. a TPM NV index "handle:0x1500010") must be read via
 * OSSL_STORE rather than the filesystem. file:// URIs and bare paths are
 * filesystem paths.
 *
 * @param location The certificate location to classify.
 * @return true if the location is a provider object, false otherwise.
 */
bool isProviderCert(std::string_view location);

/**
 * @brief Loads a certificate from a provider URI via the OpenSSL OSSL_STORE
 *        API and returns it as a PEM string.
 *
 * This is the cert counterpart of loadPrivateKeyUriIntoContext; the returned
 * PEM feeds the same use_certificate_chain path a filesystem cert does.
 *
 * @param uri The certificate location, e.g. a TPM NV "handle:".
 * @return The certificate as a PEM string, or nullopt on failure.
 */
std::optional<std::string> loadCertPemFromUri(const std::string& uri);

/**
 * @brief Drains and logs the OpenSSL error queue with the given context prefix.
 *
 * A boost::asio "asio.ssl error" hides the underlying OpenSSL reason (cert
 * verify failure, provider/TPM error, key mismatch); calling this on failure
 * surfaces it.
 *
 * @param context A prefix identifying where the error occurred.
 */
void logOpenSSLErrors(std::string_view context);

} // namespace ensuressl
