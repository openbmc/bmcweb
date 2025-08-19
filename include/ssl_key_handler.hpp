// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once

#include <boost/asio/ssl/context.hpp>

#include <memory>
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

} // namespace ensuressl
