// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "file_test_utilities.hpp"
#include "metadata.hpp"

#include <filesystem>
#include <format>
#include <string_view>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

/*
Example from Redfish and OData whitepaper.

https://www.dmtf.org/sites/default/files/standards/documents/DSP2052_1.0.0.pdf
*/
constexpr std::string_view content =
    "<edmx:Edmx xmlns:edms=\"http://docs.oasis-open.org/odata/ns/edmx\" Version=\"4.0\">\n"
    "  <edmx:Reference Uri=\"http://contoso.org/schemas/ExternalSchema.xml\">\n"
    "    <edmx:Include Namespace=\"ExternalNamespace\"/>\n"
    "    <edmx:Include Namespace=\"Other.Namespace\"/>\n"
    "  </edmx:Reference>\n"
    "  <edmx:DataServices>\n"
    "    <Schema xmlns=\"http://docs.oasis-open.org/odata/ns/edm\" Namespace=\"MyNewNamespace\">\n"
    "      <ComplexType Name=\"MyDataType\">\n"
    "        <Property Name=\"MyProperty\" Type=\"ExternalNamespace.ReferencedDataType\"/>\n"
    "        <Property Name=\"MyProperty2\" Type=\"Other.Namespace.OtherDataType\"/>\n"
    "        <Property Name=\"MyProperty3\" Type=\"Edm.Int64\"/>\n"
    "      </ComplexType>\n"
    "    </Schema>\n"
    "  </edmx:DataServices>\n"
    "</edmx:Edmx>\n";

TEST(MetadataGet, GetOneFile)
{
    TemporaryFileHandle file(content);

    std::filesystem::path path{file.stringPath};
    EXPECT_EQ(
        getMetadataPieceForFile(path),
        std::format("    <edmx:Reference Uri=\"/redfish/v1/schema/{}\">\n"
                    "        <edmx:Include Namespace=\"MyNewNamespace\"/>\n"
                    "    </edmx:Reference>\n",
                    path.filename().string()));

    EXPECT_EQ(getMetadataPieceForFile("DoesNotExist_v1.xml"), "");
}

} // namespace
} // namespace redfish
