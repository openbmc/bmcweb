// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once

#include <boost/asio/ssl/context.hpp>

#include <optional>
#include <string>

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

std::optional<boost::asio::ssl::context>
    getSSLClientContext(VerifyCertificate verifyCertificate);

} // namespace ensuressl
