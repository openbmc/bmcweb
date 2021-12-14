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
                                  {"vector", std::vector<uint8_t>{1, 2, 3}}};

    int integer = 0;
    std::string str;
    std::vector<uint8_t> vec;
    EXPECT_TRUE(readJsonPatch(jsonRequest, res, "integer", integer, "string",
                              str, "vector", vec));
    EXPECT_EQ(integer, 1);
    EXPECT_EQ(str, "hello");
    EXPECT_TRUE((vec == std::vector<uint8_t>{1, 2, 3}));
}

TEST(readJsonPatch, MissingElements)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}, {"string0", "hello"}};

    int integer = 0;
    std::string str0, str1;
    std::vector<uint8_t> vec;
    EXPECT_FALSE(readJsonPatch(jsonRequest, res, "integer", integer));
    EXPECT_FALSE(readJsonPatch(jsonRequest, res, "integer", str0));
    EXPECT_FALSE(readJsonPatch(jsonRequest, res, "integer", integer, "string0",
                               str0, "vector", vec));
    EXPECT_FALSE(readJsonPatch(jsonRequest, res, "integer", integer, "string0",
                               str0, "string1", str1));
}

TEST(readJsonPatch, JsonSubElement)
{
    crow::Response res;
    nlohmann::json jsonRequest = R"(
        {
            "TestJson": [{"hello": "yes"}, {}]
        }
    )"_json;

    nlohmann::json json;
    std::string str;
    EXPECT_TRUE(readJsonPatch(jsonRequest, res, "TestJson", json));
    // Should we support fetching the subelemnts directdirectly?
    // EXPECT_TRUE(readJsonPatch(jsonRequest, res, "TestJson/0/", json));
    // EXPECT_TRUE(readJsonPatch(jsonRequest, res, "TestJson/1/", json));
}

TEST(readJsonAction, ValidEmptyElement)
{
    crow::Response res;
    nlohmann::json jsonRequest = {{"integer", 1}, {"string0", "hello"}};

    std::optional<int> integer;
    std::optional<std::string> str0, str1;
    std::optional<std::vector<uint8_t>> vec;
    EXPECT_TRUE(readJsonAction(jsonRequest, res, "integer", integer));
    EXPECT_EQ(integer, 1);

    EXPECT_TRUE(readJsonAction(jsonRequest, res, "integer", str0));
    EXPECT_NE(str0, std::nullopt);

    EXPECT_TRUE(readJsonAction(jsonRequest, res, "integer", integer, "string0",
                               str0, "vector", vec));
    EXPECT_EQ(str0, "hello");
    EXPECT_EQ(vec, std::nullopt);

    EXPECT_TRUE(readJsonAction(jsonRequest, res, "integer", integer, "string0",
                               str0, "string1", str1));
    EXPECT_EQ(str1, std::nullopt);
}
