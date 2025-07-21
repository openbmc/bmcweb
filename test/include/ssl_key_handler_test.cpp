// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "duplicatable_file_handle.hpp"
#include "ssl_key_handler.hpp"

#include <string>

#include <gtest/gtest.h>

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

} // namespace ensuressl
