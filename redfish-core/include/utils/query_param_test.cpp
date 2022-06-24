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

TEST(Delegate, TopNegative)
{
    Query query{
        .top = 42,
    };
    Query delegated = delegate(QueryCapabilities{}, query);
    EXPECT_EQ(delegated.top, maxEntriesPerPage);
    EXPECT_EQ(query.top, 42);
}

TEST(Delegate, TopPositive)
{
    Query query{
        .top = 42,
    };
    QueryCapabilities capabilities{
        .canDelegateTop = true,
    };
    Query delegated = delegate(capabilities, query);
    EXPECT_EQ(delegated.top, 42);
    EXPECT_EQ(query.top, maxEntriesPerPage);
}

TEST(Delegate, SkipNegative)
{
    Query query{
        .skip = 42,
    };
    Query delegated = delegate(QueryCapabilities{}, query);
    EXPECT_EQ(delegated.skip, 0);
    EXPECT_EQ(query.skip, 42);
}

TEST(Delegate, SkipPositive)
{
    Query query{
        .skip = 42,
    };
    QueryCapabilities capabilities{
        .canDelegateSkip = true,
    };
    Query delegated = delegate(capabilities, query);
    EXPECT_EQ(delegated.skip, 42);
    EXPECT_EQ(query.skip, 0);
}

TEST(FormatQueryForExpand, NoSubQueryWhenQueryIsEmpty)
{
    EXPECT_EQ(formatQueryForExpand(Query{}), "");
}

TEST(FormatQueryForExpand, NoSubQueryWhenExpandLevelsLeOne)
{
    EXPECT_EQ(formatQueryForExpand(
                  Query{.expandLevel = 1, .expandType = ExpandType::Both}),
              "");
    EXPECT_EQ(formatQueryForExpand(Query{.expandType = ExpandType::Links}), "");
    EXPECT_EQ(formatQueryForExpand(Query{.expandType = ExpandType::NotLinks}),
              "");
}

TEST(FormatQueryForExpand, NoSubQueryWhenExpandTypeIsNone)
{
    EXPECT_EQ(formatQueryForExpand(
                  Query{.expandLevel = 2, .expandType = ExpandType::None}),
              "");
}

TEST(FormatQueryForExpand, DelegatedSubQueriesHaveSameTypeAndOneLessLevels)
{
    EXPECT_EQ(formatQueryForExpand(
                  Query{.expandLevel = 3, .expandType = ExpandType::Both}),
              "?$expand=*($levels=2)");
    EXPECT_EQ(formatQueryForExpand(
                  Query{.expandLevel = 4, .expandType = ExpandType::Links}),
              "?$expand=~($levels=3)");
    EXPECT_EQ(formatQueryForExpand(
                  Query{.expandLevel = 2, .expandType = ExpandType::NotLinks}),
              "?$expand=.($levels=1)");
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
    if constexpr (bmcwebInsecureEnableQueryParams)
    {
        ASSERT_NE(query, std::nullopt);
        EXPECT_TRUE(query->expandType ==
                    redfish::query_param::ExpandType::Both);
    }
    else
    {
        ASSERT_EQ(query, std::nullopt);
    }
}

TEST(QueryParams, ParseParametersTop)
{
    auto ret = boost::urls::parse_relative_ref("/redfish/v1?$top=1");
    ASSERT_TRUE(ret);

    crow::Response res;

    using redfish::query_param::parseParameters;
    using redfish::query_param::Query;
    std::optional<Query> query = parseParameters(ret->params(), res);
    ASSERT_TRUE(query != std::nullopt);
    EXPECT_EQ(query->top, 1);
}

TEST(QueryParams, ParseParametersTopOutOfRangeNegative)
{
    auto ret = boost::urls::parse_relative_ref("/redfish/v1?$top=-1");
    ASSERT_TRUE(ret);

    crow::Response res;

    using redfish::query_param::parseParameters;
    using redfish::query_param::Query;
    std::optional<Query> query = parseParameters(ret->params(), res);
    ASSERT_TRUE(query == std::nullopt);
}

TEST(QueryParams, ParseParametersTopOutOfRangePositive)
{
    auto ret = boost::urls::parse_relative_ref("/redfish/v1?$top=1001");
    ASSERT_TRUE(ret);

    crow::Response res;

    using redfish::query_param::parseParameters;
    using redfish::query_param::Query;
    std::optional<Query> query = parseParameters(ret->params(), res);
    ASSERT_TRUE(query == std::nullopt);
}

TEST(QueryParams, ParseParametersSkip)
{
    auto ret = boost::urls::parse_relative_ref("/redfish/v1?$skip=1");
    ASSERT_TRUE(ret);

    crow::Response res;

    using redfish::query_param::parseParameters;
    using redfish::query_param::Query;
    std::optional<Query> query = parseParameters(ret->params(), res);
    ASSERT_TRUE(query != std::nullopt);
    EXPECT_EQ(query->skip, 1);
}
TEST(QueryParams, ParseParametersSkipOutOfRange)
{
    auto ret = boost::urls::parse_relative_ref(
        "/redfish/v1?$skip=99999999999999999999");
    ASSERT_TRUE(ret);

    crow::Response res;

    using redfish::query_param::parseParameters;
    using redfish::query_param::Query;
    std::optional<Query> query = parseParameters(ret->params(), res);
    ASSERT_EQ(query, std::nullopt);
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
static void PrintTo(const ExpandNode& value, ::std::ostream* os)
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
    EXPECT_THAT(findNavigationReferences(ExpandType::Both, singleTreeNode),
                UnorderedElementsAre(redfish::query_param::ExpandNode{
                    json::json_pointer("/Foo"), "/foobar"}));

    // Parsing in Non-hyperlinks mode should net one entry
    EXPECT_THAT(findNavigationReferences(ExpandType::NotLinks, singleTreeNode),
                UnorderedElementsAre(redfish::query_param::ExpandNode{
                    json::json_pointer("/Foo"), "/foobar"}));

    // Searching for not types should return empty set
    EXPECT_TRUE(
        findNavigationReferences(ExpandType::None, singleTreeNode).empty());

    // Searching for hyperlinks only should return empty set
    EXPECT_TRUE(
        findNavigationReferences(ExpandType::Links, singleTreeNode).empty());

    json multiTreeNodes =
        R"({"Links": {"@odata.id": "/links"}, "Foo" : {"@odata.id": "/foobar"}})"_json;
    // Should still find Foo
    EXPECT_THAT(findNavigationReferences(ExpandType::NotLinks, multiTreeNodes),
                UnorderedElementsAre(redfish::query_param::ExpandNode{
                    json::json_pointer("/Foo"), "/foobar"}));
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
    EXPECT_THAT(findNavigationReferences(ExpandType::Both, singleLinkNode),
                UnorderedElementsAre(redfish::query_param::ExpandNode{
                    json::json_pointer("/Links/Sessions"), "/foobar"}));
    // Parsing in hyperlinks mode should net one entry
    EXPECT_THAT(findNavigationReferences(ExpandType::Links, singleLinkNode),
                UnorderedElementsAre(redfish::query_param::ExpandNode{
                    json::json_pointer("/Links/Sessions"), "/foobar"}));

    // Searching for not types should return empty set
    EXPECT_TRUE(
        findNavigationReferences(ExpandType::None, singleLinkNode).empty());

    // Searching for non-hyperlinks only should return empty set
    EXPECT_TRUE(
        findNavigationReferences(ExpandType::NotLinks, singleLinkNode).empty());
}
