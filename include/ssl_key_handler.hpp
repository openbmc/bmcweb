// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once

#include <openssl/crypto.h>

#include <boost/asio/ssl/context.hpp>

#include <memory>
#include <optional>
#include <string>
// Common SNI hostname for mTLS
constexpr const char* kMtlsSniHostname = "mtls.bmc";

// SNI parsing macros
#define SNI_OFFSET_SIZE 2
#define SNI_NAME_TYPE_SIZE 1
#define SNI_PARSE_START_POS (SNI_OFFSET_SIZE + SNI_NAME_TYPE_SIZE)
#define SNI_GET_HOST_LENGTH(sni, pos)                                          \
    (((uint16_t)(sni)[(pos)] << 8) | (sni)[(pos) + 1])
#define SNI_HOST_START_POS(pos) ((pos) + SNI_OFFSET_SIZE)
#define SNI_MIN_LENGTH 5

namespace ensuressl
{

enum class VerifyCertificate
{
    Verify,
    NoVerify
};

constexpr const char* trustStorePath = "/etc/ssl/certs/authority";
constexpr const char* x509Comment = "Generated from OpenBMC service";

bool isTrustChainError(int errnum);

bool validateCertificate(X509* cert);

std::string verifyOpensslKeyCert(const std::string& filepath);

X509* loadCert(const std::string& filePath);

int addExt(X509* cert, int nid, const char* value);

std::string generateSslCertificate(const std::string& cn);

void writeCertificateToFile(const std::string& filepath,
                            const std::string& certificate);

std::string ensureOpensslKeyPresentAndValid(const std::string& filepath);

std::shared_ptr<boost::asio::ssl::context> getSslServerContext();

std::optional<boost::asio::ssl::context> getSSLClientContext(
    VerifyCertificate verifyCertificate);

} // namespace ensuressl
