#include "utils/json_utils.hpp"

#include <string>
#include <vector>

#include <gmock/gmock.h>

using redfish::json_util::readJsonAction;
using redfish::json_util::readJsonPatch;

template <typename... UnpackTypes>
void checkValidReadJsonPatch(nlohmann::json& jsonRequest, crow::Response& res,
                             const char* key, UnpackTypes&... in)
{
    EXPECT_TRUE(readJsonPatch(jsonRequest, res, key, in...));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());
}

template <typename... UnpackTypes>
void checkInvalidReadJsonPatch(nlohmann::json& jsonRequest, crow::Response& res,
                               const char* key, UnpackTypes&... in)
{
    EXPECT_FALSE(readJsonPatch(jsonRequest, res, key, in...));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());
}

template <typename... UnpackTypes>
void checkValidReadJsonAction(nlohmann::json& jsonRequest, crow::Response& res,
                              const char* key, UnpackTypes&... in)
{
    EXPECT_TRUE(readJsonAction(jsonRequest, res, key, in...));
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(res.jsonValue.empty());
}

template <typename... UnpackTypes>
void checkInvalidReadJsonAction(nlohmann::json& jsonRequest,
                                crow::Response& res, const char* key,
                                UnpackTypes&... in)
{
    EXPECT_FALSE(readJsonAction(jsonRequest, res, key, in...));
    EXPECT_EQ(res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(res.jsonValue.empty());
}

TEST(readJsonPatch, ValidElements)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1},
                                  {"string", "hello"},
                                  {"vector", std::vector<uint64_t>{1, 2, 3}}};

    int64_t integer = 0;
    std::string str;
    std::vector<uint64_t> vec;
    checkValidReadJsonPatch(jsonRequest, res, "integer", integer, "string", str,
                            "vector", vec);
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
    checkInvalidReadJsonPatch(jsonRequest, res, "integer", integer);
    checkInvalidReadJsonPatch(jsonRequest, res, "string0", str);
}

TEST(readJsonPatch, WrongElementType)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}, {"string0", "hello"}};

    int64_t integer = 0;
    std::string str0;
    std::string str1;
    checkInvalidReadJsonPatch(jsonRequest, res, "integer", str0);
    checkInvalidReadJsonPatch(jsonRequest, res, "string0", integer);
    checkInvalidReadJsonPatch(jsonRequest, res, "integer", str0, "string0",
                              integer);
}

TEST(readJsonPatch, MissingElement)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}, {"string0", "hello"}};

    int64_t integer = 0;
    std::string str0;
    std::string str1;
    std::vector<uint8_t> vec;
    checkInvalidReadJsonPatch(jsonRequest, res, "integer", integer, "string0",
                              str0, "vector", vec);
    checkInvalidReadJsonPatch(jsonRequest, res, "integer", integer, "string0",
                              str0, "string1", str1);
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
    checkValidReadJsonPatch(jsonRequest, res, "TestJson", jsonVec);
}

TEST(readJsonAction, ExtraElement)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}, {"string", "hello"}};

    std::optional<int> integer;
    std::optional<std::string> str;
    std::optional<std::vector<uint8_t>> vec;

    checkValidReadJsonAction(jsonRequest, res, "integer", integer);
    EXPECT_EQ(integer, 1);

    checkValidReadJsonAction(jsonRequest, res, "string", str);
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
    checkValidReadJsonAction(jsonRequest, res, "missing_integer", integer);
    EXPECT_EQ(integer, std::nullopt);

    checkValidReadJsonAction(jsonRequest, res, "missing_string", str0);
    EXPECT_EQ(str0, std::nullopt);

    checkValidReadJsonAction(jsonRequest, res, "integer", integer, "string",
                             str0, "vector", vec);
    EXPECT_EQ(integer, 1);
    EXPECT_EQ(str0, "hello");
    EXPECT_EQ(vec, std::nullopt);

    checkValidReadJsonAction(jsonRequest, res, "integer", integer, "string0",
                             str0, "missing_string", str1);
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
    checkInvalidReadJsonAction(jsonRequest, res, "missing_integer", integer);
    checkInvalidReadJsonAction(jsonRequest, res, "missing_string", str0);
    checkInvalidReadJsonAction(jsonRequest, res, "integer", integer, "string",
                               str0, "vector", vec);
    checkInvalidReadJsonAction(jsonRequest, res, "integer", integer, "string0",
                               str0, "missing_string", str1);
}

TEST(readJsonAction, WrongElementType)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}, {"string0", "hello"}};

    std::optional<int64_t> integer = 0;
    std::optional<std::string> str0;
    std::optional<std::string> str1;
    checkInvalidReadJsonAction(jsonRequest, res, "integer", str0);
    checkInvalidReadJsonAction(jsonRequest, res, "string0", integer);
    checkInvalidReadJsonAction(jsonRequest, res, "integer", str0, "string0",
                               integer);
}
