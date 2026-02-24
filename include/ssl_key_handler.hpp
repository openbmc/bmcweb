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

// SNI parsing helper macros for safe buffer access
#define SNI_MIN_LENGTH 5 // list_len(2) + type(1) + name_len(2)

// Read 2-byte big-endian value from span at position
#define SNI_READ_UINT16(span, pos)                                             \
    static_cast<uint16_t>((static_cast<uint16_t>((span)[(pos)]) << 8) |        \
                          static_cast<uint16_t>((span)[(pos) + 1]))

// Read 1-byte value from span at position
#define SNI_READ_UINT8(span, pos) (static_cast<uint8_t>((span)[(pos)]))

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
