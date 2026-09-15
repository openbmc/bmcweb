// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "certificate_service.hpp"

#include <nlohmann/json.hpp>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(UpdateCertIssuerOrSubject, EmptyValueLeavesJsonNull)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, "");
    EXPECT_TRUE(out.is_null());
}

TEST(UpdateCertIssuerOrSubject, LocalityMapsToCity)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, "L=Portland");
    EXPECT_EQ(out, R"({"City": "Portland"})"_json);
}

TEST(UpdateCertIssuerOrSubject, CommonNameMapsToCommonName)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, "CN=localhost");
    EXPECT_EQ(out, R"({"CommonName": "localhost"})"_json);
}

TEST(UpdateCertIssuerOrSubject, CountryMapsToCountry)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, "C=US");
    EXPECT_EQ(out, R"({"Country": "US"})"_json);
}

TEST(UpdateCertIssuerOrSubject, OrganizationMapsToOrganization)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, "O=openbmc-project.xyz");
    EXPECT_EQ(out, R"({"Organization": "openbmc-project.xyz"})"_json);
}

TEST(UpdateCertIssuerOrSubject, OrganizationalUnitMapsToOrganizationalUnit)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, "OU=Engineering");
    EXPECT_EQ(out, R"({"OrganizationalUnit": "Engineering"})"_json);
}

TEST(UpdateCertIssuerOrSubject, StateMapsToState)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, "ST=Oregon");
    EXPECT_EQ(out, R"({"State": "Oregon"})"_json);
}

TEST(UpdateCertIssuerOrSubject, AllSupportedKeysInOnePass)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(
        out, "L=Portland,CN=localhost,C=US,O=OpenBMC,OU=Engineering,ST=Oregon");
    EXPECT_EQ(out, R"({
        "City": "Portland",
        "CommonName": "localhost",
        "Country": "US",
        "Organization": "OpenBMC",
        "OrganizationalUnit": "Engineering",
        "State": "Oregon"
    })"_json);
}

TEST(UpdateCertIssuerOrSubject, ExampleFromSource)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, "O=openbmc-project.xyz,CN=localhost");
    EXPECT_EQ(out, R"({
        "CommonName": "localhost",
        "Organization": "openbmc-project.xyz"
    })"_json);
}

TEST(UpdateCertIssuerOrSubject, UnsupportedKeyIsDropped)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, "XYZ=abc");
    EXPECT_TRUE(out.is_null());
}

TEST(UpdateCertIssuerOrSubject, ParsingContinuesAfterUnsupportedKey)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, "XYZ=abc,CN=localhost");
    EXPECT_EQ(out, R"({"CommonName": "localhost"})"_json);
}

TEST(UpdateCertIssuerOrSubject, ValueWithoutEqualsSignIsDropped)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, "noequalsign");
    EXPECT_TRUE(out.is_null());
}

TEST(UpdateCertIssuerOrSubject, SupportedKeyWithoutEqualsSignIsDropped)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, "CN");
    EXPECT_TRUE(out.is_null());
}

TEST(UpdateCertIssuerOrSubject, LoneSeparatorIsDropped)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, ",");
    EXPECT_TRUE(out.is_null());
}

TEST(UpdateCertIssuerOrSubject, EmptyKeyIsDropped)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, "=value");
    EXPECT_TRUE(out.is_null());

    nlohmann::json empty;
    updateCertIssuerOrSubject(empty, "=");
    EXPECT_TRUE(empty.is_null());
}

TEST(UpdateCertIssuerOrSubject, EmptyValueIsStored)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, "CN=");
    EXPECT_EQ(out, R"({"CommonName": ""})"_json);

    nlohmann::json city;
    updateCertIssuerOrSubject(city, "L=");
    EXPECT_EQ(city, R"({"City": ""})"_json);
}

TEST(UpdateCertIssuerOrSubject, EmptyValueFollowedByAnotherPair)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, "CN=,O=x");
    EXPECT_EQ(out, R"({"CommonName": "", "Organization": "x"})"_json);
}

TEST(UpdateCertIssuerOrSubject, TrailingSeparatorIsConsumed)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, "CN=localhost,");
    EXPECT_EQ(out, R"({"CommonName": "localhost"})"_json);
}

TEST(UpdateCertIssuerOrSubject, LeadingSeparatorCorruptsKey)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, ",CN=localhost");
    EXPECT_TRUE(out.is_null());
}

TEST(UpdateCertIssuerOrSubject, RepeatedSeparatorDropsNextPair)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, "O=x,,CN=y");
    EXPECT_EQ(out, R"({"Organization": "x"})"_json);
}

TEST(UpdateCertIssuerOrSubject, SpaceAfterSeparatorDropsNextPair)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, "O=x, CN=y");
    EXPECT_EQ(out, R"({"Organization": "x"})"_json);
}

TEST(UpdateCertIssuerOrSubject, SpacesAroundEqualsSignDropPair)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, "O = x");
    EXPECT_TRUE(out.is_null());
}

TEST(UpdateCertIssuerOrSubject, KeysAreCaseSensitive)
{
    nlohmann::json lower;
    updateCertIssuerOrSubject(lower, "cn=localhost");
    EXPECT_TRUE(lower.is_null());

    nlohmann::json mixed;
    updateCertIssuerOrSubject(mixed, "Cn=localhost");
    EXPECT_TRUE(mixed.is_null());
}

TEST(UpdateCertIssuerOrSubject, ValueKeepsAdditionalEqualsSigns)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, "CN=a=b");
    EXPECT_EQ(out, R"({"CommonName": "a=b"})"_json);
}

TEST(UpdateCertIssuerOrSubject, ValueWhitespaceIsPreserved)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, "CN=  spaced  ");
    EXPECT_EQ(out, R"({"CommonName": "  spaced  "})"_json);
}

TEST(UpdateCertIssuerOrSubject, ValueMayContainSpaces)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, "ST=New York");
    EXPECT_EQ(out, R"({"State": "New York"})"_json);
}

TEST(UpdateCertIssuerOrSubject, RepeatedKeyKeepsLastValue)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, "CN=first,CN=second");
    EXPECT_EQ(out, R"({"CommonName": "second"})"_json);
}

TEST(UpdateCertIssuerOrSubject, SeparatorInsideValueTruncatesAndDropsRest)
{
    nlohmann::json out;
    updateCertIssuerOrSubject(out, "O=Acme, Inc.,CN=host");
    EXPECT_EQ(out, R"({"Organization": "Acme"})"_json);
}

TEST(UpdateCertIssuerOrSubject, ExistingEntriesArePreservedOrOverwritten)
{
    nlohmann::json out = R"({"Existing": "keep", "City": "OLD"})"_json;
    updateCertIssuerOrSubject(out, "L=New");
    EXPECT_EQ(out, R"({"Existing": "keep", "City": "New"})"_json);
}

} // namespace
} // namespace redfish
