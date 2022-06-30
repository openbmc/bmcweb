#include "json_utils.hpp"

#include <nlohmann/json.hpp>

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

namespace redfish::json_util
{
namespace
{

using JsonArray = nlohmann::json::array_t;

TEST(SortJsonArrayByKey, ElementMissingKeyReturnsFalse)
{
    JsonArray array = R"([{"foo": 1}, {"bar" : 1}])"_json;
    EXPECT_FALSE(
        sortJsonArrayByKey<nlohmann::json::number_integer_t>("foo", array));
}

TEST(SortJsonArrayByKey, WrongValueTypeReturnsFalse)
{
    JsonArray array =
        R"([{"foo": "str"}, {"foo" : "str"}, {"foo" : "str"}])"_json;
    EXPECT_FALSE(
        sortJsonArrayByKey<nlohmann::json::number_integer_t>("foo", array));
}

TEST(SortJsonArrayByKey, SortedByStringValueOnSuccess)
{
    JsonArray array = R"([{"foo": "20"}, {"foo" : "3"}, {"foo" : "100"}])"_json;
    ASSERT_TRUE(sortJsonArrayByKey<nlohmann::json::string_t>("foo", array));
    EXPECT_EQ(array, R"([{"foo": "3"}, {"foo" : "20"}, {"foo" : "100"}])"_json);
}

TEST(SortJsonArrayByKey, SortedByIntegerValueOnSuccess)
{
    JsonArray array = R"([{"foo": 2}, {"foo" : 1}, {"foo" : 0}])"_json;
    ASSERT_TRUE(
        sortJsonArrayByKey<nlohmann::json::number_integer_t>("foo", array));
    EXPECT_EQ(array, R"([{"foo": 0}, {"foo" : 1}, {"foo" : 2}])"_json);
}

} // namespace
} // namespace redfish::json_util
