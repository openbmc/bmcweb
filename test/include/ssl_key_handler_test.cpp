// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "duplicatable_file_handle.hpp"
#include "ossl_test_memory.hpp"
#include "ssl_key_handler.hpp"

#include <boost/asio/ssl/context.hpp>

#include <optional>
#include <string>

#include <gtest/gtest.h>

static const OpenSSLTestMemory osslInit;

namespace ensuressl
{

TEST(SSLKeyHandler, GenerateVerifyRoundTrip)
{
    /* Verifies that we can generate a certificate, then read back in the
     * certificate that was read */
    DuplicatableFileHandle myFile("");
    std::string cert = generateSslCertificate("TestCommonName");

    EXPECT_FALSE(cert.empty());

    writeCertificateToFile(myFile.filePath, cert);

    std::string cert2 = verifyOpensslKeyCert(myFile.filePath);
    EXPECT_FALSE(cert2.empty());
    EXPECT_EQ(cert, cert2);
}

TEST(SSLKeyHandler, LoadPrivateKeyFromFileUri)
{
    std::string cert = generateSslCertificate("TestCommonName");
    ASSERT_FALSE(cert.empty());
    DuplicatableFileHandle keyFile(cert);

    boost::asio::ssl::context ctx(boost::asio::ssl::context::tls_server);
    EXPECT_TRUE(
        loadPrivateKeyUriIntoContext(ctx, "file://" + keyFile.filePath));
}

TEST(SSLKeyHandler, LoadPrivateKeyFromMissingUriFails)
{
    boost::asio::ssl::context ctx(boost::asio::ssl::context::tls_server);
    EXPECT_FALSE(loadPrivateKeyUriIntoContext(
        ctx, "file:///tmp/bmcweb/does-not-exist-key.pem"));
}

TEST(SSLKeyHandler, FileUriToPath)
{
    // Absolute file:// URI -> filesystem path.
    EXPECT_EQ(fileUriToPath("file:///etc/ssl/certs/https/server.pem"),
              "/etc/ssl/certs/https/server.pem");
    // Bare absolute path (no scheme) is accepted for backwards compatibility.
    EXPECT_EQ(fileUriToPath("/etc/ssl/certs/https/server.pem"),
              "/etc/ssl/certs/https/server.pem");
    // Two-slash form is malformed (the path is not absolute) -> rejected.
    EXPECT_EQ(fileUriToPath("file://etc/ssl/certs/https/server.pem"),
              std::nullopt);
    EXPECT_EQ(fileUriToPath("file://"), std::nullopt);
    // Provider-backed schemes are not filesystem paths -> rejected.
    EXPECT_EQ(fileUriToPath("handle:0x81000000"), std::nullopt);
}

} // namespace ensuressl
