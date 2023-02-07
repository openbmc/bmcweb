#include "bmcweb_config.h"

#include "utils/query_param.hpp"

#include <boost/system/result.hpp>
#include <boost/url/url_view.hpp>
#include <nlohmann/json.hpp>

#include <new>
#include <span>

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
    for (auto const& property : properties)
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

    ASSERT_NE(query, std::nullopt);
    recursiveSelect(root, query->selectTrie.root);
    EXPECT_EQ(root, expected);
}

TEST(PropogateErrorCode, 500IsWorst)
{
    constexpr std::array<unsigned, 7> codes = {100, 200, 300, 400,
                                               401, 500, 501};
    for (auto code : codes)
    {
        EXPECT_EQ(propogateErrorCode(500, code), 500);
        EXPECT_EQ(propogateErrorCode(code, 500), 500);
    }
}

TEST(PropogateErrorCode, 5xxAreWorseThanOthers)
{
    constexpr std::array<unsigned, 7> codes = {100, 200, 300, 400,
                                               401, 501, 502};
    for (auto code : codes)
    {
        EXPECT_EQ(propogateErrorCode(code, 505), 505);
        EXPECT_EQ(propogateErrorCode(505, code), 505);
    }
    EXPECT_EQ(propogateErrorCode(502, 501), 502);
    EXPECT_EQ(propogateErrorCode(501, 502), 502);
    EXPECT_EQ(propogateErrorCode(503, 502), 503);
}

TEST(PropogateErrorCode, 401IsWorseThanOthers)
{
    constexpr std::array<unsigned, 7> codes = {100, 200, 300, 400, 401};
    for (auto code : codes)
    {
        EXPECT_EQ(propogateErrorCode(code, 401), 401);
        EXPECT_EQ(propogateErrorCode(401, code), 401);
    }
}

TEST(PropogateErrorCode, 4xxIsWorseThanOthers)
{
    constexpr std::array<unsigned, 7> codes = {100, 200, 300, 400, 402};
    for (auto code : codes)
    {
        EXPECT_EQ(propogateErrorCode(code, 405), 405);
        EXPECT_EQ(propogateErrorCode(405, code), 405);
    }
    EXPECT_EQ(propogateErrorCode(400, 402), 402);
    EXPECT_EQ(propogateErrorCode(402, 403), 403);
    EXPECT_EQ(propogateErrorCode(403, 402), 403);
}

TEST(PropogateError, IntermediateNoErrorMessageMakesNoChange)
{
    crow::Response intermediate;
    intermediate.result(boost::beast::http::status::ok);

    crow::Response finalRes;
    finalRes.result(boost::beast::http::status::ok);
    propogateError(finalRes, intermediate);
    EXPECT_EQ(finalRes.result(), boost::beast::http::status::ok);
    EXPECT_EQ(finalRes.jsonValue.find("error"), finalRes.jsonValue.end());
}

TEST(PropogateError, ErrorsArePropergatedWithErrorInRoot)
{
    nlohmann::json root = R"(
{
    "@odata.type": "#Message.v1_1_1.Message",
    "Message": "The request failed due to an internal service error.  The service is still operational.",
    "MessageArgs": [],
    "MessageId": "Base.1.13.0.InternalError",
    "MessageSeverity": "Critical",
    "Resolution": "Resubmit the request.  If the problem persists, consider resetting the service."
}
)"_json;
    crow::Response intermediate;
    intermediate.result(boost::beast::http::status::internal_server_error);
    intermediate.jsonValue = root;

    crow::Response final;
    final.result(boost::beast::http::status::ok);

    propogateError(final, intermediate);

    EXPECT_EQ(final.jsonValue["error"]["code"].get<std::string>(),
              "Base.1.13.0.InternalError");
    EXPECT_EQ(
        final.jsonValue["error"]["message"].get<std::string>(),
        "The request failed due to an internal service error.  The service is still operational.");
    EXPECT_EQ(intermediate.jsonValue, R"({})"_json);
    EXPECT_EQ(final.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(PropogateError, ErrorsArePropergatedWithErrorCode)
{
    crow::Response intermediate;
    intermediate.result(boost::beast::http::status::internal_server_error);

    nlohmann::json error = R"(
{
    "error": {
        "@Message.ExtendedInfo": [],
        "code": "Base.1.13.0.InternalError",
        "message": "The request failed due to an internal service error.  The service is still operational."
    }
}
)"_json;
    nlohmann::json extendedInfo = R"(
{
    "@odata.type": "#Message.v1_1_1.Message",
    "Message": "The request failed due to an internal service error.  The service is still operational.",
    "MessageArgs": [],
    "MessageId": "Base.1.13.0.InternalError",
    "MessageSeverity": "Critical",
    "Resolution": "Resubmit the request.  If the problem persists, consider resetting the service."
}
)"_json;

    for (int i = 0; i < 10; ++i)
    {
        error["error"][messages::messageAnnotation].push_back(extendedInfo);
    }
    intermediate.jsonValue = error;
    crow::Response final;
    final.result(boost::beast::http::status::ok);

    propogateError(final, intermediate);
    EXPECT_EQ(final.jsonValue["error"][messages::messageAnnotation],
              error["error"][messages::messageAnnotation]);
    std::string errorCode = messages::messageVersionPrefix;
    errorCode += "GeneralError";
    std::string errorMessage =
        "A general error has occurred. See Resolution for "
        "information on how to resolve the error.";
    EXPECT_EQ(final.jsonValue["error"]["code"].get<std::string>(), errorCode);
    EXPECT_EQ(final.jsonValue["error"]["message"].get<std::string>(),
              errorMessage);
    EXPECT_EQ(intermediate.jsonValue, R"({})"_json);
    EXPECT_EQ(final.result(),
              boost::beast::http::status::internal_server_error);
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

    // Responses must include their "@odata.id" property for $expand to work
    // correctly
    singleTreeNode["@odata.id"] = "/redfish/v1/";

    // Parsing as the root should net one entry
    EXPECT_THAT(findNavigationReferences(ExpandType::Both, 1, singleTreeNode),
                UnorderedElementsAre(
                    ExpandNode{json::json_pointer("/Foo"), "/foobar"}));

    // Parsing in Non-hyperlinks mode should net one entry
    EXPECT_THAT(
        findNavigationReferences(ExpandType::NotLinks, 1, singleTreeNode),
        UnorderedElementsAre(
            ExpandNode{json::json_pointer("/Foo"), "/foobar"}));

    // Searching for not types should return empty set
    EXPECT_TRUE(
        findNavigationReferences(ExpandType::None, 1, singleTreeNode).empty());

    // Searching for hyperlinks only should return empty set
    EXPECT_TRUE(
        findNavigationReferences(ExpandType::Links, 1, singleTreeNode).empty());

    json multiTreeNodes =
        R"({"Links": {"@odata.id": "/links"}, "Foo" : {"@odata.id": "/foobar"}})"_json;

    // Responses must include their "@odata.id" property for $expand to work
    // correctly
    multiTreeNodes["@odata.id"] = "/redfish/v1/";

    // Should still find Foo
    EXPECT_THAT(
        findNavigationReferences(ExpandType::NotLinks, 1, multiTreeNodes),
        UnorderedElementsAre(
            ExpandNode{json::json_pointer("/Foo"), "/foobar"}));
}

TEST(QueryParams, FindNavigationReferencesLink)
{
    using nlohmann::json;

    json singleLinkNode =
        R"({"Links" : {"Sessions": {"@odata.id": "/foobar"}}})"_json;

    // Responses must include their "@odata.id" property for $expand to work
    // correctly
    singleLinkNode["@odata.id"] = "/redfish/v1/";

    // Parsing as the root should net one entry
    EXPECT_THAT(findNavigationReferences(ExpandType::Both, 1, singleLinkNode),
                UnorderedElementsAre(ExpandNode{
                    json::json_pointer("/Links/Sessions"), "/foobar"}));
    // Parsing in hyperlinks mode should net one entry
    EXPECT_THAT(findNavigationReferences(ExpandType::Links, 1, singleLinkNode),
                UnorderedElementsAre(ExpandNode{
                    json::json_pointer("/Links/Sessions"), "/foobar"}));

    // Searching for not types should return empty set
    EXPECT_TRUE(
        findNavigationReferences(ExpandType::None, 1, singleLinkNode).empty());

    // Searching for non-hyperlinks only should return empty set
    EXPECT_TRUE(
        findNavigationReferences(ExpandType::NotLinks, 1, singleLinkNode)
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
        findNavigationReferences(ExpandType::NotLinks, 1, expNode).empty());

    // Previous expand was only a single level so we should further expand
    EXPECT_THAT(findNavigationReferences(ExpandType::NotLinks, 2, expNode),
                UnorderedElementsAre(
                    ExpandNode{json::json_pointer("/Members/0/Sensors"),
                               "/redfish/v1/Chassis/5B247A_Sat1/Sensors"},
                    ExpandNode{json::json_pointer("/Members/1/Sensors"),
                               "/redfish/v1/Chassis/5B247A_Sat2/Sensors"}));

    // Make sure we can handle when an array was expanded further down the tree
    json expNode2 = R"({"@odata.id" : "/redfish/v1"})"_json;
    expNode2["Chassis"] = std::move(expNode);
    EXPECT_TRUE(
        findNavigationReferences(ExpandType::NotLinks, 1, expNode2).empty());
    EXPECT_TRUE(
        findNavigationReferences(ExpandType::NotLinks, 2, expNode2).empty());

    // Previous expand was two levels so we should further expand
    EXPECT_THAT(findNavigationReferences(ExpandType::NotLinks, 3, expNode2),
                UnorderedElementsAre(
                    ExpandNode{json::json_pointer("/Chassis/Members/0/Sensors"),
                               "/redfish/v1/Chassis/5B247A_Sat1/Sensors"},
                    ExpandNode{json::json_pointer("/Chassis/Members/1/Sensors"),
                               "/redfish/v1/Chassis/5B247A_Sat2/Sensors"}));
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
        findNavigationReferences(ExpandType::NotLinks, 1, expNode),
        UnorderedElementsAre(ExpandNode{json::json_pointer("/Members/0"),
                                        "/redfish/v1/Chassis/Local"}));

    // The 5B247A_Sat1 Chassis was already expanded a single level so we should
    // further expand it as well as the Local Chassis
    EXPECT_THAT(findNavigationReferences(ExpandType::NotLinks, 2, expNode),
                UnorderedElementsAre(
                    ExpandNode{json::json_pointer("/Members/0"),
                               "/redfish/v1/Chassis/Local"},
                    ExpandNode{json::json_pointer("/Members/1/Sensors"),
                               "/redfish/v1/Chassis/5B247A_Sat1/Sensors"}));

    // Now the response has paths that have been expanded 0, 1, and 2 times
    json expNode2 = R"({"@odata.id" : "/redfish/v1",
                        "Systems": {"@odata.id": "/redfish/v1/Systems"}})"_json;
    expNode2["Chassis"] = std::move(expNode);
    std::cout << expNode2.dump(4);

    EXPECT_THAT(findNavigationReferences(ExpandType::NotLinks, 1, expNode2),
                UnorderedElementsAre(ExpandNode{json::json_pointer("/Systems"),
                                                "/redfish/v1/Systems"}));

    EXPECT_THAT(
        findNavigationReferences(ExpandType::NotLinks, 2, expNode2),
        UnorderedElementsAre(
            ExpandNode{json::json_pointer("/Systems"), "/redfish/v1/Systems"},
            ExpandNode{json::json_pointer("/Chassis/Members/0"),
                       "/redfish/v1/Chassis/Local"}));

    EXPECT_THAT(
        findNavigationReferences(ExpandType::NotLinks, 3, expNode2),
        UnorderedElementsAre(
            ExpandNode{json::json_pointer("/Systems"), "/redfish/v1/Systems"},
            ExpandNode{json::json_pointer("/Chassis/Members/0"),
                       "/redfish/v1/Chassis/Local"},
            ExpandNode{json::json_pointer("/Chassis/Members/1/Sensors"),
                       "/redfish/v1/Chassis/5B247A_Sat1/Sensors"}));
}

} // namespace
} // namespace redfish::query_param
