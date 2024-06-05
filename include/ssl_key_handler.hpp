#pragma once

#include <boost/asio/ssl/context.hpp>

#include <optional>
#include <string>

namespace ensuressl
{
constexpr const char* trustStorePath = "/etc/ssl/certs/authority";
constexpr const char* x509Comment = "Generated from OpenBMC service";

// Trust chain related errors.`
bool isTrustChainError(int errnum);

bool validateCertificate(X509* const cert);

bool verifyOpensslKeyCert(const std::string& filepath);

X509* loadCert(const std::string& filePath);

int addExt(X509* cert, int nid, const char* value);

void generateSslCertificate(const std::string& filepath, const std::string& cn);

void ensureOpensslKeyPresentAndValid(const std::string& filepath);

std::shared_ptr<boost::asio::ssl::context>
    getSslContext(const std::string& sslPemFile);

std::optional<boost::asio::ssl::context> getSSLClientContext();

} // namespace ensuressl
