#include "query_param.hpp"

#include <nlohmann/json.hpp>

#include <iostream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace redfish::query_param
{
namespace
{

TEST(Delegate, OnlyPositive)
{
    Query query{
        .isOnly = true,
    };
    QueryCapabilities capabilities{
        .canDelegateOnly = true,
    };
    Query delegated = delegate(capabilities, query);
    EXPECT_TRUE(delegated.isOnly);
    EXPECT_FALSE(query.isOnly);
}

TEST(Delegate, ExpandPositive)
{
    Query query{
        .isOnly = false,
        .expandLevel = 5,
        .expandType = ExpandType::Both,
    };
    QueryCapabilities capabilities{
        .canDelegateExpandLevel = 3,
    };
    Query delegated = delegate(capabilities, query);
    EXPECT_FALSE(delegated.isOnly);
    EXPECT_EQ(delegated.expandLevel, capabilities.canDelegateExpandLevel);
    EXPECT_EQ(delegated.expandType, ExpandType::Both);
    EXPECT_EQ(query.expandLevel, 2);
}

TEST(Delegate, OnlyNegative)
{
    Query query{
        .isOnly = true,
    };
    QueryCapabilities capabilities{
        .canDelegateOnly = false,
    };
    Query delegated = delegate(capabilities, query);
    EXPECT_FALSE(delegated.isOnly);
    EXPECT_EQ(query.isOnly, true);
}

TEST(Delegate, ExpandNegative)
{
    Query query{
        .isOnly = false,
        .expandType = ExpandType::None,
    };
    Query delegated = delegate(QueryCapabilities{}, query);
    EXPECT_EQ(delegated.expandType, ExpandType::None);
}

} // namespace
} // namespace redfish::query_param

TEST(QueryParams, ParseParametersOnly)
{
    auto ret = boost::urls::parse_relative_ref("/redfish/v1?only");
    ASSERT_TRUE(ret);

    crow::Response res;

    using redfish::query_param::parseParameters;
    using redfish::query_param::Query;
    std::optional<Query> query = parseParameters(ret->params(), res);
    ASSERT_TRUE(query != std::nullopt);
    EXPECT_TRUE(query->isOnly);
}

TEST(QueryParams, ParseParametersExpand)
{
    auto ret = boost::urls::parse_relative_ref("/redfish/v1?$expand=*");
    ASSERT_TRUE(ret);

    crow::Response res;

    using redfish::query_param::parseParameters;
    using redfish::query_param::Query;
    std::optional<Query> query = parseParameters(ret->params(), res);
    ASSERT_TRUE(query != std::nullopt);
    EXPECT_TRUE(query->expandType == redfish::query_param::ExpandType::Both);
}

TEST(QueryParams, ParseParametersUnexpectedGetsIgnored)
{
    auto ret = boost::urls::parse_relative_ref("/redfish/v1?unexpected_param");
    ASSERT_TRUE(ret);

    crow::Response res;

    using redfish::query_param::parseParameters;
    using redfish::query_param::Query;
    std::optional<Query> query = parseParameters(ret->params(), res);
    ASSERT_TRUE(query != std::nullopt);
}

TEST(QueryParams, ParseParametersUnexpectedDollarGetsError)
{
    auto ret = boost::urls::parse_relative_ref("/redfish/v1?$unexpected_param");
    ASSERT_TRUE(ret);

    crow::Response res;

    using redfish::query_param::parseParameters;
    using redfish::query_param::Query;
    std::optional<Query> query = parseParameters(ret->params(), res);
    ASSERT_TRUE(query == std::nullopt);
    EXPECT_EQ(res.result(), boost::beast::http::status::not_implemented);
}

TEST(QueryParams, GetExpandType)
{
    redfish::query_param::Query query{};

    EXPECT_FALSE(getExpandType("", query));
    EXPECT_FALSE(getExpandType(".(", query));
    EXPECT_FALSE(getExpandType(".()", query));
    EXPECT_FALSE(getExpandType(".($levels=1", query));

    EXPECT_TRUE(getExpandType("*", query));
    EXPECT_EQ(query.expandType, redfish::query_param::ExpandType::Both);
    EXPECT_TRUE(getExpandType(".", query));
    EXPECT_EQ(query.expandType, redfish::query_param::ExpandType::NotLinks);
    EXPECT_TRUE(getExpandType("~", query));
    EXPECT_EQ(query.expandType, redfish::query_param::ExpandType::Links);

    // Per redfish specification, level defaults to 1
    EXPECT_TRUE(getExpandType(".", query));
    EXPECT_EQ(query.expandLevel, 1);

    EXPECT_TRUE(getExpandType(".($levels=42)", query));
    EXPECT_EQ(query.expandLevel, 42);

    // Overflow
    EXPECT_FALSE(getExpandType(".($levels=256)", query));

    // Negative
    EXPECT_FALSE(getExpandType(".($levels=-1)", query));

    // No number
    EXPECT_FALSE(getExpandType(".($levels=a)", query));
}

namespace redfish::query_param
{
// NOLINTNEXTLINE(readability-identifier-naming)
void PrintTo(const ExpandNode& value, ::std::ostream* os)
{
    *os << "ExpandNode: " << value.location << " " << value.uri;
}
}; // namespace redfish::query_param

TEST(QueryParams, FindNavigationReferencesNonLink)
{
    using nlohmann::json;
    using redfish::query_param::ExpandType;
    using redfish::query_param::findNavigationReferences;
    using ::testing::UnorderedElementsAre;
    json singleTreeNode = R"({"Foo" : {"@odata.id": "/foobar"}})"_json;

    // Parsing as the root should net one entry
    EXPECT_THAT(findNavigationReferences(ExpandType::Both, singleTreeNode,
                                         json::json_pointer("")),
                UnorderedElementsAre(redfish::query_param::ExpandNode{
                    json::json_pointer("/Foo"), "/foobar"}));
    // Parsing at a depth should net one entry at depth
    EXPECT_THAT(findNavigationReferences(ExpandType::Both, singleTreeNode,
                                         json::json_pointer("/baz")),
                UnorderedElementsAre(redfish::query_param::ExpandNode{
                    json::json_pointer("/baz/Foo"), "/foobar"}));

    // Parsing in Non-hyperlinks mode should net one entry
    EXPECT_THAT(findNavigationReferences(ExpandType::NotLinks, singleTreeNode,
                                         json::json_pointer("")),
                UnorderedElementsAre(redfish::query_param::ExpandNode{
                    json::json_pointer("/Foo"), "/foobar"}));

    // Parsing non-hyperlinks at depth should net one entry at depth
    EXPECT_THAT(findNavigationReferences(ExpandType::NotLinks, singleTreeNode,
                                         json::json_pointer("/baz")),
                UnorderedElementsAre(redfish::query_param::ExpandNode{
                    json::json_pointer("/baz/Foo"), "/foobar"}));

    // Searching for not types should return empty set
    EXPECT_TRUE(findNavigationReferences(ExpandType::None, singleTreeNode,
                                         json::json_pointer(""))
                    .empty());

    // Searching for hyperlinks only should return empty set
    EXPECT_TRUE(findNavigationReferences(ExpandType::Links, singleTreeNode,
                                         json::json_pointer(""))
                    .empty());
}

TEST(QueryParams, FindNavigationReferencesLink)
{
    using nlohmann::json;
    using redfish::query_param::ExpandType;
    using redfish::query_param::findNavigationReferences;
    using ::testing::UnorderedElementsAre;
    json singleLinkNode =
        R"({"Links" : {"Sessions": {"@odata.id": "/foobar"}}})"_json;

    // Parsing as the root should net one entry
    EXPECT_THAT(findNavigationReferences(ExpandType::Both, singleLinkNode,
                                         json::json_pointer("")),
                UnorderedElementsAre(redfish::query_param::ExpandNode{
                    json::json_pointer("/Links/Sessions"), "/foobar"}));
    // Parsing at a depth should net one entry at depth
    EXPECT_THAT(findNavigationReferences(ExpandType::Both, singleLinkNode,
                                         json::json_pointer("/baz")),
                UnorderedElementsAre(redfish::query_param::ExpandNode{
                    json::json_pointer("/baz/Links/Sessions"), "/foobar"}));

    // Parsing in hyperlinks mode should net one entry
    EXPECT_THAT(findNavigationReferences(ExpandType::Links, singleLinkNode,
                                         json::json_pointer("")),
                UnorderedElementsAre(redfish::query_param::ExpandNode{
                    json::json_pointer("/Links/Sessions"), "/foobar"}));

    // Parsing hyperlinks at depth should net one entry at depth
    EXPECT_THAT(findNavigationReferences(ExpandType::Links, singleLinkNode,
                                         json::json_pointer("/baz")),
                UnorderedElementsAre(redfish::query_param::ExpandNode{
                    json::json_pointer("/baz/Links/Sessions"), "/foobar"}));

    // Searching for not types should return empty set
    EXPECT_TRUE(findNavigationReferences(ExpandType::None, singleLinkNode,
                                         json::json_pointer(""))
                    .empty());

    // Searching for non-hyperlinks only should return empty set
    EXPECT_TRUE(findNavigationReferences(ExpandType::NotLinks, singleLinkNode,
                                         json::json_pointer(""))
                    .empty());
}
