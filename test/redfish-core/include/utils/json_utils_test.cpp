#include "http_request.hpp"
#include "http_response.hpp"
#include "utils/json_utils.hpp"

#include <boost/beast/http/status.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <system_error>
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
    EXPECT_EQ(jsonVec, R"([{"hello": "yes"}, [{"there": "no"}, "nice"]])"_json);
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
    req.req.set(boost::beast::http::field::content_type, "application/json");

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
    req.req.set(boost::beast::http::field::content_type, "application/json");
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

TEST(ReadJsonAction, ValidElementsReturnsTrueResponseOkValuesUnpackedCorrectly)
{
    crow::Response res;
    std::error_code ec;
    crow::Request req("{\"integer\": 1}", ec);
    req.req.set(boost::beast::http::field::content_type, "application/json");
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
    req.req.set(boost::beast::http::field::content_type, "application/json");
    // Ignore errors intentionally

    std::optional<int64_t> integer = 0;
    ASSERT_TRUE(readJsonAction(req, res, "integer", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_THAT(res.jsonValue, IsEmpty());
}

} // namespace
} // namespace redfish::json_util
