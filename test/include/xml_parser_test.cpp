// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "xml_parser.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace xml
{
namespace
{

using ::testing::ElementsAre;
using ::testing::Optional;

TEST(XmlParser, SelfClosingElement)
{
    std::optional<Element> root = parse("<root/>");
    ASSERT_TRUE(root);
    EXPECT_EQ(root->name, "root");
    EXPECT_TRUE(root->attributes.empty());
    EXPECT_TRUE(root->children.empty());
    EXPECT_TRUE(root->text.empty());
}

TEST(XmlParser, AttributesBothQuoteStyles)
{
    std::optional<Element> root = parse(R"(<a x="1" y='2'>hello</a>)");
    ASSERT_TRUE(root);
    EXPECT_EQ(root->name, "a");
    EXPECT_THAT(root->attributes,
                ElementsAre(Attribute{"x", "1"}, Attribute{"y", "2"}));
    EXPECT_EQ(root->text, "hello");
}

TEST(XmlParser, LeadingAndTrailingWhitespaceIgnored)
{
    Element expected;
    expected.name = "root";
    EXPECT_THAT(parse("  \n <root/> \t\n"), Optional(expected));
}

TEST(XmlParser, SkipsPrologAndComments)
{
    std::string_view doc =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!-- a comment -->\n"
        "<node name=\"/\">\n"
        "  <!-- inline comment -->\n"
        "  <child/>\n"
        "</node>\n";
    std::optional<Element> root = parse(doc);
    ASSERT_TRUE(root);
    EXPECT_EQ(root->name, "node");
    ASSERT_EQ(root->attributes.size(), 1);
    EXPECT_EQ(root->attributes[0].value, "/");
    ASSERT_EQ(root->children.size(), 1);
    EXPECT_EQ(root->children[0].name, "child");
}

TEST(XmlParser, NestedDbusIntrospection)
{
    std::string_view doc =
        "<node name=\"/\">"
        "  <interface name=\"org.freedesktop.DBus\">"
        "    <method name=\"Hello\"/>"
        "  </interface>"
        "  <node name=\"child\"/>"
        "</node>";
    std::optional<Element> root = parse(doc);
    ASSERT_TRUE(root);
    ASSERT_EQ(root->children.size(), 2);

    const Element& iface = root->children[0];
    EXPECT_EQ(iface.name, "interface");
    EXPECT_THAT(iface.attributes,
                ElementsAre(Attribute{"name", "org.freedesktop.DBus"}));
    ASSERT_EQ(iface.children.size(), 1);
    EXPECT_EQ(iface.children[0].name, "method");
    EXPECT_THAT(iface.children[0].attributes,
                ElementsAre(Attribute{"name", "Hello"}));

    EXPECT_EQ(root->children[1].name, "node");
}

TEST(XmlParser, RepeatedNestingOfSameName)
{
    std::optional<Element> root = parse("<a><a><a>x</a></a></a>");
    ASSERT_TRUE(root);
    ASSERT_EQ(root->children.size(), 1);
    ASSERT_EQ(root->children[0].children.size(), 1);
    EXPECT_EQ(root->children[0].children[0].text, "x");
}

TEST(XmlParser, MismatchedCloseTagFails)
{
    EXPECT_FALSE(parse("<a></b>"));
}

TEST(XmlParser, UnclosedElementFails)
{
    EXPECT_FALSE(parse("<a>"));
}

TEST(XmlParser, TrailingGarbageFails)
{
    EXPECT_FALSE(parse("<a/> extra"));
}

TEST(XmlParser, EmptyInputFails)
{
    EXPECT_FALSE(parse(""));
}

} // namespace
} // namespace xml
