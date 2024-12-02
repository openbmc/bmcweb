#include "http_request.hpp"
#include "http_response.hpp"
#include "utils/json_utils.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <system_error>
#include <variant>
#include <vector>

#include <gmock/gmock.h> // IWYU pragma: keep
#include <gtest/gtest.h> // IWYU pragma: keep

// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"
// IWYU pragma: no_include <boost/intrusive/detail/list_iterator.hpp>

namespace redfish::json_util
{
namespace
{

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Not;

TEST(ReadJson, ValidElementsReturnsTrueResponseOkValuesUnpackedCorrectly)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1},
                                  {"string", "hello"},
                                  {"vector", std::vector<uint64_t>{1, 2, 3}}};

    int64_t integer = 0;
    std::string str;
    std::vector<uint64_t> vec;
    ASSERT_TRUE(readJson(jsonRequest, res, "integer", integer, "string", str,
                         "vector", vec));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_THAT(res.jsonValue, IsEmpty());

    EXPECT_EQ(integer, 1);
    EXPECT_EQ(str, "hello");
    EXPECT_THAT(vec, ElementsAre(1, 2, 3));
}

TEST(ReadJson, ValidObjectElementsReturnsTrueResponseOkValuesUnpackedCorrectly)
{
    crow::Response res;
    nlohmann::json::object_t jsonRequest;
    jsonRequest["integer"] = 1;
    jsonRequest["string"] = "hello";
    jsonRequest["vector"] = std::vector<uint64_t>{1, 2, 3};

    int64_t integer = 0;
    std::string str;
    std::vector<uint64_t> vec;
    ASSERT_TRUE(readJsonObject(jsonRequest, res, "integer", integer, "string",
                               str, "vector", vec));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_THAT(res.jsonValue, IsEmpty());

    EXPECT_EQ(integer, 1);
    EXPECT_EQ(str, "hello");
    EXPECT_THAT(vec, ElementsAre(1, 2, 3));
}

TEST(ReadJson, VariantValueUnpackedNull)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"nullval", nullptr}};

    std::variant<std::string, std::nullptr_t> str;

    ASSERT_TRUE(readJson(jsonRequest, res, "nullval", str));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);

    EXPECT_TRUE(std::holds_alternative<std::nullptr_t>(str));
}

TEST(ReadJson, VariantValueUnpackedValue)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"stringval", "mystring"}};

    std::variant<std::string, std::nullptr_t> str;

    ASSERT_TRUE(readJson(jsonRequest, res, "stringval", str));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);

    ASSERT_TRUE(std::holds_alternative<std::string>(str));
    EXPECT_EQ(std::get<std::string>(str), "mystring");
}

TEST(readJson, ExtraElementsReturnsFalseReponseIsBadRequest)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}, {"string", "hello"}};

    std::optional<int> integer;
    std::optional<std::string> str;

    ASSERT_FALSE(readJson(jsonRequest, res, "integer", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_THAT(res.jsonValue, Not(IsEmpty()));
    EXPECT_EQ(integer, 1);

    ASSERT_FALSE(readJson(jsonRequest, res, "string", str));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_THAT(res.jsonValue, Not(IsEmpty()));
    EXPECT_EQ(str, "hello");
}

TEST(ReadJson, WrongElementTypeReturnsFalseReponseIsBadRequest)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}, {"string0", "hello"}};

    int64_t integer = 0;
    std::string str0;
    ASSERT_FALSE(readJson(jsonRequest, res, "integer", str0));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_THAT(res.jsonValue, Not(IsEmpty()));

    ASSERT_FALSE(readJson(jsonRequest, res, "string0", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_THAT(res.jsonValue, Not(IsEmpty()));

    ASSERT_FALSE(
        readJson(jsonRequest, res, "integer", str0, "string0", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_THAT(res.jsonValue, Not(IsEmpty()));
}

TEST(ReadJson, MissingElementReturnsFalseReponseIsBadRequest)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}, {"string0", "hello"}};

    int64_t integer = 0;
    std::string str0;
    std::string str1;
    std::vector<uint8_t> vec;
    ASSERT_FALSE(readJson(jsonRequest, res, "integer", integer, "string0", str0,
                          "vector", vec));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_THAT(res.jsonValue, Not(IsEmpty()));

    ASSERT_FALSE(readJson(jsonRequest, res, "integer", integer, "string0", str0,
                          "string1", str1));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_THAT(res.jsonValue, Not(IsEmpty()));
}

TEST(ReadJson, JsonArrayAreUnpackedCorrectly)
{
    crow::Response res;
    nlohmann::json jsonRequest = R"(
        {
            "TestJson": [{"hello": "yes"}, [{"there": "no"}, "nice"]]
        }
    )"_json;

    std::vector<nlohmann::json> jsonVec;
    ASSERT_TRUE(readJson(jsonRequest, res, "TestJson", jsonVec));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_THAT(res.jsonValue, IsEmpty());
    EXPECT_THAT(jsonVec, ElementsAre(R"({"hello": "yes"})"_json,
                                     R"([{"there": "no"}, "nice"])"_json));
}

TEST(ReadJson, JsonSubElementValueAreUnpackedCorrectly)
{
    crow::Response res;
    nlohmann::json jsonRequest = R"(
        {
            "json": {"integer": 42}
        }
    )"_json;

    int integer = 0;
    ASSERT_TRUE(readJson(jsonRequest, res, "json/integer", integer));
    EXPECT_EQ(integer, 42);
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_THAT(res.jsonValue, IsEmpty());
}

TEST(ReadJson, JsonDeeperSubElementValueAreUnpackedCorrectly)
{
    crow::Response res;
    nlohmann::json jsonRequest = R"(
        {
            "json": {
                "json2": {"string": "foobar"}
            }
        }
    )"_json;

    std::string foobar;
    ASSERT_TRUE(readJson(jsonRequest, res, "json/json2/string", foobar));
    EXPECT_EQ(foobar, "foobar");
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_THAT(res.jsonValue, IsEmpty());
}

TEST(ReadJson, MultipleJsonSubElementValueAreUnpackedCorrectly)
{
    crow::Response res;
    nlohmann::json jsonRequest = R"(
        {
            "json": {
                "integer": 42,
                "string": "foobar"
            },
            "string": "bazbar"
        }
    )"_json;

    int integer = 0;
    std::string foobar;
    std::string bazbar;
    ASSERT_TRUE(readJson(jsonRequest, res, "json/integer", integer,
                         "json/string", foobar, "string", bazbar));
    EXPECT_EQ(integer, 42);
    EXPECT_EQ(foobar, "foobar");
    EXPECT_EQ(bazbar, "bazbar");
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_THAT(res.jsonValue, IsEmpty());
}

TEST(ReadJson, ExtraElement)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}, {"string", "hello"}};

    std::optional<int> integer;
    std::optional<std::string> str;

    EXPECT_FALSE(readJson(jsonRequest, res, "integer", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());
    EXPECT_EQ(integer, 1);

    EXPECT_FALSE(readJson(jsonRequest, res, "string", str));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());
    EXPECT_EQ(str, "hello");
}

TEST(ReadJson, ValidMissingElementReturnsTrue)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}};

    std::optional<int> integer;
    int requiredInteger = 0;
    std::optional<std::string> str0;
    std::optional<std::string> str1;
    std::optional<std::vector<uint8_t>> vec;
    ASSERT_TRUE(readJson(jsonRequest, res, "missing_integer", integer,
                         "integer", requiredInteger));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());
    EXPECT_EQ(integer, std::nullopt);

    ASSERT_TRUE(readJson(jsonRequest, res, "missing_string", str0, "integer",
                         requiredInteger));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_THAT(res.jsonValue, IsEmpty());
    EXPECT_EQ(str0, std::nullopt);

    ASSERT_TRUE(readJson(jsonRequest, res, "integer", integer, "string", str0,
                         "vector", vec));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_THAT(res.jsonValue, IsEmpty());
    EXPECT_EQ(integer, 1);
    EXPECT_EQ(str0, std::nullopt);
    EXPECT_EQ(vec, std::nullopt);

    ASSERT_TRUE(readJson(jsonRequest, res, "integer", integer, "string0", str0,
                         "missing_string", str1));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_THAT(res.jsonValue, IsEmpty());
    EXPECT_EQ(str1, std::nullopt);
}

TEST(ReadJson, InvalidMissingElementReturnsFalse)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}, {"string", "hello"}};

    int integer = 0;
    std::string str0;
    std::string str1;
    std::vector<uint8_t> vec;
    ASSERT_FALSE(readJson(jsonRequest, res, "missing_integer", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_THAT(res.jsonValue, Not(IsEmpty()));

    ASSERT_FALSE(readJson(jsonRequest, res, "missing_string", str0));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_THAT(res.jsonValue, Not(IsEmpty()));

    ASSERT_FALSE(readJson(jsonRequest, res, "integer", integer, "string", str0,
                          "vector", vec));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_THAT(res.jsonValue, Not(IsEmpty()));

    ASSERT_FALSE(readJson(jsonRequest, res, "integer", integer, "string0", str0,
                          "missing_string", str1));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_THAT(res.jsonValue, Not(IsEmpty()));
}

TEST(ReadJsonPatch, ValidElementsReturnsTrueResponseOkValuesUnpackedCorrectly)
{
    crow::Response res;
    std::error_code ec;
    crow::Request req("{\"integer\": 1}", ec);

    // Ignore errors intentionally
    req.addHeader(boost::beast::http::field::content_type, "application/json");

    int64_t integer = 0;
    ASSERT_TRUE(readJsonPatch(req, res, "integer", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_THAT(res.jsonValue, IsEmpty());
    EXPECT_EQ(integer, 1);
}

TEST(ReadJsonPatch, EmptyObjectReturnsFalseResponseBadRequest)
{
    crow::Response res;
    std::error_code ec;
    crow::Request req("{}", ec);
    // Ignore errors intentionally

    std::optional<int64_t> integer = 0;
    ASSERT_FALSE(readJsonPatch(req, res, "integer", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_THAT(res.jsonValue, Not(IsEmpty()));
}

TEST(ReadJsonPatch, OdataIgnored)
{
    crow::Response res;
    std::error_code ec;
    crow::Request req(R"({"@odata.etag": "etag", "integer": 1})", ec);
    req.addHeader(boost::beast::http::field::content_type, "application/json");
    // Ignore errors intentionally

    std::optional<int64_t> integer = 0;
    ASSERT_TRUE(readJsonPatch(req, res, "integer", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_THAT(res.jsonValue, IsEmpty());
    EXPECT_EQ(integer, 1);
}

TEST(ReadJsonPatch, OnlyOdataGivesNoOperation)
{
    crow::Response res;
    std::error_code ec;
    crow::Request req(R"({"@odata.etag": "etag"})", ec);
    // Ignore errors intentionally

    std::optional<int64_t> integer = 0;
    ASSERT_FALSE(readJsonPatch(req, res, "integer", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_THAT(res.jsonValue, Not(IsEmpty()));
}

TEST(ReadJsonPatch, VerifyReadJsonPatchIntegerReturnsOutOfRange)
{
    crow::Response res;
    std::error_code ec;

    // 4294967296 is an out-of-range value for uint32_t
    crow::Request req(R"({"@odata.etag": "etag", "integer": 4294967296})", ec);
    req.addHeader(boost::beast::http::field::content_type, "application/json");

    uint32_t integer = 0;
    ASSERT_FALSE(readJsonPatch(req, res, "integer", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);

    const nlohmann::json& resExtInfo =
        res.jsonValue["error"]["@Message.ExtendedInfo"];
    EXPECT_THAT(resExtInfo[0]["@odata.type"], "#Message.v1_1_1.Message");
    EXPECT_THAT(resExtInfo[0]["MessageId"],
                "Base.1.19.0.PropertyValueOutOfRange");
    EXPECT_THAT(resExtInfo[0]["MessageSeverity"], "Warning");
}

TEST(ReadJsonAction, ValidElementsReturnsTrueResponseOkValuesUnpackedCorrectly)
{
    crow::Response res;
    std::error_code ec;
    crow::Request req("{\"integer\": 1}", ec);
    req.addHeader(boost::beast::http::field::content_type, "application/json");
    // Ignore errors intentionally

    int64_t integer = 0;
    ASSERT_TRUE(readJsonAction(req, res, "integer", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_THAT(res.jsonValue, IsEmpty());
    EXPECT_EQ(integer, 1);
}

TEST(ReadJsonAction, EmptyObjectReturnsTrueResponseOk)
{
    crow::Response res;
    std::error_code ec;
    crow::Request req({"{}"}, ec);
    req.addHeader(boost::beast::http::field::content_type, "application/json");
    // Ignore errors intentionally

    std::optional<int64_t> integer = 0;
    ASSERT_TRUE(readJsonAction(req, res, "integer", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_THAT(res.jsonValue, IsEmpty());
}

TEST(odataObjectCmp, PositiveCases)
{
    EXPECT_EQ(0, odataObjectCmp(R"({"@odata.id": "/redfish/v1/1"})"_json,
                                R"({"@odata.id": "/redfish/v1/1"})"_json));
    EXPECT_EQ(0, odataObjectCmp(R"({"@odata.id": ""})"_json,
                                R"({"@odata.id": ""})"_json));
    EXPECT_EQ(0, odataObjectCmp(R"({"@odata.id": 42})"_json,
                                R"({"@odata.id": 0})"_json));
    EXPECT_EQ(0, odataObjectCmp(R"({})"_json, R"({})"_json));

    EXPECT_GT(0, odataObjectCmp(R"({"@odata.id": "/redfish/v1"})"_json,
                                R"({"@odata.id": "/redfish/v1/1"})"_json));
    EXPECT_LT(0, odataObjectCmp(R"({"@odata.id": "/redfish/v1/1"})"_json,
                                R"({"@odata.id": "/redfish/v1"})"_json));

    EXPECT_LT(0, odataObjectCmp(R"({"@odata.id": "/10"})"_json,
                                R"({"@odata.id": "/1"})"_json));
    EXPECT_GT(0, odataObjectCmp(R"({"@odata.id": "/1"})"_json,
                                R"({"@odata.id": "/10"})"_json));

    EXPECT_GT(0, odataObjectCmp(R"({})"_json, R"({"@odata.id": "/1"})"_json));
    EXPECT_LT(0, odataObjectCmp(R"({"@odata.id": "/1"})"_json, R"({})"_json));

    EXPECT_GT(0, odataObjectCmp(R"({"@odata.id": 4})"_json,
                                R"({"@odata.id": "/1"})"_json));
    EXPECT_LT(0, odataObjectCmp(R"({"@odata.id": "/1"})"_json,
                                R"({"@odata.id": 4})"_json));
}

TEST(SortJsonArrayByOData, ElementMissingKeyReturnsFalseArrayIsPartlySorted)
{
    nlohmann::json::array_t array =
        R"([{"@odata.id" : "/redfish/v1/100"}, {"@odata.id": "/redfish/v1/1"}, {"@odata.id" : "/redfish/v1/20"}])"_json;
    sortJsonArrayByOData(array);
    // Objects with other keys are always larger than those with the specified
    // key.
    EXPECT_THAT(array,
                ElementsAre(R"({"@odata.id": "/redfish/v1/1"})"_json,
                            R"({"@odata.id" : "/redfish/v1/20"})"_json,
                            R"({"@odata.id" : "/redfish/v1/100"})"_json));
}

TEST(SortJsonArrayByOData, SortedByStringValueOnSuccessArrayIsSorted)
{
    nlohmann::json::array_t array =
        R"([{"@odata.id": "/redfish/v1/20"}, {"@odata.id" : "/redfish/v1"}, {"@odata.id" : "/redfish/v1/100"}])"_json;
    sortJsonArrayByOData(array);
    EXPECT_THAT(array,
                ElementsAre(R"({"@odata.id": "/redfish/v1"})"_json,
                            R"({"@odata.id" : "/redfish/v1/20"})"_json,
                            R"({"@odata.id" : "/redfish/v1/100"})"_json));
}

TEST(objectKeyCmp, PositiveCases)
{
    EXPECT_EQ(
        0, objectKeyCmp("@odata.id",
                        R"({"@odata.id": "/redfish/v1/1", "Name": "a"})"_json,
                        R"({"@odata.id": "/redfish/v1/1", "Name": "b"})"_json));
    EXPECT_GT(
        0, objectKeyCmp("Name",
                        R"({"@odata.id": "/redfish/v1/1", "Name": "a"})"_json,
                        R"({"@odata.id": "/redfish/v1/1", "Name": "b"})"_json));
    EXPECT_EQ(0, objectKeyCmp(
                     "Name",
                     R"({"@odata.id": "/redfish/v1/1", "Name": "a 45"})"_json,
                     R"({"@odata.id": "/redfish/v1/1", "Name": "a 45"})"_json));
    EXPECT_GT(0, objectKeyCmp(
                     "Name",
                     R"({"@odata.id": "/redfish/v1/1", "Name": "a 45"})"_json,
                     R"({"@odata.id": "/redfish/v1/1", "Name": "b 45"})"_json));

    EXPECT_GT(
        0, objectKeyCmp("@odata.id",
                        R"({"@odata.id": "/redfish/v1/1", "Name": "b"})"_json,
                        R"({"@odata.id": "/redfish/v1/2", "Name": "b"})"_json));
    EXPECT_EQ(
        0, objectKeyCmp("Name",
                        R"({"@odata.id": "/redfish/v1/1", "Name": "b"})"_json,
                        R"({"@odata.id": "/redfish/v1/2", "Name": "b"})"_json));

    EXPECT_LT(0,
              objectKeyCmp(
                  "@odata.id",
                  R"({"@odata.id": "/redfish/v1/p10/", "Name": "a1"})"_json,
                  R"({"@odata.id": "/redfish/v1/p1/", "Name": "a10"})"_json));
    EXPECT_GT(0,
              objectKeyCmp(
                  "Name",
                  R"({"@odata.id": "/redfish/v1/p10/", "Name": "a1"})"_json,
                  R"({"@odata.id": "/redfish/v1/p1/", "Name": "a10"})"_json));

    nlohmann::json leftRequest =
        R"({"Name": "fan1", "@odata.id": "/redfish/v1/Chassis/chassis2"})"_json;
    nlohmann::json rightRequest =
        R"({"Name": "fan2", "@odata.id": "/redfish/v1/Chassis/chassis1"})"_json;

    EXPECT_GT(0, objectKeyCmp("Name", leftRequest, rightRequest));
    EXPECT_LT(0, objectKeyCmp("@odata.id", leftRequest, rightRequest));
    EXPECT_EQ(0, objectKeyCmp("DataSourceUri", leftRequest, rightRequest));
}

TEST(SortJsonArrayByKey, ElementMissingKeyReturnsFalseArrayIsPartlySorted)
{
    nlohmann::json::array_t array =
        R"([{"@odata.id" : "/redfish/v1/100"}, {"Name" : "/redfish/v1/5"}, {"@odata.id": "/redfish/v1/1"}, {"@odata.id" : "/redfish/v1/20"}])"_json;
    sortJsonArrayByKey(array, "@odata.id");
    // Objects with other keys are always smaller than those with the specified
    // key.
    EXPECT_THAT(array,
                ElementsAre(R"({"Name" : "/redfish/v1/5"})"_json,
                            R"({"@odata.id": "/redfish/v1/1"})"_json,
                            R"({"@odata.id" : "/redfish/v1/20"})"_json,
                            R"({"@odata.id" : "/redfish/v1/100"})"_json));
}

TEST(SortJsonArrayByKey, SortedByStringValueOnSuccessArrayIsSorted)
{
    nlohmann::json::array_t array =
        R"([{"@odata.id": "/redfish/v1/20", "Name": "a"}, {"@odata.id" : "/redfish/v1", "Name": "c"}, {"@odata.id" : "/redfish/v1/100", "Name": "b"}])"_json;

    sortJsonArrayByKey(array, "@odata.id");
    EXPECT_THAT(
        array,
        ElementsAre(R"({"@odata.id": "/redfish/v1", "Name": "c"})"_json,
                    R"({"@odata.id": "/redfish/v1/20", "Name": "a"})"_json,
                    R"({"@odata.id": "/redfish/v1/100", "Name": "b"})"_json));

    sortJsonArrayByKey(array, "Name");
    EXPECT_THAT(
        array,
        ElementsAre(R"({"@odata.id": "/redfish/v1/20", "Name": "a"})"_json,
                    R"({"@odata.id": "/redfish/v1/100", "Name": "b"})"_json,
                    R"({"@odata.id": "/redfish/v1", "Name": "c"})"_json));
}

TEST(GetEstimatedJsonSize, NumberIs8Bytpes)
{
    EXPECT_EQ(getEstimatedJsonSize(nlohmann::json(123)), 8);
    EXPECT_EQ(getEstimatedJsonSize(nlohmann::json(77777777777)), 8);
    EXPECT_EQ(getEstimatedJsonSize(nlohmann::json(3.14)), 8);
}

TEST(GetEstimatedJsonSize, BooleanIs5Byte)
{
    EXPECT_EQ(getEstimatedJsonSize(nlohmann::json(true)), 5);
    EXPECT_EQ(getEstimatedJsonSize(nlohmann::json(false)), 5);
}

TEST(GetEstimatedJsonSize, NullIs4Byte)
{
    EXPECT_EQ(getEstimatedJsonSize(nlohmann::json()), 4);
}

TEST(GetEstimatedJsonSize, StringAndBytesReturnsLengthAndQuote)
{
    EXPECT_EQ(getEstimatedJsonSize(nlohmann::json("1234")), 6);
    EXPECT_EQ(getEstimatedJsonSize(nlohmann::json::binary({1, 2, 3, 4})), 4);
}

TEST(GetEstimatedJsonSize, ArrayReturnsSum)
{
    nlohmann::json arr = {1, 3.14, "123", nlohmann::json::binary({1, 2, 3, 4})};
    EXPECT_EQ(getEstimatedJsonSize(arr), 8 + 8 + 5 + 4);
}

TEST(GetEstimatedJsonSize, ObjectsReturnsSumWithKeyAndValue)
{
    nlohmann::json obj = R"(
{
  "key0": 123,
  "key1": "123",
  "key2": [1, 2, 3],
  "key3": {"key4": "123"}
}
)"_json;

    uint64_t expected = 0;
    // 5 keys of length 4
    expected += uint64_t(5) * 4;
    // 5 colons, 5 quote pairs, and 5 spaces for 5 keys
    expected += uint64_t(5) * (1 + 2 + 1);
    // 2 string values of length 3
    expected += uint64_t(2) * (3 + 2);
    // 1 number value
    expected += 8;
    // 1 array value of 3 numbers
    expected += uint64_t(3) * 8;
    EXPECT_EQ(getEstimatedJsonSize(obj), expected);
}

} // namespace
} // namespace redfish::json_util
