#include "bmcweb_config.h"

#include "query_param.hpp"

#include <boost/system/result.hpp>
#include <boost/url/url_view.hpp>
#include <nlohmann/json.hpp>

#include <new>

#include <gmock/gmock.h> // IWYU pragma: keep
#include <gtest/gtest.h> // IWYU pragma: keep

// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"
// IWYU pragma: no_include <boost/url/impl/url_view.hpp>
// IWYU pragma: no_include <gmock/gmock-matchers.h>
// IWYU pragma: no_include <gtest/gtest-matchers.h>

namespace redfish::query_param
{
namespace
{

using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::Key;
using ::testing::UnorderedElementsAre;

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
    EXPECT_EQ(delegated.top, std::nullopt);
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
    EXPECT_EQ(query.top, std::nullopt);
}

TEST(Delegate, SkipNegative)
{
    Query query{
        .skip = 42,
    };
    Query delegated = delegate(QueryCapabilities{}, query);
    EXPECT_EQ(delegated.skip, std::nullopt);
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

TEST(IsSelectedPropertyAllowed, NotAllowedCharactersReturnsFalse)
{
    EXPECT_FALSE(isSelectedPropertyAllowed("?"));
    EXPECT_FALSE(isSelectedPropertyAllowed("!"));
    EXPECT_FALSE(isSelectedPropertyAllowed("-"));
}

TEST(IsSelectedPropertyAllowed, EmptyStringReturnsFalse)
{
    EXPECT_FALSE(isSelectedPropertyAllowed(""));
}

TEST(IsSelectedPropertyAllowed, TooLongStringReturnsFalse)
{
    std::string strUnderTest = "ab";
    // 2^10
    for (int i = 0; i < 10; ++i)
    {
        strUnderTest += strUnderTest;
    }
    EXPECT_FALSE(isSelectedPropertyAllowed(strUnderTest));
}

TEST(IsSelectedPropertyAllowed, ValidPropertReturnsTrue)
{
    EXPECT_TRUE(isSelectedPropertyAllowed("Chassis"));
    EXPECT_TRUE(isSelectedPropertyAllowed("@odata.type"));
    EXPECT_TRUE(isSelectedPropertyAllowed("#ComputerSystem.Reset"));
    EXPECT_TRUE(isSelectedPropertyAllowed(
        "Boot/BootSourceOverrideTarget@Redfish.AllowableValues"));
}

TEST(GetSelectParam, EmptyValueReturnsError)
{
    Query query;
    EXPECT_FALSE(getSelectParam("", query));
}

TEST(GetSelectParam, EmptyPropertyReturnsError)
{
    Query query;
    EXPECT_FALSE(getSelectParam(",", query));
    EXPECT_FALSE(getSelectParam(",,", query));
}

TEST(GetSelectParam, InvalidPathPropertyReturnsError)
{
    Query query;
    EXPECT_FALSE(getSelectParam("\0,\0", query));
    EXPECT_FALSE(getSelectParam("%%%", query));
}

TEST(GetSelectParam, PropertyReturnsOk)
{
    Query query;
    ASSERT_TRUE(getSelectParam("foo/bar,bar", query));
    EXPECT_THAT(query.selectedProperties.children,
                UnorderedElementsAre(Key(Eq("foo")), Key(Eq("bar")), Key(Eq("@odata.id")),
                                     Key(Eq("@odata.type")), Key(Eq("@odata.context")),
                                     Key(Eq("@odata.etag")), Key(Eq("error"))));
    EXPECT_THAT(query.selectedProperties.children["foo"].children,
                UnorderedElementsAre(Key(Eq("bar"))));
    EXPECT_THAT(query.selectedProperties.children["bar"].children, IsEmpty());
}

TEST(RecursiveSelect, ExpectedKeysAreSelectInSimpleObject)
{
    SelectedParameterTrie shouldSelect = {"select_me"};
    nlohmann::json root = R"({"select_me" : "foo", "omit_me" : "bar"})"_json;
    nlohmann::json expected = R"({"select_me" : "foo"})"_json;
    performSelect(root, shouldSelect);
    EXPECT_EQ(root, expected);
}

TEST(RecursiveSelect, ExpectedKeysAreSelectInNestedObject)
{
    SelectedParameterTrie shouldSelect = {"prefix0/explicit_select_me",
                                          "prefix1", "prefix2", "select_me"};
    nlohmann::json root = R"(
{
  "select_me":[
    "foo"
  ],
  "omit_me":"bar",
  "prefix0":{
    "explicit_select_me":"123",
    "omit_me":"456"
  },
  "prefix1":{
    "implicit_select_me":"123"
  },
  "prefix2":[
    {
      "implicit_select_me":"123"
    }
  ],
  "prefix3":[
    "omit_me"
  ]
}
)"_json;
    nlohmann::json expected = R"(
{
  "select_me":[
    "foo"
  ],
  "prefix0":{
    "explicit_select_me":"123"
  },
  "prefix1":{
    "implicit_select_me":"123"
  },
  "prefix2":[
    {
      "implicit_select_me":"123"
    }
  ]
}
)"_json;
    performSelect(root, shouldSelect);
    EXPECT_EQ(root, expected);
}

TEST(RecursiveSelect, OdataPropertiesAreSelected)
{
    nlohmann::json root = R"(
{
  "omit_me":"bar",
  "@odata.id":1,
  "@odata.type":2,
  "@odata.context":3,
  "@odata.etag":4,
  "prefix1":{
    "omit_me":"bar",
    "@odata.id":1
  },
  "prefix2":[1, 2, 3],
  "prefix3":[
    {
      "omit_me":"bar",
      "@odata.id":1
    }
  ]
}
)"_json;
    nlohmann::json expected = R"(
{
  "@odata.id":1,
  "@odata.type":2,
  "@odata.context":3,
  "@odata.etag":4
}
)"_json;
    auto ret = boost::urls::parse_relative_ref("/redfish/v1?$select=abc");
    ASSERT_TRUE(ret);
    crow::Response res;
    std::optional<Query> query = parseParameters(ret->params(), res);
    if constexpr (bmcwebInsecureEnableQueryParams)
    {
        ASSERT_NE(query, std::nullopt);
        performSelect(root, query->selectedProperties);
        EXPECT_EQ(root, expected);
    }
    else
    {
        EXPECT_EQ(query, std::nullopt);
    }
}

TEST(QueryParams, ParseParametersOnly)
{
    auto ret = boost::urls::parse_relative_ref("/redfish/v1?only");
    ASSERT_TRUE(ret);

    crow::Response res;
    std::optional<Query> query = parseParameters(ret->params(), res);
    ASSERT_TRUE(query != std::nullopt);
    EXPECT_TRUE(query->isOnly);
}

TEST(QueryParams, ParseParametersExpand)
{
    auto ret = boost::urls::parse_relative_ref("/redfish/v1?$expand=*");
    ASSERT_TRUE(ret);

    crow::Response res;

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

    std::optional<Query> query = parseParameters(ret->params(), res);
    ASSERT_TRUE(query != std::nullopt);
    EXPECT_EQ(query->top, 1);
}

TEST(QueryParams, ParseParametersTopOutOfRangeNegative)
{
    auto ret = boost::urls::parse_relative_ref("/redfish/v1?$top=-1");
    ASSERT_TRUE(ret);

    crow::Response res;

    std::optional<Query> query = parseParameters(ret->params(), res);
    ASSERT_TRUE(query == std::nullopt);
}

TEST(QueryParams, ParseParametersTopOutOfRangePositive)
{
    auto ret = boost::urls::parse_relative_ref("/redfish/v1?$top=1001");
    ASSERT_TRUE(ret);

    crow::Response res;

    std::optional<Query> query = parseParameters(ret->params(), res);
    ASSERT_TRUE(query == std::nullopt);
}

TEST(QueryParams, ParseParametersSkip)
{
    auto ret = boost::urls::parse_relative_ref("/redfish/v1?$skip=1");
    ASSERT_TRUE(ret);

    crow::Response res;

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

    std::optional<Query> query = parseParameters(ret->params(), res);
    ASSERT_EQ(query, std::nullopt);
}

TEST(QueryParams, ParseParametersUnexpectedGetsIgnored)
{
    auto ret = boost::urls::parse_relative_ref("/redfish/v1?unexpected_param");
    ASSERT_TRUE(ret);

    crow::Response res;

    std::optional<Query> query = parseParameters(ret->params(), res);
    ASSERT_TRUE(query != std::nullopt);
}

TEST(QueryParams, ParseParametersUnexpectedDollarGetsError)
{
    auto ret = boost::urls::parse_relative_ref("/redfish/v1?$unexpected_param");
    ASSERT_TRUE(ret);

    crow::Response res;

    std::optional<Query> query = parseParameters(ret->params(), res);
    ASSERT_TRUE(query == std::nullopt);
    EXPECT_EQ(res.result(), boost::beast::http::status::not_implemented);
}

TEST(QueryParams, GetExpandType)
{
    Query query{};

    EXPECT_FALSE(getExpandType("", query));
    EXPECT_FALSE(getExpandType(".(", query));
    EXPECT_FALSE(getExpandType(".()", query));
    EXPECT_FALSE(getExpandType(".($levels=1", query));

    EXPECT_TRUE(getExpandType("*", query));
    EXPECT_EQ(query.expandType, ExpandType::Both);
    EXPECT_TRUE(getExpandType(".", query));
    EXPECT_EQ(query.expandType, ExpandType::NotLinks);
    EXPECT_TRUE(getExpandType("~", query));
    EXPECT_EQ(query.expandType, ExpandType::Links);

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

TEST(QueryParams, FindNavigationReferencesNonLink)
{
    using nlohmann::json;

    json singleTreeNode = R"({"Foo" : {"@odata.id": "/foobar"}})"_json;

    // Parsing as the root should net one entry
    EXPECT_THAT(findNavigationReferences(ExpandType::Both, singleTreeNode),
                UnorderedElementsAre(
                    ExpandNode{json::json_pointer("/Foo"), "/foobar"}));

    // Parsing in Non-hyperlinks mode should net one entry
    EXPECT_THAT(findNavigationReferences(ExpandType::NotLinks, singleTreeNode),
                UnorderedElementsAre(
                    ExpandNode{json::json_pointer("/Foo"), "/foobar"}));

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
                UnorderedElementsAre(
                    ExpandNode{json::json_pointer("/Foo"), "/foobar"}));
}

TEST(QueryParams, FindNavigationReferencesLink)
{
    using nlohmann::json;

    json singleLinkNode =
        R"({"Links" : {"Sessions": {"@odata.id": "/foobar"}}})"_json;

    // Parsing as the root should net one entry
    EXPECT_THAT(findNavigationReferences(ExpandType::Both, singleLinkNode),
                UnorderedElementsAre(ExpandNode{
                    json::json_pointer("/Links/Sessions"), "/foobar"}));
    // Parsing in hyperlinks mode should net one entry
    EXPECT_THAT(findNavigationReferences(ExpandType::Links, singleLinkNode),
                UnorderedElementsAre(ExpandNode{
                    json::json_pointer("/Links/Sessions"), "/foobar"}));

    // Searching for not types should return empty set
    EXPECT_TRUE(
        findNavigationReferences(ExpandType::None, singleLinkNode).empty());

    // Searching for non-hyperlinks only should return empty set
    EXPECT_TRUE(
        findNavigationReferences(ExpandType::NotLinks, singleLinkNode).empty());
}

} // namespace
} // namespace redfish::query_param
