#include "utils/json_utils.hpp"

#include <string>
#include <vector>

#include <gmock/gmock.h>

using redfish::json_util::readJsonAction;
using redfish::json_util::readJsonPatch;

TEST(readJsonPatch, ValidElements)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1},
                                  {"string", "hello"},
                                  {"vector", std::vector<uint64_t>{1, 2, 3}}};

    int64_t integer = 0;
    std::string str;
    std::vector<uint64_t> vec;
    EXPECT_TRUE(readJsonPatch(jsonRequest, res, "integer", integer, "string",
                              str, "vector", vec));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());

    EXPECT_EQ(integer, 1);
    EXPECT_EQ(str, "hello");
    EXPECT_TRUE((vec == std::vector<uint64_t>{1, 2, 3}));
}

TEST(readJsonPatch, ExtraElements)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}, {"string0", "hello"}};

    int64_t integer = 0;
    std::string str;
    EXPECT_FALSE(readJsonPatch(jsonRequest, res, "integer", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());

    EXPECT_FALSE(readJsonPatch(jsonRequest, res, "string0", str));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());
}

TEST(readJsonPatch, WrongElementType)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}, {"string0", "hello"}};

    int64_t integer = 0;
    std::string str0;
    std::string str1;
    EXPECT_FALSE(readJsonPatch(jsonRequest, res, "integer", str0));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());

    EXPECT_FALSE(readJsonPatch(jsonRequest, res, "string0", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());

    EXPECT_FALSE(
        readJsonPatch(jsonRequest, res, "integer", str0, "string0", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());
}

TEST(readJsonPatch, MissingElement)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}, {"string0", "hello"}};

    int64_t integer = 0;
    std::string str0;
    std::string str1;
    std::vector<uint8_t> vec;
    EXPECT_FALSE(readJsonPatch(jsonRequest, res, "integer", integer, "string0",
                               str0, "vector", vec));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());

    EXPECT_FALSE(readJsonPatch(jsonRequest, res, "integer", integer, "string0",
                               str0, "string1", str1));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());
}

TEST(readJsonPatch, JsonVector)
{
    crow::Response res;
    nlohmann::json jsonRequest = R"(
        {
            "TestJson": [{"hello": "yes"}, [{"there": "no"}, "nice"]]
        }
    )"_json;

    std::vector<nlohmann::json> jsonVec;
    EXPECT_TRUE(readJsonPatch(jsonRequest, res, "TestJson", jsonVec));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());
}

TEST(readJsonPatch, JsonSubElementArray)
{
    crow::Response res;
    nlohmann::json jsonRequest = R"(
        {
            "array": [{"hello": "yes"}, [{"there": "no"}, "nice"]],
            "json": {"array": []},
            "string": "yes"
        }
    )"_json;

    nlohmann::json json0;
    nlohmann::json json1;
    nlohmann::json json2;
    std::string str;
    checkValidReadJsonPatch(jsonRequest, res, "array/0", json0, "array/1",
                            json1, "string", str, "json", json2);
    EXPECT_EQ(str, "yes");
}

TEST(readJsonPatch, JsonSubElementValue)
{
    crow::Response res;
    nlohmann::json jsonRequest = R"(
        {
            "array": [{"hello": "yes"}, [{"there": "no"}, "nice"]],
            "json": {"array": []},
            "string": "yes"
        }
    )"_json;

    nlohmann::json json;
    std::string str0;
    std::string str1;
    std::string str2;
    std::string str3;
    std::string str4;
    checkValidReadJsonPatch(jsonRequest, res, "array/0/hello", str0,
                            "array/1/0/there", str1, "json/array", json,
                            "array/1/1", str2, "string", str3);
    EXPECT_EQ(str0, "yes");
    EXPECT_EQ(str1, "no");
    EXPECT_EQ(str2, "nice");
    EXPECT_EQ(str3, "yes");
    EXPECT_EQ(json.size(), 0);
}

TEST(readJsonAction, ExtraElement)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}, {"string", "hello"}};

    std::optional<int> integer;
    std::optional<std::string> str;
    std::optional<std::vector<uint8_t>> vec;

    EXPECT_TRUE(readJsonAction(jsonRequest, res, "integer", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());
    EXPECT_EQ(integer, 1);

    EXPECT_TRUE(readJsonAction(jsonRequest, res, "string", str));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());
    EXPECT_EQ(str, "hello");
}

TEST(readJsonAction, ValidMissingElement)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}, {"string", "hello"}};

    std::optional<int> integer;
    std::optional<std::string> str0;
    std::optional<std::string> str1;
    std::optional<std::vector<uint8_t>> vec;
    EXPECT_TRUE(readJsonAction(jsonRequest, res, "missing_integer", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());
    EXPECT_EQ(integer, std::nullopt);

    EXPECT_TRUE(readJsonAction(jsonRequest, res, "missing_string", str0));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());
    EXPECT_EQ(str0, std::nullopt);

    EXPECT_TRUE(readJsonAction(jsonRequest, res, "integer", integer, "string",
                               str0, "vector", vec));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());
    EXPECT_EQ(integer, 1);
    EXPECT_EQ(str0, "hello");
    EXPECT_EQ(vec, std::nullopt);

    EXPECT_TRUE(readJsonAction(jsonRequest, res, "integer", integer, "string0",
                               str0, "missing_string", str1));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());
    EXPECT_EQ(str1, std::nullopt);
}

TEST(readJsonAction, InvalidMissingElement)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}, {"string", "hello"}};

    int integer = 0;
    std::string str0;
    std::string str1;
    std::vector<uint8_t> vec;
    EXPECT_FALSE(readJsonAction(jsonRequest, res, "missing_integer", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());

    EXPECT_FALSE(readJsonAction(jsonRequest, res, "missing_string", str0));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());

    EXPECT_FALSE(readJsonAction(jsonRequest, res, "integer", integer, "string",
                                str0, "vector", vec));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());

    EXPECT_FALSE(readJsonAction(jsonRequest, res, "integer", integer, "string0",
                                str0, "missing_string", str1));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());
}

TEST(readJsonAction, WrongElementType)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}, {"string0", "hello"}};

    std::optional<int64_t> integer = 0;
    std::optional<std::string> str0;
    std::optional<std::string> str1;
    EXPECT_FALSE(readJsonAction(jsonRequest, res, "integer", str0));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());

    EXPECT_FALSE(readJsonAction(jsonRequest, res, "string0", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());

    EXPECT_FALSE(
        readJsonAction(jsonRequest, res, "integer", str0, "string0", integer));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());
}

TEST(readJsonAction, JsonSubElementArray)
{
    crow::Response res;
    nlohmann::json jsonRequest = R"(
        {
            "array": [{"hello": "yes"}, [{"there": "no"}, "nice"]],
            "json": {"array": []},
            "string": "yes"
        }
    )"_json;

    std::optional<nlohmann::json> json0;
    std::optional<nlohmann::json> json1;
    std::optional<nlohmann::json> json2;
    std::optional<std::string> str;
    EXPECT_TRUE(readJsonPatch(jsonRequest, res, "array/0", json0, "array/1",
                              json1, "string", str, "json", json2));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());
    EXPECT_EQ(str, "yes");
}

TEST(readJsonAction, JsonSubElementValue)
{
    crow::Response res;
    nlohmann::json jsonRequest = R"(
        {
            "array": [{"hello": "yes"}, [{"there": "no"}, "nice"]],
            "json": {"array": []},
            "string": "yes"
        }
    )"_json;

    std::optional<nlohmann::json> json;
    std::optional<std::string> str0;
    std::optional<std::string> str1;
    std::optional<std::string> str2;
    std::optional<std::string> str3;
    std::optional<std::string> str4;
    EXPECT_TRUE(readJsonPatch(jsonRequest, res, "array/0/hello", str0,
                              "array/1/0/there", str1, "json/array", json,
                              "array/1/1", str2, "string", str3));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());
    EXPECT_EQ(str0, "yes");
    EXPECT_EQ(str1, "no");
    EXPECT_EQ(str2, "nice");
    EXPECT_EQ(str3, "yes");
    EXPECT_EQ(json->size(), 0);
}
