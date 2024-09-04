#include "file_test_utilities.hpp"
#include "ssl_key_handler.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace ensuressl
{

TEST(SSLKeyHandler, GenerateVerifyRoundTrip)
{
    /* Verifies that we can generate a certificate, then read back in the
     * certificate that was read */
    TemporaryFileHandle myFile("");
    std::string cert = generateSslCertificate("TestCommonName");

    EXPECT_FALSE(cert.empty());

    writeCertificateToFile(myFile.stringPath, cert);

    std::string cert2 = verifyOpensslKeyCert(myFile.stringPath);
    EXPECT_FALSE(cert2.empty());
    EXPECT_EQ(cert, cert2);
}

} // namespace ensuressl
