// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "utils/json_utils.hpp"
#include "utils/query_param.hpp"

#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/result.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/url_view.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace redfish::query_param
{
namespace
{

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
    EXPECT_EQ(query.expandLevel, 5);
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
    EXPECT_FALSE(isSelectedPropertyAllowed("/"));
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
        "BootSourceOverrideTarget@Redfish.AllowableValues"));
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

TEST(GetSelectParam, TrieNodesRespectAllProperties)
{
    Query query;
    ASSERT_TRUE(getSelectParam("foo/bar,bar", query));
    ASSERT_FALSE(query.selectTrie.root.empty());

    const SelectTrieNode* child = query.selectTrie.root.find("foo");
    ASSERT_NE(child, nullptr);
    EXPECT_FALSE(child->isSelected());
    ASSERT_NE(child->find("bar"), nullptr);
    EXPECT_TRUE(child->find("bar")->isSelected());

    ASSERT_NE(query.selectTrie.root.find("bar"), nullptr);
    EXPECT_TRUE(query.selectTrie.root.find("bar")->isSelected());
}

SelectTrie getTrie(std::span<std::string_view> properties)
{
    SelectTrie trie;
    for (const auto& property : properties)
    {
        EXPECT_TRUE(trie.insertNode(property));
    }
    return trie;
}

TEST(RecursiveSelect, ExpectedKeysAreSelectInSimpleObject)
{
    std::vector<std::string_view> properties = {"SelectMe"};
    SelectTrie trie = getTrie(properties);
    nlohmann::json root = R"({"SelectMe" : "foo", "OmitMe" : "bar"})"_json;
    nlohmann::json expected = R"({"SelectMe" : "foo"})"_json;
    recursiveSelect(root, trie.root);
    EXPECT_EQ(root, expected);
}

TEST(RecursiveSelect, ExpectedKeysAreSelectInNestedObject)
{
    std::vector<std::string_view> properties = {
        "SelectMe", "Prefix0/ExplicitSelectMe", "Prefix1", "Prefix2",
        "Prefix4/ExplicitSelectMe"};
    SelectTrie trie = getTrie(properties);
    nlohmann::json root = R"(
{
  "SelectMe":[
    "foo"
  ],
  "OmitMe":"bar",
  "Prefix0":{
    "ExplicitSelectMe":"123",
    "OmitMe":"456"
  },
  "Prefix1":{
    "ImplicitSelectMe":"123"
  },
  "Prefix2":[
    {
      "ImplicitSelectMe":"123"
    }
  ],
  "Prefix3":[
    "OmitMe"
  ],
  "Prefix4":[
    {
      "ExplicitSelectMe":"123",
      "OmitMe": "456"
    }
  ]
}
)"_json;
    nlohmann::json expected = R"(
{
  "SelectMe":[
    "foo"
  ],
  "Prefix0":{
    "ExplicitSelectMe":"123"
  },
  "Prefix1":{
    "ImplicitSelectMe":"123"
  },
  "Prefix2":[
    {
      "ImplicitSelectMe":"123"
    }
  ],
  "Prefix4":[
    {
      "ExplicitSelectMe":"123"
    }
  ]
}
)"_json;
    recursiveSelect(root, trie.root);
    EXPECT_EQ(root, expected);
}

TEST(RecursiveSelect, ReservedPropertiesAreSelected)
{
    nlohmann::json root = R"(
{
  "OmitMe":"bar",
  "@odata.id":1,
  "@odata.type":2,
  "@odata.context":3,
  "@odata.etag":4,
  "Prefix1":{
    "OmitMe":"bar",
    "@odata.id":1,
    "ExplicitSelectMe": 1
  },
  "Prefix2":[1, 2, 3],
  "Prefix3":[
    {
      "OmitMe":"bar",
      "@odata.id":1,
      "ExplicitSelectMe": 1
    }
  ]
}
)"_json;
    nlohmann::json expected = R"(
{
  "@odata.id":1,
  "@odata.type":2,
  "@odata.context":3,
  "@odata.etag":4,
  "Prefix1":{
    "@odata.id":1,
    "ExplicitSelectMe": 1
  },
  "Prefix3":[
    {
      "@odata.id":1,
      "ExplicitSelectMe": 1
    }
  ]
}
)"_json;
    auto ret = boost::urls::parse_relative_ref(
        "/redfish/v1?$select=Prefix1/ExplicitSelectMe,Prefix3/ExplicitSelectMe");
    ASSERT_TRUE(ret);
    crow::Response res;
    std::optional<Query> query = parseParameters(ret->params(), res);
    ASSERT_TRUE(query);
    if (!query)
    {
        return;
    }
    recursiveSelect(root, query.value().selectTrie.root);
    EXPECT_EQ(root, expected);
}

TEST(QueryParams, ParseParametersOnly)
{
    auto ret = boost::urls::parse_relative_ref("/redfish/v1?only");
    ASSERT_TRUE(ret);
    if (!ret)
    {
        return;
    }

    crow::Response res;
    std::optional<Query> query = parseParameters(ret->params(), res);
    ASSERT_TRUE(query);
    if (!query)
    {
        return;
    }
    EXPECT_TRUE(query->isOnly);
}

TEST(QueryParams, ParseParametersExpand)
{
    auto ret = boost::urls::parse_relative_ref("/redfish/v1?$expand=*");
    ASSERT_TRUE(ret);
    if (!ret)
    {
        return;
    }

    crow::Response res;

    std::optional<Query> query = parseParameters(ret->params(), res);
    if constexpr (BMCWEB_INSECURE_ENABLE_REDFISH_QUERY)
    {
        ASSERT_TRUE(query);
        if (!query)
        {
            return;
        }
        EXPECT_TRUE(query.value().expandType ==
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
    if (!ret)
    {
        return;
    }

    crow::Response res;

    std::optional<Query> query = parseParameters(ret->params(), res);
    ASSERT_TRUE(query);
    if (!query)
    {
        return;
    }
    EXPECT_EQ(query.value().top, 1);
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
    if (!ret)
    {
        return;
    }
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
    ASSERT_TRUE(query);
    if (!query)
    {
        return;
    }
    EXPECT_EQ(query.value().skip, 1);
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

    // Responses must include their "@odata.id" property for $expand to work
    // correctly
    json singleTreeNode =
        R"({"@odata.id": "/redfish/v1",
        "Foo" : {"@odata.id": "/foobar"}})"_json;

    // Parsing as the root should net one entry
    EXPECT_THAT(
        findNavigationReferences(ExpandType::Both, 1, 0, singleTreeNode),
        UnorderedElementsAre(
            ExpandNode{json::json_pointer("/Foo"), "/foobar"}));

    // Parsing in Non-hyperlinks mode should net one entry
    EXPECT_THAT(
        findNavigationReferences(ExpandType::NotLinks, 1, 0, singleTreeNode),
        UnorderedElementsAre(
            ExpandNode{json::json_pointer("/Foo"), "/foobar"}));

    // Searching for not types should return empty set
    EXPECT_TRUE(findNavigationReferences(ExpandType::None, 1, 0, singleTreeNode)
                    .empty());

    // Searching for hyperlinks only should return empty set
    EXPECT_TRUE(
        findNavigationReferences(ExpandType::Links, 1, 0, singleTreeNode)
            .empty());

    // Responses must include their "@odata.id" property for $expand to work
    // correctly
    json multiTreeNodes =
        R"({"@odata.id": "/redfish/v1",
        "Links": {"@odata.id": "/links"},
        "Foo" : {"@odata.id": "/foobar"}})"_json;

    // Should still find Foo
    EXPECT_THAT(
        findNavigationReferences(ExpandType::NotLinks, 1, 0, multiTreeNodes),
        UnorderedElementsAre(
            ExpandNode{json::json_pointer("/Foo"), "/foobar"}));
}

TEST(QueryParams, FindNavigationReferencesLink)
{
    using nlohmann::json;

    // Responses must include their "@odata.id" property for $expand to work
    // correctly
    json singleLinkNode =
        R"({"@odata.id": "/redfish/v1",
        "Links" : {"Sessions": {"@odata.id": "/foobar"}}})"_json;

    // Parsing as the root should net one entry
    EXPECT_THAT(
        findNavigationReferences(ExpandType::Both, 1, 0, singleLinkNode),
        UnorderedElementsAre(
            ExpandNode{json::json_pointer("/Links/Sessions"), "/foobar"}));
    // Parsing in hyperlinks mode should net one entry
    EXPECT_THAT(
        findNavigationReferences(ExpandType::Links, 1, 0, singleLinkNode),
        UnorderedElementsAre(
            ExpandNode{json::json_pointer("/Links/Sessions"), "/foobar"}));

    // Searching for not types should return empty set
    EXPECT_TRUE(findNavigationReferences(ExpandType::None, 1, 0, singleLinkNode)
                    .empty());

    // Searching for non-hyperlinks only should return empty set
    EXPECT_TRUE(
        findNavigationReferences(ExpandType::NotLinks, 1, 0, singleLinkNode)
            .empty());
}

TEST(QueryParams, PreviouslyExpanded)
{
    using nlohmann::json;

    // Responses must include their "@odata.id" property for $expand to work
    // correctly
    json expNode = json::parse(R"(
{
  "@odata.id": "/redfish/v1/Chassis",
  "@odata.type": "#ChassisCollection.ChassisCollection",
  "Members": [
    {
      "@odata.id": "/redfish/v1/Chassis/5B247A_Sat1",
      "@odata.type": "#Chassis.v1_17_0.Chassis",
      "Sensors": {
        "@odata.id": "/redfish/v1/Chassis/5B247A_Sat1/Sensors"
      }
    },
    {
      "@odata.id": "/redfish/v1/Chassis/5B247A_Sat2",
      "@odata.type": "#Chassis.v1_17_0.Chassis",
      "Sensors": {
        "@odata.id": "/redfish/v1/Chassis/5B247A_Sat2/Sensors"
      }
    }
  ],
  "Members@odata.count": 2,
  "Name": "Chassis Collection"
}
)",
                               nullptr, false);

    // Expand has already occurred so we should not do anything
    EXPECT_TRUE(
        findNavigationReferences(ExpandType::NotLinks, 1, 0, expNode).empty());

    // Previous expand was only a single level so we should further expand
    EXPECT_THAT(findNavigationReferences(ExpandType::NotLinks, 2, 0, expNode),
                UnorderedElementsAre(
                    ExpandNode{json::json_pointer("/Members/0/Sensors"),
                               "/redfish/v1/Chassis/5B247A_Sat1/Sensors"},
                    ExpandNode{json::json_pointer("/Members/1/Sensors"),
                               "/redfish/v1/Chassis/5B247A_Sat2/Sensors"}));

    // Make sure we can handle when an array was expanded further down the tree
    json expNode2 = R"({"@odata.id" : "/redfish/v1"})"_json;
    expNode2["Chassis"] = std::move(expNode);
    EXPECT_TRUE(
        findNavigationReferences(ExpandType::NotLinks, 1, 0, expNode2).empty());
    EXPECT_TRUE(
        findNavigationReferences(ExpandType::NotLinks, 2, 0, expNode2).empty());

    // Previous expand was two levels so we should further expand
    EXPECT_THAT(findNavigationReferences(ExpandType::NotLinks, 3, 0, expNode2),
                UnorderedElementsAre(
                    ExpandNode{json::json_pointer("/Chassis/Members/0/Sensors"),
                               "/redfish/v1/Chassis/5B247A_Sat1/Sensors"},
                    ExpandNode{json::json_pointer("/Chassis/Members/1/Sensors"),
                               "/redfish/v1/Chassis/5B247A_Sat2/Sensors"}));
}

TEST(QueryParams, DelegatedSkipExpanded)
{
    using nlohmann::json;

    // Responses must include their "@odata.id" property for $expand to work
    // correctly
    json expNode = json::parse(R"(
{
  "@odata.id": "/redfish/v1",
  "Foo": {
    "@odata.id": "/foo"
  },
  "Bar": {
    "@odata.id": "/bar",
    "Foo": {
      "@odata.id": "/barfoo"
    }
  }
}
)",
                               nullptr, false);

    EXPECT_THAT(findNavigationReferences(ExpandType::NotLinks, 2, 0, expNode),
                UnorderedElementsAre(
                    ExpandNode{json::json_pointer("/Foo"), "/foo"},
                    ExpandNode{json::json_pointer("/Bar/Foo"), "/barfoo"}));

    // Skip the first expand level
    EXPECT_THAT(findNavigationReferences(ExpandType::NotLinks, 1, 1, expNode),
                UnorderedElementsAre(
                    ExpandNode{json::json_pointer("/Bar/Foo"), "/barfoo"}));
}

TEST(QueryParams, PartiallyPreviouslyExpanded)
{
    using nlohmann::json;

    // Responses must include their "@odata.id" property for $expand to work
    // correctly
    json expNode = json::parse(R"(
{
  "@odata.id": "/redfish/v1/Chassis",
  "@odata.type": "#ChassisCollection.ChassisCollection",
  "Members": [
    {
      "@odata.id": "/redfish/v1/Chassis/Local"
    },
    {
      "@odata.id": "/redfish/v1/Chassis/5B247A_Sat1",
      "@odata.type": "#Chassis.v1_17_0.Chassis",
      "Sensors": {
        "@odata.id": "/redfish/v1/Chassis/5B247A_Sat1/Sensors"
      }
    }
  ],
  "Members@odata.count": 2,
  "Name": "Chassis Collection"
}
)",
                               nullptr, false);

    // The 5B247A_Sat1 Chassis was already expanded a single level so we should
    // only want to expand the Local Chassis
    EXPECT_THAT(
        findNavigationReferences(ExpandType::NotLinks, 1, 0, expNode),
        UnorderedElementsAre(ExpandNode{json::json_pointer("/Members/0"),
                                        "/redfish/v1/Chassis/Local"}));

    // The 5B247A_Sat1 Chassis was already expanded a single level so we should
    // further expand it as well as the Local Chassis
    EXPECT_THAT(findNavigationReferences(ExpandType::NotLinks, 2, 0, expNode),
                UnorderedElementsAre(
                    ExpandNode{json::json_pointer("/Members/0"),
                               "/redfish/v1/Chassis/Local"},
                    ExpandNode{json::json_pointer("/Members/1/Sensors"),
                               "/redfish/v1/Chassis/5B247A_Sat1/Sensors"}));

    // Now the response has paths that have been expanded 0, 1, and 2 times
    json expNode2 = R"({"@odata.id" : "/redfish/v1",
                        "Systems": {"@odata.id": "/redfish/v1/Systems"}})"_json;
    expNode2["Chassis"] = std::move(expNode);

    EXPECT_THAT(findNavigationReferences(ExpandType::NotLinks, 1, 0, expNode2),
                UnorderedElementsAre(ExpandNode{json::json_pointer("/Systems"),
                                                "/redfish/v1/Systems"}));

    EXPECT_THAT(
        findNavigationReferences(ExpandType::NotLinks, 2, 0, expNode2),
        UnorderedElementsAre(
            ExpandNode{json::json_pointer("/Systems"), "/redfish/v1/Systems"},
            ExpandNode{json::json_pointer("/Chassis/Members/0"),
                       "/redfish/v1/Chassis/Local"}));

    EXPECT_THAT(
        findNavigationReferences(ExpandType::NotLinks, 3, 0, expNode2),
        UnorderedElementsAre(
            ExpandNode{json::json_pointer("/Systems"), "/redfish/v1/Systems"},
            ExpandNode{json::json_pointer("/Chassis/Members/0"),
                       "/redfish/v1/Chassis/Local"},
            ExpandNode{json::json_pointer("/Chassis/Members/1/Sensors"),
                       "/redfish/v1/Chassis/5B247A_Sat1/Sensors"}));
}

TEST(MultiAsyncResp, PlaceResultChargesBytesWhenChargeBytesTrue)
{
    auto finalRes = std::make_shared<bmcweb::AsyncResp>();
    auto multi = std::make_shared<MultiAsyncResp>(finalRes);

    crow::Response child;
    child.jsonValue = nlohmann::json{{"Foo", "Bar"}};

    uint64_t before = expandBytesInFlight();
    EXPECT_TRUE(
        multi->placeResult(nlohmann::json::json_pointer("/Child"), child,
                           MultiAsyncResp::BudgetAction::Charge));
    EXPECT_EQ(finalRes->res.jsonValue["Child"]["Foo"], "Bar");
    EXPECT_GT(expandBytesInFlight(), before);
}

// OEM fragment merging always uses BudgetAction::Skip, so the gauge must not
// change and the merge must succeed regardless of payload size or any
// concurrent $expand tree's budget state. Uses a small fixed payload rather
// than one sized from crow::httpResponseBodyLimit -- that value is a Meson
// option that can be configured up to 512 MiB (or disabled via 0), and Skip
// bypasses size checks entirely, so no large allocation is needed to prove
// that.
TEST(MultiAsyncResp, PlaceResultWithBudgetActionSkipOemPathDoesNotChargeBudget)
{
    auto finalRes = std::make_shared<bmcweb::AsyncResp>();
    auto multi = std::make_shared<MultiAsyncResp>(finalRes);

    std::string smallValue = "some OEM fragment data";
    crow::Response child;
    child.jsonValue = nlohmann::json{{"Foo", smallValue}};

    uint64_t before = expandBytesInFlight();
    EXPECT_TRUE(multi->placeResult(nlohmann::json::json_pointer("/Child"),
                                   child, MultiAsyncResp::BudgetAction::Skip));
    EXPECT_EQ(expandBytesInFlight(), before);
    EXPECT_EQ(finalRes->res.jsonValue["Child"]["Foo"], smallValue);
}

// An OEM fragment merge (BudgetAction::Skip) must succeed even when a
// concurrent $expand tree has already exhausted the global budget.
TEST(MultiAsyncResp,
     PlaceResultWithBudgetActionSkipSucceedsWhenGlobalBudgetExhausted)
{
    auto finalRes = std::make_shared<bmcweb::AsyncResp>();
    auto multi = std::make_shared<MultiAsyncResp>(finalRes);

    uint64_t before = expandBytesInFlight();
    expandBytesInFlight() += crow::expandGlobalBodyLimit;

    crow::Response child;
    child.jsonValue = nlohmann::json{{"Foo", "Bar"}};
    EXPECT_TRUE(multi->placeResult(nlohmann::json::json_pointer("/Child"),
                                   child, MultiAsyncResp::BudgetAction::Skip));
    EXPECT_EQ(finalRes->res.jsonValue["Child"]["Foo"], "Bar");
    EXPECT_EQ(finalRes->res.result(), boost::beast::http::status::ok);

    expandBytesInFlight() = before;
}

// Primes |multi|'s per-request budget to one byte under the limit without
// allocating a limit-sized payload. Return false when this build's configured
// limits cannot exercise the per-request check.
bool primeBudgetJustUnderLimit(
    const std::shared_ptr<MultiAsyncResp>& multi,
    const std::shared_ptr<bmcweb::AsyncResp>& finalRes)
{
    if (crow::httpResponseBodyLimit == 0)
    {
        return false;
    }

    uint64_t initialResponseSize =
        json_util::getEstimatedJsonSize(finalRes->res.jsonValue);
    if (initialResponseSize >= crow::httpResponseBodyLimit)
    {
        return false;
    }

    uint64_t precharge = crow::httpResponseBodyLimit - initialResponseSize - 1;
    uint64_t globalBefore = expandBytesInFlight();
    if (crow::expandGlobalBodyLimit > 0 &&
        (globalBefore >= crow::expandGlobalBodyLimit ||
         crow::httpResponseBodyLimit - 1 >=
             crow::expandGlobalBodyLimit - globalBefore))
    {
        return false;
    }

    auto budget = makeExpandBudget();
    *budget = precharge;
    expandBytesInFlight() += precharge;
    auto context = std::make_shared<ExpandContext>(ExpandContext{
        .payloadUsed = budget,
        .budgetExceeded = std::make_shared<bool>(false),
    });
    Query query;
    Query delegated;
    crow::Request req;
    multi->startQuery(query, delegated, req, context);
    EXPECT_EQ(*budget, crow::httpResponseBodyLimit - 1);
    EXPECT_EQ(expandBytesInFlight(),
              globalBefore + crow::httpResponseBodyLimit - 1);
    return true;
}

// Errors from sibling responses must still be propagated to the final response
// even after the budget has been exceeded (budgetExceeded=true).
TEST(MultiAsyncResp, BudgetExceededStillPropagatesSiblingErrors)
{
    auto finalRes = std::make_shared<bmcweb::AsyncResp>();
    auto multi = std::make_shared<MultiAsyncResp>(finalRes);
    if (!primeBudgetJustUnderLimit(multi, finalRes))
    {
        GTEST_SKIP() << "configured limits cannot exercise this test";
    }

    // Trip the budget
    crow::Response overLimit;
    overLimit.jsonValue = nlohmann::json{{"X", "over the limit"}};
    ASSERT_FALSE(
        multi->placeResult(nlohmann::json::json_pointer("/A"), overLimit,
                           MultiAsyncResp::BudgetAction::Charge));

    // Now send a sibling with a non-507 error — it must still be merged
    crow::Response errResp;
    errResp.result(boost::beast::http::status::internal_server_error);
    messages::internalError(errResp);
    EXPECT_FALSE(multi->placeResult(nlohmann::json::json_pointer("/B"), errResp,
                                    MultiAsyncResp::BudgetAction::Charge));
    // The 500 error must have been propagated to finalRes
    EXPECT_EQ(finalRes->res.result(),
              boost::beast::http::status::internal_server_error);
}

// A 507 returned by an aggregator or another route is an ordinary child
// error, not proof that this expand tree exhausted its own shared budget.
TEST(MultiAsyncResp, ForeignInsufficientStorageDoesNotStopSibling)
{
    auto finalRes = std::make_shared<bmcweb::AsyncResp>();
    auto multi = std::make_shared<MultiAsyncResp>(finalRes);

    crow::Response foreignError;
    messages::insufficientStorage(foreignError);
    EXPECT_TRUE(
        multi->placeResult(nlohmann::json::json_pointer("/Foreign"),
                           foreignError, MultiAsyncResp::BudgetAction::Charge));

    crow::Response sibling;
    sibling.jsonValue = nlohmann::json{{"Result", "success"}};
    EXPECT_TRUE(
        multi->placeResult(nlohmann::json::json_pointer("/Sibling"), sibling,
                           MultiAsyncResp::BudgetAction::Charge));
    EXPECT_EQ(finalRes->res.result(),
              boost::beast::http::status::insufficient_storage);
    EXPECT_EQ(finalRes->res.jsonValue["Sibling"]["Result"], "success");
}

TEST(MultiAsyncResp, PlaceResultTripsPerRequestBudget)
{
    auto finalRes = std::make_shared<bmcweb::AsyncResp>();
    auto multi = std::make_shared<MultiAsyncResp>(finalRes);
    if (!primeBudgetJustUnderLimit(multi, finalRes))
    {
        GTEST_SKIP() << "configured limits cannot exercise this test";
    }

    crow::Response child;
    child.jsonValue = nlohmann::json{{"Foo", "x"}};

    EXPECT_FALSE(
        multi->placeResult(nlohmann::json::json_pointer("/Child"), child,
                           MultiAsyncResp::BudgetAction::Charge));
    EXPECT_EQ(finalRes->res.result(),
              boost::beast::http::status::insufficient_storage);
}

TEST(MultiAsyncResp, BudgetExceededSuppressesFurtherMerges)
{
    auto finalRes = std::make_shared<bmcweb::AsyncResp>();
    auto multi = std::make_shared<MultiAsyncResp>(finalRes);
    if (!primeBudgetJustUnderLimit(multi, finalRes))
    {
        GTEST_SKIP() << "configured limits cannot exercise this test";
    }

    crow::Response overLimit;
    overLimit.jsonValue = nlohmann::json{{"Foo", "over the limit"}};
    ASSERT_FALSE(
        multi->placeResult(nlohmann::json::json_pointer("/A"), overLimit,
                           MultiAsyncResp::BudgetAction::Charge));

    crow::Response small;
    small.jsonValue = nlohmann::json{{"Bar", "Baz"}};
    EXPECT_FALSE(multi->placeResult(nlohmann::json::json_pointer("/B"), small,
                                    MultiAsyncResp::BudgetAction::Charge));
    EXPECT_FALSE(finalRes->res.jsonValue.contains("B"));
}

// A nested query marks the shared budget as exceeded before its 507 response
// reaches its parent. The parent must still propagate that first 507 rather
// than mistaking it for a duplicate.
TEST(MultiAsyncResp, PropagatesFirstNestedInsufficientStorage)
{
    auto finalRes = std::make_shared<bmcweb::AsyncResp>();
    auto multi = std::make_shared<MultiAsyncResp>(finalRes);
    auto sharedBudget = makeExpandBudget();
    auto sharedExceeded = std::make_shared<bool>(false);
    auto context = std::make_shared<ExpandContext>(ExpandContext{
        .payloadUsed = sharedBudget,
        .budgetExceeded = sharedExceeded,
    });

    Query query;
    Query delegated;
    crow::Request req;
    multi->startQuery(query, delegated, req, context);

    crow::Response child;
    messages::insufficientStorage(child);
    *sharedExceeded = true;

    EXPECT_FALSE(
        multi->placeResult(nlohmann::json::json_pointer("/Child"), child,
                           MultiAsyncResp::BudgetAction::VerifyOnly));
    EXPECT_EQ(finalRes->res.result(),
              boost::beast::http::status::insufficient_storage);
}

TEST(MultiAsyncResp, BudgetExceededStopsNestedQueryBeforeCharging)
{
    uint64_t before = expandBytesInFlight();
    auto finalRes = std::make_shared<bmcweb::AsyncResp>();
    auto multi = std::make_shared<MultiAsyncResp>(finalRes);
    auto sharedBudget = makeExpandBudget();
    auto context = std::make_shared<ExpandContext>(ExpandContext{
        .payloadUsed = sharedBudget,
        .budgetExceeded = std::make_shared<bool>(true),
    });

    Query query;
    Query delegated;
    crow::Request req;
    multi->startQuery(query, delegated, req, context);

    EXPECT_EQ(*sharedBudget, 0U);
    EXPECT_EQ(expandBytesInFlight(), before);
}

// expandBytesInFlight() is only refunded once every shared_ptr copy of the
// budget is released, not when an earlier one is.
TEST(ExpandBudget, RefundsOnlyWhenLastReferenceReleased)
{
    uint64_t initial = expandBytesInFlight();

    auto budget = makeExpandBudget();
    *budget = 100;
    expandBytesInFlight() += 100;

    auto copy = budget; // simulates a nested level sharing the same budget
    budget.reset();
    EXPECT_EQ(expandBytesInFlight(), initial + 100); // still held by copy

    copy.reset();
    EXPECT_EQ(expandBytesInFlight(), initial); // last reference released
}

// Drives a real $levels>1 nested merge: root and child share one budget via
// startQuery(), the child is destroyed, then root merges its result with
// BudgetAction::VerifyOnly. The gauge must stay charged across all of that and
// only refund once the last shared_ptr (the test's own) is released.
TEST(MultiAsyncResp, NestedExpandGaugeDeferredUntilTreeReleased)
{
    uint64_t initial = expandBytesInFlight();

    auto sharedBudget = makeExpandBudget();
    auto sharedExceeded = std::make_shared<bool>(false);
    Query query;
    Query delegated;

    auto rootFinalRes = std::make_shared<bmcweb::AsyncResp>();
    auto root = std::make_shared<MultiAsyncResp>(rootFinalRes);
    {
        crow::Request rootReq;
        auto rootContext = std::make_shared<ExpandContext>(ExpandContext{
            .payloadUsed = sharedBudget,
            .budgetExceeded = sharedExceeded,
        });
        root->startQuery(query, delegated, rootReq, rootContext);
    }
    EXPECT_GT(expandBytesInFlight(), initial);

    uint64_t afterRoot = expandBytesInFlight();

    {
        auto childFinalRes = std::make_shared<bmcweb::AsyncResp>();
        childFinalRes->res.jsonValue =
            nlohmann::json{{"Leaf", std::string(256, 'z')}};
        auto child = std::make_shared<MultiAsyncResp>(childFinalRes);
        crow::Request childReq;
        auto childContext = std::make_shared<ExpandContext>(ExpandContext{
            .payloadUsed = sharedBudget,
            .budgetExceeded = sharedExceeded,
        });
        child->startQuery(query, delegated, childReq, childContext);
        EXPECT_GT(expandBytesInFlight(), afterRoot);
    }
    // Child destroyed; root and sharedBudget still hold the tree alive, so
    // the gauge must stay charged (not refund early).
    EXPECT_GT(expandBytesInFlight(), afterRoot);
    uint64_t afterChildDestroyed = expandBytesInFlight();

    crow::Response merged;
    merged.jsonValue = nlohmann::json{{"Leaf", std::string(256, 'z')}};
    EXPECT_TRUE(
        root->placeResult(nlohmann::json::json_pointer("/Merged"), merged,
                          MultiAsyncResp::BudgetAction::VerifyOnly));
    EXPECT_EQ(expandBytesInFlight(), afterChildDestroyed); // not re-charged

    root.reset();
    EXPECT_EQ(expandBytesInFlight(),
              afterChildDestroyed); // sharedBudget remains

    sharedBudget.reset(); // last reference released → gauge fully refunded
    EXPECT_EQ(expandBytesInFlight(), initial);
}

// Reproduces context propagation through the real dispatch path: route setup
// takes the context using the AsyncResp that survives deferred privilege
// validation, while nested processing receives a separate request copy.
TEST(MultiAsyncResp, ExpandBudgetContextPropagatesOutsideRequest)
{
    auto context = std::make_shared<ExpandContext>(ExpandContext{
        .payloadUsed = makeExpandBudget(),
        .budgetExceeded = std::make_shared<bool>(false),
    });
    auto originalSubrequest = std::make_shared<crow::Request>();
    auto dispatchResp = std::make_shared<bmcweb::AsyncResp>();

    // As startSubquery() and query.hpp do: register before dispatch and take
    // the context during route setup, without storing it on crow::Request.
    registerExpandContext(dispatchResp.get(), context);
    std::shared_ptr<ExpandContext> capturedContext =
        takeExpandContext(dispatchResp.get());
    EXPECT_EQ(takeExpandContext(dispatchResp.get()), nullptr);
    crow::Request requestCopy = originalSubrequest->copy();

    Query query;
    Query delegated;
    auto childFinalRes = std::make_shared<bmcweb::AsyncResp>();
    auto child = std::make_shared<MultiAsyncResp>(childFinalRes);
    child->startQuery(query, delegated, requestCopy, capturedContext);

    EXPECT_TRUE(context->budgetCharged)
        << "parent would pick BudgetAction::Charge and double-charge an "
           "already-accounted subtree";
}

// A stale clearExpandContext() call must not drop a different context that
// has since claimed the same AsyncResp address.
TEST(MultiAsyncResp, DelayedClearExpandContextDoesNotDropReusedAddress)
{
    auto staleContext = std::make_shared<ExpandContext>(ExpandContext{
        .payloadUsed = makeExpandBudget(),
        .budgetExceeded = std::make_shared<bool>(false),
    });
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    registerExpandContext(asyncResp.get(), staleContext);

    // Simulate address reuse by a later, unrelated request.
    auto liveContext = std::make_shared<ExpandContext>(ExpandContext{
        .payloadUsed = makeExpandBudget(),
        .budgetExceeded = std::make_shared<bool>(false),
    });
    registerExpandContext(asyncResp.get(), liveContext);

    clearExpandContext(asyncResp.get(), staleContext);

    EXPECT_EQ(takeExpandContext(asyncResp.get()), liveContext);
}

// Exercise a real @odata.id dispatch. The child runs query setup and charges
// the shared budget, so its parent's merge must verify rather than charge the
// same child response a second time.
TEST(MultiAsyncResp, NavigationSubqueryChargesSharedBudgetOnce)
{
    crow::App app;
    BMCWEB_ROUTE(app, "/child/")
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue = {{"@odata.id", "/child/"},
                                            {"Value", "expanded"}};
                // $expand string parsing is gated behind the
                // insecure-enable-redfish-query build option, which this
                // test target doesn't enable; drive processAllParams()
                // directly with an already-parsed Query so this still
                // exercises the real startQuery()/startSubquery() dispatch
                // and the Charge/VerifyOnly decision, same as
                // setUpRedfishRoute() would once query params are parsed.
                std::shared_ptr<ExpandContext> expandContext =
                    takeExpandContext(asyncResp.get());
                std::function<void(crow::Response&)> handler =
                    asyncResp->res.releaseCompleteRequestHandler();
                Query childQuery{
                    .expandLevel = 1,
                    .expandType = ExpandType::Both,
                };
                Query childDelegated;
                processAllParams(app, childQuery, childDelegated, handler,
                                 asyncResp->res, req, expandContext);
            });
    app.validate();

    auto finalRes = std::make_shared<bmcweb::AsyncResp>();
    finalRes->res.jsonValue = {
        {"@odata.id", "/root/"},
        {"Child", {{"@odata.id", "/child/"}}},
    };
    uint64_t expectedRootSize =
        json_util::getEstimatedJsonSize(finalRes->res.jsonValue);
    nlohmann::json expectedChild = {{"@odata.id", "/child/"},
                                    {"Value", "expanded"}};
    uint64_t expectedChildSize = json_util::getEstimatedJsonSize(expectedChild);
    uint64_t initial = expandBytesInFlight();

    auto multi = std::make_shared<MultiAsyncResp>(app, finalRes);
    Query query{
        .expandLevel = 2,
        .expandType = ExpandType::Both,
    };
    Query delegated;
    crow::Request req;
    multi->startQuery(query, delegated, req);

    EXPECT_EQ(finalRes->res.jsonValue["Child"], expectedChild);
    EXPECT_EQ(expandBytesInFlight(),
              initial + expectedRootSize + expectedChildSize);

    multi.reset();
    EXPECT_EQ(expandBytesInFlight(), initial);
}

} // namespace
} // namespace redfish::query_param
