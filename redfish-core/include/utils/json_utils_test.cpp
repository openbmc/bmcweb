#include "json_utils.hpp"

#include <nlohmann/json.hpp>

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

namespace redfish::json_util
{
namespace
{

using JsonArray = nlohmann::json::array_t;

TEST(SortJsonArrayByKey, ElementMissingKeyReturnsFalseArrayIsPartlySorted)
{
    JsonArray array = R"([{"foo" : "100"}, {"bar": "1"}, {"foo" : "20"}])"_json;
    ASSERT_FALSE(sortJsonArrayByKey("foo", array));
    // Objects with other keys are always larger than those with the specified
    // key.
    EXPECT_EQ(array, R"([{"bar": "1"}, {"foo" : "20"}, {"foo" : "100"}])"_json);
}

TEST(SortJsonArrayByKey, SortedByStringValueOnSuccessArrayIsSorted)
{
    JsonArray array = R"([{"foo": "20"}, {"foo" : "3"}, {"foo" : "100"}])"_json;
    ASSERT_TRUE(sortJsonArrayByKey("foo", array));
    EXPECT_EQ(array, R"([{"foo": "3"}, {"foo" : "20"}, {"foo" : "100"}])"_json);
}

} // namespace
} // namespace redfish::json_util
