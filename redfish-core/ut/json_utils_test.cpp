#include "utils/json_utils.hpp"

#include <string>
#include <vector>

#include <gmock/gmock.h>

using redfish::json_util::readJson;
using redfish::json_util::readJsonAction;
using redfish::json_util::readJsonPatch;

TEST(readJson, ValidElements)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1},
                                  {"string", "hello"},
                                  {"vector", std::vector<uint64_t>{1, 2, 3}}};

    int64_t integer = 0;
    std::string str;
    std::vector<uint64_t> vec;
    EXPECT_TRUE(readJson(jsonRequest, res, "integer", integer, "string", str,
                         "vector", vec));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());

    EXPECT_EQ(integer, 1);
    EXPECT_EQ(str, "hello");
    EXPECT_TRUE((vec == std::vector<uint64_t>{1, 2, 3}));
}

TEST(readJson, ExtraElements)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}, {"string0", "hello"}};

    int64_t integer = 0;
    std::string str;
    EXPECT_FALSE(readJson(jsonRequest, res, "integer", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());

    EXPECT_FALSE(readJson(jsonRequest, res, "string0", str));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());
}

TEST(readJson, WrongElementType)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}, {"string0", "hello"}};

    int64_t integer = 0;
    std::string str0;
    std::string str1;
    EXPECT_FALSE(readJson(jsonRequest, res, "integer", str0));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());

    EXPECT_FALSE(readJson(jsonRequest, res, "string0", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());

    EXPECT_FALSE(
        readJson(jsonRequest, res, "integer", str0, "string0", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());
}

TEST(readJson, MissingElement)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}, {"string0", "hello"}};

    int64_t integer = 0;
    std::string str0;
    std::string str1;
    std::vector<uint8_t> vec;
    EXPECT_FALSE(readJson(jsonRequest, res, "integer", integer, "string0", str0,
                          "vector", vec));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());

    EXPECT_FALSE(readJson(jsonRequest, res, "integer", integer, "string0", str0,
                          "string1", str1));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());
}

TEST(readJson, JsonVector)
{
    crow::Response res;
    nlohmann::json jsonRequest = R"(
        {
            "TestJson": [{"hello": "yes"}, [{"there": "no"}, "nice"]]
        }
    )"_json;

    std::vector<nlohmann::json> jsonVec;
    EXPECT_TRUE(readJson(jsonRequest, res, "TestJson", jsonVec));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());
}

TEST(readJson, JsonSubElementValue)
{
    crow::Response res;
    nlohmann::json jsonRequest = R"(
        {
            "json": {"integer": 42}
        }
    )"_json;

    int integer = 0;
    EXPECT_TRUE(readJson(jsonRequest, res, "json/integer", integer));
    EXPECT_EQ(integer, 42);
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());
}

TEST(readJson, JsonSubElementValueDepth2)
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
    EXPECT_TRUE(readJson(jsonRequest, res, "json/json2/string", foobar));
    EXPECT_EQ(foobar, "foobar");
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());
}

TEST(readJson, JsonSubElementValueMultiple)
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
    EXPECT_TRUE(readJson(jsonRequest, res, "json/integer", integer,
                         "json/string", foobar, "string", bazbar));
    EXPECT_EQ(integer, 42);
    EXPECT_EQ(foobar, "foobar");
    EXPECT_EQ(bazbar, "bazbar");
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());
}

TEST(readJson, ExtraElement)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}, {"string", "hello"}};

    std::optional<int> integer;
    std::optional<std::string> str;
    std::optional<std::vector<uint8_t>> vec;

    EXPECT_FALSE(readJson(jsonRequest, res, "integer", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());
    EXPECT_EQ(integer, 1);

    EXPECT_FALSE(readJson(jsonRequest, res, "string", str));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());
    EXPECT_EQ(str, "hello");
}

TEST(readJson, ValidMissingElement)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}};

    std::optional<int> integer;
    int requiredInteger = 0;
    std::optional<std::string> str0;
    std::optional<std::string> str1;
    std::optional<std::vector<uint8_t>> vec;
    EXPECT_TRUE(readJson(jsonRequest, res, "missing_integer", integer,
                         "integer", requiredInteger));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());
    EXPECT_EQ(integer, std::nullopt);

    EXPECT_TRUE(readJson(jsonRequest, res, "missing_string", str0, "integer",
                         requiredInteger));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());
    EXPECT_EQ(str0, std::nullopt);

    EXPECT_TRUE(readJson(jsonRequest, res, "integer", integer, "string", str0,
                         "vector", vec));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());
    EXPECT_EQ(integer, 1);
    EXPECT_EQ(str0, std::nullopt);
    EXPECT_EQ(vec, std::nullopt);

    EXPECT_TRUE(readJson(jsonRequest, res, "integer", integer, "string0", str0,
                         "missing_string", str1));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());
    EXPECT_EQ(str1, std::nullopt);
}

TEST(readJson, InvalidMissingElement)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}, {"string", "hello"}};

    int integer = 0;
    std::string str0;
    std::string str1;
    std::vector<uint8_t> vec;
    EXPECT_FALSE(readJson(jsonRequest, res, "missing_integer", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());

    EXPECT_FALSE(readJson(jsonRequest, res, "missing_string", str0));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());

    EXPECT_FALSE(readJson(jsonRequest, res, "integer", integer, "string", str0,
                          "vector", vec));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());

    EXPECT_FALSE(readJson(jsonRequest, res, "integer", integer, "string0", str0,
                          "missing_string", str1));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());
}

TEST(readJsonPatch, ValidElements)
{
    crow::Response res;
    std::error_code ec;
    crow::Request req({}, ec);
    // Ignore errors intentionally
    req.body = "{\"integer\": 1}";

    int64_t integer = 0;
    EXPECT_TRUE(readJsonPatch(req, res, "integer", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());
}

TEST(readJsonPatch, EmptyObjectDisallowed)
{
    crow::Response res;
    std::error_code ec;
    crow::Request req({}, ec);
    // Ignore errors intentionally
    req.body = "{}";

    std::optional<int64_t> integer = 0;
    EXPECT_FALSE(readJsonPatch(req, res, "integer", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());
}

TEST(readJsonAction, ValidElements)
{
    crow::Response res;
    std::error_code ec;
    crow::Request req({}, ec);
    // Ignore errors intentionally
    req.body = "{\"integer\": 1}";

    int64_t integer = 0;
    EXPECT_TRUE(readJsonAction(req, res, "integer", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());
}

TEST(readJsonAction, EmptyObjectAllowed)
{
    crow::Response res;
    std::error_code ec;
    crow::Request req({}, ec);
    // Ignore errors intentionally
    req.body = "{}";

    std::optional<int64_t> integer = 0;
    EXPECT_TRUE(readJsonAction(req, res, "integer", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());
}
