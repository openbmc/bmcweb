#include "metadata.hpp"

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(MetadataGet, GetOneFile)
{
    EXPECT_EQ(
        getMetadataPieceForFile("test/redfish-core/lib/metadata_v1.xml"),
        "    <edmx:Reference Uri=\"/redfish/v1/schema/metadata_v1.xml\">\n"
        "        <edmx:Include Namespace=\"MyNewNamespace\"/>\n"
        "    </edmx:Reference>\n");

    EXPECT_EQ(getMetadataPieceForFile("DoesNotExist_v1.xml"), "");
}

} // namespace
} // namespace redfish
